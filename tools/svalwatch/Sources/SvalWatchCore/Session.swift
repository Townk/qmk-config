/// The conversation with one connected board, as a state machine.
///
/// The driver feeds it timer firings, input reports and passive-input
/// notices; it answers with actions. It never touches IOKit or a clock
/// itself, so every path, including expiry and lost replies, runs in
/// tests.
///
/// One poll is: [bootstrap -> info, when needed] -> matrix -> [layer
/// state] -> wait. The layer is read only when a switch changed (almost
/// every layer change starts with one) or `layerInterval` has passed (for
/// the ones that don't, such as auto-mouse): each read is a USB round
/// trip, and the kernel side of those is most of what polling costs.
///
/// Polling is the fallback. First the session subscribes to the board's
/// announcements (users/townk/townk_announce.c): if the firmware grants
/// it, the board prints each change to its debug console, the driver
/// feeds those lines in through `console(_:at:)`, and the only traffic
/// left is renewing the lease every `renewInterval`. Firmware without the
/// module answers the subscribe without its signature, and the session
/// polls instead. So does a granted subscription that stays silent for
/// two renewals (each renewal re-announces the full state, so silence
/// means the console is not reaching us).
///
/// Only one request is ever outstanding; a reply that is not the one
/// awaited (someone else's, or a late duplicate) is ignored.
public struct Session: Sendable {
    public struct Config: Sendable {
        public var activeHz: Double = 60
        public var idleHz: Double = 4
        public var linger: Double = 2
        /// Longest gap between layer reads while no switch changes.
        public var layerInterval: Double = 1.0 / 15
        /// The board stamps IDs with the top 16 bits of its millisecond
        /// clock and rejects them 120 s after that stamp, so an ID can
        /// have as little as ~54 s left when issued. Renew well inside it.
        public var clientLifetime: Double = 45
        public var replyTimeout: Double = 0.25
        /// Try announcements before polling. The driver turns this off when
        /// it has no console to read them from.
        public var useAnnouncements = true
        /// How long the board keeps announcing without a renewal, and how
        /// often the session renews. The gap covers a missed renewal.
        public var leaseMs: UInt16 = 5000
        public var renewInterval: Double = 2

        public init() {}
    }

    public enum Action: Equatable, Sendable {
        case send([UInt8])
        case emit(WatchEvent)
        /// Replaces whatever timer was pending.
        case pollAfter(Double)
        case timeoutAfter(Double)
        /// Whether passive input from the board should wake polling early.
        /// Only wanted while idle: while active it would just add wakeups.
        case passiveWake(Bool)
    }

    private enum Phase: Equatable {
        case waiting
        case bootstrapping(nonce: [UInt8])
        case info
        case subscribing
        case layer
        case matrix
    }

    public enum Mode: Equatable, Sendable {
        /// Connected, announcements not tried yet.
        case probing
        case polling
        /// The board announces changes; only lease renewals go out.
        case announced
    }

    public private(set) var mode = Mode.probing

    public let serial: String
    private let config: Config
    private let makeNonce: @Sendable () -> [UInt8]

    private var phase = Phase.waiting
    private var client: UInt32?
    private var clientIssued = 0.0
    private var flags: InfoFlags?
    private var pacer: Pacer
    private var tracker = ChangeTracker()
    private var passiveArmed = false
    private var failures = 0
    private var pollStarted = 0.0
    private var lastLayerRead = -Double.infinity
    private var heardSinceRenewal = false
    private var silentRenewals = 0

    public private(set) var stats = Stats()

    public struct Stats: Equatable, Sendable {
        public var polls = 0
        public var renewals = 0
        public var announcements = 0
        public var timeouts = 0
        public var rejections = 0
    }

    public init(serial: String, config: Config = Config(),
                makeNonce: @escaping @Sendable () -> [UInt8]) {
        self.serial = serial
        self.config = config
        self.makeNonce = makeNonce
        pacer = Pacer(activeHz: config.activeHz, idleHz: config.idleHz, linger: config.linger)
    }

    /// The poll timer fired (or the board just connected).
    public mutating func poll(at now: Double) -> [Action] {
        guard phase == .waiting else { return [] }
        if mode == .announced {
            stats.renewals += 1
            silentRenewals = heardSinceRenewal ? 0 : silentRenewals + 1
            if silentRenewals >= 2 { mode = .polling }
        }
        if mode != .announced { stats.polls += 1 }
        pollStarted = now
        if client == nil || now - clientIssued >= config.clientLifetime {
            let nonce = makeNonce()
            phase = .bootstrapping(nonce: nonce)
            return request(SvalProtocol.bootstrapRequest(nonce: nonce))
        }
        return afterClient()
    }

    /// One line from the board's console, already parsed.
    public mutating func console(_ line: ConsoleParser.Line, at now: Double) -> [Action] {
        guard mode == .announced || phase == .subscribing else { return [] }
        stats.announcements += 1
        heardSinceRenewal = true
        let event: WatchEvent?
        switch line {
        case let .layer(state): event = tracker.update(layer: flags.map { state.masked(by: $0) } ?? state)
        case let .keys(pressed): event = tracker.update(keys: pressed)
        case let .pointer(state): event = tracker.update(pointer: state)
        }
        return event.map { [.emit($0)] } ?? []
    }

    /// The Mac saw input from the board itself (a key report, trackball
    /// motion). Polls now if the session is between polls.
    public mutating func passiveInput(at now: Double) -> [Action] {
        guard mode != .announced else { return [] }
        let wasIdle = pacer.mode(at: now) == .idle
        pacer.noteActivity(at: now)
        var actions = armPassive(false)
        if wasIdle && phase == .waiting { actions += poll(at: now) }
        return actions
    }

    public mutating func receive(_ report: [UInt8], at now: Double) -> [Action] {
        guard let reply = SvalProtocol.parse(report) else { return [] }
        switch (phase, reply) {
        case let (.bootstrapping(nonce), .bootstrap(echo, id)) where echo == nonce:
            guard id != 0, id != .max else { return fail(at: now) }
            client = id
            clientIssued = now
            if flags == nil {
                phase = .info
                return request(SvalProtocol.infoRequest(client: id))
            }
            return afterClient()

        case let (.info, .info(id, f)) where id == client:
            let first = flags == nil
            flags = f
            var actions: [Action] = first
                ? [.emit(.connected(serial: serial, protocolVersion: Int(f.version)))] : []
            actions += afterClient()
            return actions

        case let (.subscribing, .subscription(id, granted)) where id == client:
            failures = 0
            guard granted != nil else {
                mode = .polling
                return beginReads()
            }
            mode = .announced
            phase = .waiting
            heardSinceRenewal = false
            return armPassive(false) + [.pollAfter(config.renewInterval)]

        case let (.matrix, .matrix(id, pressed)) where id == client:
            failures = 0
            var actions: [Action] = []
            if let event = tracker.update(keys: pressed) {
                pacer.noteActivity(at: now)
                actions.append(.emit(event))
            }
            return actions + readLayerIfDue(changed: !actions.isEmpty, at: now)

        case let (.matrix, .malformed(id)) where id == client:
            return readLayerIfDue(changed: false, at: now)

        case let (.layer, .layerState(id, state)) where id == client:
            lastLayerRead = now
            var actions: [Action] = []
            if let event = tracker.update(layer: state.masked(by: flags!)) {
                pacer.noteActivity(at: now)
                actions.append(.emit(event))
            }
            return actions + finishPoll(at: now)

        case let (.info, .error(id, _)) where id == client,
             let (.subscribing, .error(id, _)) where id == client,
             let (.layer, .error(id, _)) where id == client,
             let (.matrix, .error(id, _)) where id == client:
            // Usually an expired ID; a fresh bootstrap fixes it.
            stats.rejections += 1
            return fail(at: now)

        default:
            return []
        }
    }

    /// The reply timer fired before the awaited reply arrived.
    public mutating func timeout(at now: Double) -> [Action] {
        guard phase != .waiting else { return [] }
        stats.timeouts += 1
        return fail(at: now)
    }

    private mutating func afterClient() -> [Action] {
        if mode == .probing && !config.useAnnouncements { mode = .polling }
        if mode == .polling { return beginReads() }
        phase = .subscribing
        return request(SvalProtocol.subscribeRequest(client: client!, leaseMs: config.leaseMs))
    }

    private mutating func beginReads() -> [Action] {
        phase = .matrix
        return request(SvalProtocol.matrixRequest(client: client!))
    }

    private mutating func readLayerIfDue(changed: Bool, at now: Double) -> [Action] {
        guard changed || now - lastLayerRead >= config.layerInterval else {
            return finishPoll(at: now)
        }
        phase = .layer
        return request(SvalProtocol.layerStateRequest(client: client!))
    }

    private func request(_ bytes: [UInt8]) -> [Action] {
        [.send(bytes), .timeoutAfter(config.replyTimeout)]
    }

    private mutating func finishPoll(at now: Double) -> [Action] {
        phase = .waiting
        let idle = pacer.mode(at: now) == .idle
        // Measured from the poll's start, so the rate is the rate.
        let wait = max(0, pacer.interval(at: now) - (now - pollStarted))
        return armPassive(idle) + [.pollAfter(wait)]
    }

    /// Drops the client ID and waits: right away the first time (an
    /// expired ID is the common case), at the idle rate after that, so a
    /// board that keeps refusing is not hammered.
    private mutating func fail(at now: Double) -> [Action] {
        phase = .waiting
        client = nil
        failures += 1
        return [.pollAfter(failures == 1 ? 0 : pacer.idleInterval)]
    }

    private mutating func armPassive(_ on: Bool) -> [Action] {
        guard passiveArmed != on else { return [] }
        passiveArmed = on
        return [.passiveWake(on)]
    }
}
