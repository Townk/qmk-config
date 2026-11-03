import Testing
@testable import SvalWatchCore

// A fake board: builds the replies the firmware would send, with the
// layouts from modules/svalboard/core (client_wrapper.c, sval.c) and
// quantum/via.c.
enum Board {
    static let client: UInt32 = 0x1234_0007

    static func report(_ bytes: [UInt8]) -> [UInt8] {
        bytes + [UInt8](repeating: 0, count: 64 - bytes.count)
    }

    static func bootstrap(echo nonce: [UInt8], id: UInt32 = client) -> [UInt8] {
        report([0xDD, 0, 0, 0, 0] + nonce + SvalProtocol.le32(id) + [120, 0])
    }

    static func info(id: UInt32 = client, version: UInt8 = 3,
                     flags1: UInt8 = 0x6B, flags2: UInt8 = 0x0F) -> [UInt8] {
        var inner = [UInt8](repeating: 0, count: 20)
        inner[0] = 0xDF
        inner[1] = 0x00
        inner[2] = version
        inner[14] = flags1
        inner[19] = flags2
        return report([0xDD] + SvalProtocol.le32(id) + inner)
    }

    static func layer(id: UInt32 = client, active: UInt32, default def: UInt32 = 1,
                      mods: UInt8 = 0, status: UInt8 = 0) -> [UInt8] {
        report([0xDD] + SvalProtocol.le32(id) + [0xDF, 0x16]
            + SvalProtocol.le32(active) + SvalProtocol.le32(def) + [mods, status])
    }

    static func matrix(id: UInt32 = client, rows: [UInt8] = Array(repeating: 0, count: 10)) -> [UInt8] {
        report([0xDD] + SvalProtocol.le32(id) + [0xFE, 0x02, 0x03, 0x00] + rows)
    }

    /// The subscribe reply: townk_announce.c's when `granted`, otherwise
    /// what stock firmware sends back (the request, echoed untouched).
    static func subscription(id: UInt32 = client, granted: Bool = true, leaseMs: UInt16 = 5000) -> [UInt8] {
        let head = [0xDD] + SvalProtocol.le32(id) + [0xFE, 0x08, 0x57, 0xF7]
        let lease = [UInt8(leaseMs & 0xFF), UInt8(leaseMs >> 8)]
        return report(head + (granted ? Array("svw".utf8) + [1] + lease : lease))
    }

    /// VIA's "unhandled", as quantum/via.c sends it: the request itself,
    /// with 0xFF written over its command byte and everything else left
    /// as it was. The wrapper header stays (client_wrapper.c).
    static func unhandled(request: [UInt8]) -> [UInt8] {
        var r = request
        r[6] = 0xFF
        return report(Array(r.prefix(64)))
    }

    /// Firmware without townk_announce.c's subscribe answers it "unhandled".
    static func unhandled(id: UInt32 = client) -> [UInt8] {
        unhandled(request: SvalProtocol.subscribeRequest(client: id, leaseMs: 5000))
    }

    /// The fingerprint every test board was built with.
    static let fingerprint: UInt32 = 0xB68B_17F4

    /// townk_announce.c's fingerprint reply, or with nil what firmware
    /// from before it sends: "unhandled" on its channel.
    static func keydefs(id: UInt32 = client, _ fp: UInt32? = fingerprint) -> [UInt8] {
        guard let fp else { return unhandled(request: SvalProtocol.keydefsRequest(client: id)) }
        return report([0xDD] + SvalProtocol.le32(id) + [0xFE, 0x08, 0x57, 0xF8]
            + Array("kdf".utf8) + SvalProtocol.le32(fp))
    }

    /// What stock firmware sends back for the fingerprint read: the
    /// request, untouched (svalboard.c ignores the channel).
    static func stockKeydefs(id: UInt32 = client) -> [UInt8] {
        report(Array(SvalProtocol.keydefsRequest(client: id).prefix(64)))
    }

    /// VIA's layer count reply: the request echoed, the count after it.
    static func layerCount(id: UInt32 = client, _ layers: UInt8) -> [UInt8] {
        report([0xDD] + SvalProtocol.le32(id) + [0xFE, 0x11, layers])
    }

    /// VIA's keymap buffer reply: the request echoed (command, offset
    /// big-endian, size), then that many bytes of the keymap.
    static func keymapBuffer(id: UInt32 = client, offset: Int, _ bytes: [UInt8]) -> [UInt8] {
        report([0xDD] + SvalProtocol.le32(id)
            + [0xFE, 0x12, UInt8(offset >> 8), UInt8(offset & 0xFF), UInt8(bytes.count)] + bytes)
    }

    /// A keymap whose every keycode is its own index plus 0x100, stored
    /// big-endian as the board does, so a byte-order slip shows.
    static func keymapBytes(layers: Int) -> [UInt8] {
        (0..<layers * 60).flatMap { i -> [UInt8] in
            let kc = UInt16(i + 0x100)
            return [UInt8(kc >> 8), UInt8(kc & 0xFF)]
        }
    }

    /// Answers each buffer request in `actions` from `bytes`, the way the
    /// board does, until the session stops asking.
    static func answerKeymap(_ s: inout Session, _ actions: [Session.Action], bytes: [UInt8],
                             at t: Double = 0) -> [Session.Action] {
        var all: [Session.Action] = []
        var pending = actions
        while let req = sent(pending).last, req[5] == 0xFE, req[6] == 0x12 {
            let offset = Int(req[7]) << 8 | Int(req[8])
            pending = s.receive(keymapBuffer(offset: offset, Array(bytes[offset..<offset + Int(req[9])])), at: t)
            all += pending
        }
        return all
    }

    static func rejected(id: UInt32 = client) -> [UInt8] {
        report([0xDD] + SvalProtocol.le32(id) + [0xFF, 0x01])
    }
}

let nonce = Array(UInt8(1)...UInt8(20))

/// A polling session: these tests are about the fallback, so they skip
/// the subscribe attempt (AnnouncedTests covers it).
func makeSession(_ tweak: (inout Session.Config) -> Void = { _ in }) -> Session {
    var config = Session.Config()
    config.useAnnouncements = false
    tweak(&config)
    return Session(serial: "sval:TEST", config: config, makeNonce: { nonce })
}

func sent(_ actions: [Session.Action]) -> [[UInt8]] {
    actions.compactMap { if case let .send(b) = $0 { b } else { nil } }
}

func emitted(_ actions: [Session.Action]) -> [WatchEvent] {
    actions.compactMap { if case let .emit(e) = $0 { e } else { nil } }
}

func nextPoll(_ actions: [Session.Action]) -> Double? {
    actions.lazy.compactMap { if case let .pollAfter(d) = $0 { d } else { nil } }.first
}

/// Runs the first poll through bootstrap, info, keymap, matrix and layer.
@discardableResult
func connect(_ s: inout Session, at t: Double = 0, active: UInt32 = 1,
             rows: [UInt8] = Array(repeating: 0, count: 10)) -> [Session.Action] {
    var all = s.poll(at: t)
    all += s.receive(Board.bootstrap(echo: nonce), at: t)
    all += s.receive(Board.info(), at: t)
    all += readKeymap(&s, at: t)
    all += s.receive(Board.matrix(rows: rows), at: t)
    all += s.receive(Board.layer(active: active), at: t)
    return all
}

/// Answers the keymap read that follows the first info (one layer), and
/// the fingerprint read after it. Fails the calling test when the session
/// was not reading them.
@discardableResult
func readKeymap(_ s: inout Session, at t: Double = 0, layers: Int = 1,
                keydefs: UInt32? = Board.fingerprint,
                sourceLocation: SourceLocation = #_sourceLocation) -> [Session.Action] {
    let count = s.receive(Board.layerCount(UInt8(layers)), at: t)
    #expect(sent(count).first?[6] == 0x12, "no keymap read in progress", sourceLocation: sourceLocation)
    var all = count + Board.answerKeymap(&s, count, bytes: Board.keymapBytes(layers: layers), at: t)
    #expect(sent(all).last.map { Array($0[5..<9]) } == [0xFE, 0x08, 0x57, 0xF8],
            "no fingerprint read after the keymap", sourceLocation: sourceLocation)
    all += s.receive(Board.keydefs(keydefs), at: t)
    #expect(emitted(all).contains { if case .keymap = $0 { true } else { false } },
            "no keymap emitted", sourceLocation: sourceLocation)
    return all
}

func isKeymap(_ e: WatchEvent) -> Bool {
    if case .keymap = e { true } else { false }
}

/// One later poll with a valid client ID: the matrix reply, then the
/// layer reply if the session asks for it.
@discardableResult
func pollOnce(_ s: inout Session, at t: Double, active: UInt32 = 1,
              rows: [UInt8] = Array(repeating: 0, count: 10)) -> [Session.Action] {
    var all = s.poll(at: t)
    all += s.receive(Board.matrix(rows: rows), at: t)
    if sent(all).last?[6] == 0x16 { all += s.receive(Board.layer(active: active), at: t) }
    return all
}

let header = [0xDD] + SvalProtocol.le32(Board.client)
let rowOne: [UInt8] = [0, 0b100, 0, 0, 0, 0, 0, 0, 0, 0]

@Suite struct SessionTests {
    @Test func firstPollBootstrapsAndReportsEverything() {
        var s = makeSession()
        let actions = connect(&s, active: 0x8001, rows: rowOne)
        // Typed up front: Xcode 16's type checker times out on this
        // literal inside #expect.
        let expected: [[UInt8]] = [
            [0xDD, 0, 0, 0, 0, 1, 2],    // bootstrap + nonce
            header + [0xDF, 0x00],       // info
            header + [0xFE, 0x11],       // keymap layer count
            header + [0xFE, 0x12],       // keymap buffer: 54 + 54 + 12 bytes
            header + [0xFE, 0x12],
            header + [0xFE, 0x12],
            header + [0xFE, 0x08],       // key-definitions fingerprint
            header + [0xFE, 0x02],       // matrix
            header + [0xDF, 0x16],       // layer
        ]
        #expect(sent(actions).map { Array($0.prefix(7)) } == expected)
        #expect(emitted(actions) == [
            .connected(serial: "sval:TEST", protocolVersion: 3),
            .keymap(Keymap(layers: 1, rows: 10, cols: 6, keycodes: (0..<60).map { UInt16($0 + 0x100) }),
                    keydefs: Board.fingerprint),
            .keys([KeyPosition(row: 1, col: 2)]),
            .layer(LayerState(active: 0x8001, default: 1)),
        ])
    }

    @Test func pollsAreSpacedFromTheirStart() {
        var s = makeSession()
        _ = connect(&s, at: 0)
        var actions = s.poll(at: 1)
        actions += s.receive(Board.matrix(), at: 1.004)
        actions += s.receive(Board.layer(active: 1), at: 1.006)
        #expect(abs(nextPoll(actions)! - (1.0 / 60 - 0.006)) < 1e-9)
        // A poll that overran its interval asks for the next one at once.
        actions = s.poll(at: 1.1)
        actions += s.receive(Board.matrix(), at: 1.13)
        actions += s.receive(Board.layer(active: 1), at: 1.135)
        #expect(nextPoll(actions) == 0)
    }

    @Test func unchangedSwitchesSkipTheLayerReadUntilItIsDue() {
        var s = makeSession()
        connect(&s, at: 0)
        let soon = pollOnce(&s, at: 0.02)
        #expect(sent(soon).count == 1)          // matrix only, no new bootstrap
        #expect(emitted(soon).isEmpty)
        let due = pollOnce(&s, at: 0.07, active: 1 << 15)
        #expect(sent(due).map { $0[5] } == [0xFE, 0xDF])
        #expect(emitted(due) == [.layer(LayerState(active: 1 << 15, default: 1))])
    }

    @Test func aSwitchChangeReadsTheLayerAtOnce() {
        var s = makeSession()
        connect(&s, at: 0)
        let actions = pollOnce(&s, at: 0.02, active: 1 << 5, rows: rowOne)
        #expect(emitted(actions) == [
            .keys([KeyPosition(row: 1, col: 2)]),
            .layer(LayerState(active: 1 << 5, default: 1)),
        ])
    }

    @Test func dropsToTheIdleRateAfterLinger() {
        var s = makeSession()
        connect(&s, at: 0)
        let actions = pollOnce(&s, at: 2.5)
        #expect(nextPoll(actions) == 0.25)
        #expect(actions.contains(.passiveWake(true)))
        #expect(sent(actions).count == 2)  // idle polls always read both
    }

    @Test func passiveInputWhileIdlePollsAtOnceAndDisarms() {
        var s = makeSession()
        connect(&s, at: 0)
        pollOnce(&s, at: 3)  // idle, armed
        let actions = s.passiveInput(at: 3.1)
        #expect(actions.first == .passiveWake(false))
        #expect(sent(actions).map { $0[5] } == [0xFE])
        let rest = s.receive(Board.matrix(), at: 3.1) + s.receive(Board.layer(active: 1), at: 3.1)
        #expect(abs(nextPoll(rest)! - 1.0 / 60) < 1e-9)
    }

    @Test func passiveInputWhileActiveDoesNotPollEarly() {
        var s = makeSession()
        connect(&s, at: 0)
        #expect(s.passiveInput(at: 0.005).isEmpty)
    }

    @Test func aChangeKeepsPollingFast() {
        var s = makeSession()
        connect(&s, at: 0)
        pollOnce(&s, at: 1.9, active: 1 << 5)  // numbers layer up
        let actions = pollOnce(&s, at: 3.5, active: 1 << 5)
        #expect(abs(nextPoll(actions)! - 1.0 / 60) < 1e-9)
    }

    @Test func renewsTheClientIdBeforeTheBoardExpiresIt() {
        var s = makeSession()
        connect(&s, at: 0)
        pollOnce(&s, at: 44)
        let renew = s.poll(at: 45)
        #expect(sent(renew).first.map { Array($0.prefix(5)) } == [0xDD, 0, 0, 0, 0])
        // No second info read, no second "connected".
        let after = s.receive(Board.bootstrap(echo: nonce), at: 45)
        #expect(sent(after).first.map { Array($0.prefix(7)) } == header + [0xFE, 0x02])
        #expect(emitted(after).isEmpty)
    }

    @Test func aRejectedIdRebootstrapsAtOnceThenBacksOff() {
        var s = makeSession()
        connect(&s)
        _ = s.poll(at: 1)
        #expect(nextPoll(s.receive(Board.rejected(), at: 1)) == 0)
        _ = s.poll(at: 1)
        _ = s.receive(Board.bootstrap(echo: nonce), at: 1)
        #expect(nextPoll(s.receive(Board.rejected(), at: 1)) == 0.25)
        #expect(s.stats.rejections == 2)
    }

    @Test func otherClientsTrafficIsIgnored() {
        var s = makeSession()
        connect(&s)
        _ = s.poll(at: 1)
        #expect(s.receive(Board.matrix(id: 0x9999_0001, rows: rowOne), at: 1).isEmpty)
        #expect(s.receive(Board.layer(active: 0xFF), at: 1).isEmpty)  // not awaited
        #expect(s.receive(Board.bootstrap(echo: Array(repeating: 9, count: 20)), at: 1).isEmpty)
        #expect(s.receive(Board.rejected(id: 0x9999_0001), at: 1).isEmpty)
        let ours = s.receive(Board.matrix(rows: rowOne), at: 1)
        #expect(emitted(ours) == [.keys([KeyPosition(row: 1, col: 2)])])
    }

    @Test func aLostReplyTimesOutAndRetries() {
        var s = makeSession()
        connect(&s)
        let poll = s.poll(at: 1)
        #expect(poll.contains(.timeoutAfter(0.25)))
        #expect(nextPoll(s.timeout(at: 1.25)) == 0)
        #expect(s.stats.timeouts == 1)
        // A late reply after the timeout is not taken for the new poll.
        #expect(s.receive(Board.matrix(rows: rowOne), at: 1.3).isEmpty)
    }

    @Test func aTimeoutWithNothingOutstandingIsIgnored() {
        var s = makeSession()
        connect(&s)
        #expect(s.timeout(at: 5).isEmpty)
    }

    @Test func malformedMatrixRowsAreDropped() {
        var s = makeSession()
        connect(&s)
        _ = s.poll(at: 1)
        let actions = s.receive(Board.matrix(rows: [0x40] + Array(repeating: 0, count: 9)), at: 1)
        #expect(emitted(actions).isEmpty)
        #expect(sent(actions).map { $0[6] } == [0x16])  // still reads the due layer
    }

    @Test func unadvertisedBytesAreMaskedOff() {
        var s = makeSession()
        var actions = s.poll(at: 0)
        actions += s.receive(Board.bootstrap(echo: nonce), at: 0)
        actions += s.receive(Board.info(flags1: 0x0B, flags2: 0x00), at: 0)
        actions += readKeymap(&s)
        actions += s.receive(Board.matrix(), at: 0)
        actions += s.receive(Board.layer(active: 1, default: 1, mods: 0x02, status: 0x01), at: 0)
        #expect(emitted(actions).last == .layer(LayerState(active: 1, default: 0)))
    }
}

@Suite struct KeymapTests {
    @Test func readsTheKeymapAfterInfo() {
        var s = makeSession()
        var actions = s.poll(at: 0)
        actions += s.receive(Board.bootstrap(echo: nonce), at: 0)
        actions = s.receive(Board.info(), at: 0)
        #expect(sent(actions).map { Array($0.prefix(7)) } == [header + [0xFE, 0x11]])
        #expect(actions.contains(.timeoutAfter(0.25)))

        // Two layers: 2 * 10 * 6 keycodes of two bytes, 54 bytes a reply.
        let bytes = Board.keymapBytes(layers: 2)
        var reads: [[UInt8]] = []
        actions = s.receive(Board.layerCount(2), at: 0)
        var all: [Session.Action] = []
        while let req = sent(actions).last, req[6] == 0x12 {
            reads.append(Array(req[5..<10]))
            let offset = Int(req[7]) << 8 | Int(req[8])
            actions = s.receive(Board.keymapBuffer(offset: offset, Array(bytes[offset..<offset + Int(req[9])])), at: 0)
            all += actions
        }
        let expected: [[UInt8]] = [
            [0xFE, 0x12, 0, 0, 54],
            [0xFE, 0x12, 0, 54, 54],
            [0xFE, 0x12, 0, 108, 54],
            [0xFE, 0x12, 0, 162, 54],
            [0xFE, 0x12, 0, 216, 24],
        ]
        #expect(reads == expected)
        // Not emitted yet: the fingerprint read comes first.
        #expect(emitted(all).isEmpty)
        #expect(sent(all).last.map { Array($0.prefix(9)) } == header + [0xFE, 0x08, 0x57, 0xF8])
        all = s.receive(Board.keydefs(), at: 0)
        #expect(emitted(all) == [.keymap(Keymap(layers: 2, rows: 10, cols: 6,
                                                 keycodes: (0..<120).map { UInt16($0 + 0x100) }),
                                          keydefs: Board.fingerprint)])
        // Then the reads polling starts with.
        #expect(sent(all).last.map { Array($0.prefix(7)) } == header + [0xFE, 0x02])
    }

    @Test func announcedReadsTheKeymapAfterSubscribing() {
        var s = announcedSession()
        _ = s.poll(at: 0)
        _ = s.receive(Board.bootstrap(echo: nonce), at: 0)
        _ = s.receive(Board.info(), at: 0)
        let actions = s.receive(Board.subscription(), at: 0)
        #expect(sent(actions).last.map { Array($0.prefix(7)) } == header + [0xFE, 0x11])
        #expect(nextPoll(actions) == nil)
        let rest = readKeymap(&s)
        #expect(emitted(rest).count == 1)
        #expect(nextPoll(rest) == 2)  // then only renewals
        #expect(s.mode == .announced)
    }

    @Test func readsItOncePerConnect() {
        var s = makeSession()
        connect(&s)
        pollOnce(&s, at: 44)
        _ = s.poll(at: 45)
        let after = s.receive(Board.bootstrap(echo: nonce), at: 45)
        #expect(sent(after).map { $0[6] } == [0x02])
    }

    /// Bootstrap and info, leaving the session waiting on the layer count.
    func untilLayerCount(_ s: inout Session) {
        _ = s.poll(at: 0)
        _ = s.receive(Board.bootstrap(echo: nonce), at: 0)
        _ = s.receive(Board.info(), at: 0)
    }

    @Test func dropsAPartialKeymapOnTimeout() {
        var s = makeSession()
        untilLayerCount(&s)
        _ = s.receive(Board.layerCount(2), at: 0)
        let first = s.receive(Board.keymapBuffer(offset: 0, Array(Board.keymapBytes(layers: 2)[0..<54])), at: 0)
        #expect(emitted(first).isEmpty)
        let gaveUp = s.timeout(at: 0.25)
        #expect(emitted(gaveUp).isEmpty)
        #expect(s.stats.timeouts == 1)
        #expect(s.stats.keymapFailures == 1)
        // The rest of the keymap is not taken late, half a keymap never
        // shows up, and the session keeps its client ID.
        let late = s.receive(Board.keymapBuffer(offset: 54, Array(Board.keymapBytes(layers: 2)[54..<108])), at: 0.3)
        #expect(late.isEmpty)
        #expect(sent(gaveUp).map { Array($0.prefix(7)) } == [header + [0xFE, 0x02]])
    }

    @Test func aFailedKeymapReadStillWatchesLayers() {
        var s = makeSession()
        untilLayerCount(&s)
        _ = s.receive(Board.layerCount(2), at: 0)
        _ = s.receive(Board.keymapBuffer(offset: 0, Array(Board.keymapBytes(layers: 2)[0..<54])), at: 0)
        var actions = s.timeout(at: 0.25)
        #expect(sent(actions).map { $0[6] } == [0x02])  // the matrix read
        actions = s.receive(Board.matrix(rows: rowOne), at: 0.25)
        actions += s.receive(Board.layer(active: 1 << 5), at: 0.25)
        #expect(emitted(actions) == [
            .keys([KeyPosition(row: 1, col: 2)]),
            .layer(LayerState(active: 1 << 5, default: 1)),
        ])
        // A new client ID does not try the keymap again.
        pollOnce(&s, at: 44)
        _ = s.poll(at: 45)
        let after = s.receive(Board.bootstrap(echo: nonce), at: 45)
        #expect(sent(after).map { $0[6] } == [0x02])
    }

    @Test func aRejectedKeymapReadRebootstrapsWithoutIt() {
        var s = makeSession()
        untilLayerCount(&s)
        #expect(nextPoll(s.receive(Board.rejected(), at: 0)) == 0)
        #expect(s.stats.rejections == 1)
        #expect(s.stats.keymapFailures == 1)
        _ = s.poll(at: 0)
        let after = s.receive(Board.bootstrap(echo: nonce), at: 0)
        #expect(sent(after).map { $0[6] } == [0x02])
    }

    @Test(arguments: [UInt8(0), 33, 255])
    func anImpossibleLayerCountIsAFailedRead(layers: UInt8) {
        var s = makeSession()
        untilLayerCount(&s)
        let actions = s.receive(Board.layerCount(layers), at: 0)
        #expect(emitted(actions).isEmpty)
        #expect(sent(actions).map { $0[6] } == [0x02])
        #expect(s.stats.keymapFailures == 1)
    }

    @Test func aShortChunkIsAFailedRead() {
        var s = makeSession()
        untilLayerCount(&s)
        _ = s.receive(Board.layerCount(1), at: 0)
        let actions = s.receive(Board.keymapBuffer(offset: 0, Array(repeating: 4, count: 20)), at: 0)
        #expect(!emitted(actions).contains(where: isKeymap))
        #expect(sent(actions).map { $0[6] } == [0x02])
        #expect(s.stats.keymapFailures == 1)
    }

    @Test func viaWithoutAKeymapIsAFailedRead() {
        var s = makeSession()
        untilLayerCount(&s)
        let request = SvalProtocol.keymapLayerCountRequest(client: Board.client)
        let actions = s.receive(Board.unhandled(request: request), at: 0)
        #expect(sent(actions).map { $0[6] } == [0x02])
        #expect(s.stats.keymapFailures == 1)
        #expect(actions.contains(.log("keymap read failed (VIA has no dynamic keymap); watching without it")))
    }

    @Test func anUnhandledBufferReadIsAFailedRead() {
        var s = makeSession()
        untilLayerCount(&s)
        let first = s.receive(Board.layerCount(2), at: 0)
        _ = s.receive(Board.keymapBuffer(offset: 0, Array(Board.keymapBytes(layers: 2)[0..<54])), at: 0)
        // An "unhandled" for the chunk already read is not this one's.
        #expect(s.receive(Board.unhandled(request: sent(first).last!), at: 0).isEmpty)
        let request = SvalProtocol.keymapBufferRequest(client: Board.client, offset: 54, size: 54)
        let actions = s.receive(Board.unhandled(request: request), at: 0)
        #expect(sent(actions).map { $0[6] } == [0x02])
        #expect(s.stats.keymapFailures == 1)
        #expect(!emitted(actions).contains(where: isKeymap))
    }

    @Test func eachFailedKeymapReadIsLoggedOnce() {
        var s = makeSession()
        untilLayerCount(&s)
        let actions = s.timeout(at: 0.25)
        #expect(actions.filter { if case .log = $0 { true } else { false } }
            == [.log("keymap read failed (no reply); watching without it")])
    }

    /// Bootstrap, info and the whole keymap, leaving the session waiting
    /// on the fingerprint.
    func untilKeydefs(_ s: inout Session) {
        untilLayerCount(&s)
        let count = s.receive(Board.layerCount(1), at: 0)
        _ = Board.answerKeymap(&s, count, bytes: Board.keymapBytes(layers: 1))
    }

    let oneLayer = Keymap(layers: 1, rows: 10, cols: 6, keycodes: (0..<60).map { UInt16($0 + 0x100) })

    @Test func firmwareWithoutTheFingerprintStillSendsTheKeymap() {
        var s = makeSession()
        untilKeydefs(&s)
        let actions = s.receive(Board.keydefs(nil), at: 0)
        #expect(emitted(actions) == [.keymap(oneLayer, keydefs: nil)])
        #expect(sent(actions).map { $0[6] } == [0x02])
        #expect(s.stats.keydefsFailures == 0)  // it answered: no fingerprint is not a failure
        #expect(!actions.contains { if case .log = $0 { true } else { false } })
    }

    @Test func stockFirmwareHasNoFingerprintEither() {
        var s = makeSession()
        untilKeydefs(&s)
        #expect(emitted(s.receive(Board.stockKeydefs(), at: 0)) == [.keymap(oneLayer, keydefs: nil)])
    }

    @Test func aLostFingerprintReplySendsTheKeymapWithout() {
        var s = makeSession()
        untilKeydefs(&s)
        let actions = s.timeout(at: 0.25)
        #expect(emitted(actions) == [.keymap(oneLayer, keydefs: nil)])
        #expect(actions.contains(.log("fingerprint read failed (no reply)")))
        #expect(sent(actions).map { $0[6] } == [0x02])  // and watching goes on
        #expect(s.stats.keydefsFailures == 1)
        #expect(s.stats.keymapFailures == 0)
        // A late reply is not taken for anything.
        #expect(s.receive(Board.keydefs(), at: 0.3).isEmpty)
    }

    @Test func aRejectedFingerprintReadSendsTheKeymapAndRebootstraps() {
        var s = makeSession()
        untilKeydefs(&s)
        let actions = s.receive(Board.rejected(), at: 0)
        #expect(emitted(actions) == [.keymap(oneLayer, keydefs: nil)])
        #expect(nextPoll(actions) == 0)
        #expect(s.stats.keydefsFailures == 1)
        _ = s.poll(at: 0)
        let after = s.receive(Board.bootstrap(echo: nonce), at: 0)
        #expect(sent(after).map { $0[6] } == [0x02])  // neither read again
    }

    @Test func anotherClientsFingerprintIsIgnored() {
        var s = makeSession()
        untilKeydefs(&s)
        #expect(s.receive(Board.keydefs(id: 0x9999_0001, 0x1111_1111), at: 0).isEmpty)
        #expect(emitted(s.receive(Board.keydefs(), at: 0)) == [.keymap(oneLayer, keydefs: Board.fingerprint)])
    }

    @Test func ignoresAnotherClientsBufferReply() {
        var s = makeSession()
        _ = s.poll(at: 0)
        _ = s.receive(Board.bootstrap(echo: nonce), at: 0)
        _ = s.receive(Board.info(), at: 0)
        #expect(s.receive(Board.layerCount(id: 0x9999_0001, 4), at: 0).isEmpty)
        _ = s.receive(Board.layerCount(1), at: 0)
        let bytes = Board.keymapBytes(layers: 1)
        #expect(s.receive(Board.keymapBuffer(id: 0x9999_0001, offset: 0, Array(bytes[0..<54])), at: 0).isEmpty)
        // Nor one for an offset not asked for (a late duplicate).
        #expect(s.receive(Board.keymapBuffer(offset: 54, Array(bytes[54..<108])), at: 0).isEmpty)
        let ours = s.receive(Board.keymapBuffer(offset: 0, Array(bytes[0..<54])), at: 0)
        #expect(sent(ours).map { Array($0[6..<10]) } == [[0x12, 0, 54, 54]])
    }

}

@Suite struct SvalProtocolTests {
    @Test func keymapLayerCountRequest() {
        let r = SvalProtocol.keymapLayerCountRequest(client: Board.client)
        #expect(r.count == 64)
        #expect(Array(r.prefix(7)) == header + [0xFE, 0x11])
        #expect(r[7...].allSatisfy { $0 == 0 })
    }

    @Test func keymapBufferRequest() {
        let r = SvalProtocol.keymapBufferRequest(client: Board.client, offset: 0x0136, size: 54)
        #expect(r.count == 64)
        #expect(Array(r.prefix(10)) == header + [0xFE, 0x12, 0x01, 0x36, 54])
        #expect(r[10...].allSatisfy { $0 == 0 })
    }

    @Test func theBufferChunkFillsAWrappedReport() {
        // 64 - 6 (wrapper header the reply loses) - 4 (VIA's echo).
        #expect(SvalProtocol.keymapChunkSize == 54)
    }

    @Test func parsesKeymapReplies() {
        #expect(SvalProtocol.parse(Board.layerCount(8)) == .keymapLayerCount(client: Board.client, 8))
        #expect(SvalProtocol.parse(Board.keymapBuffer(offset: 0x0136, [0x00, 0x04, 0x52, 0x29]))
            == .keymapBuffer(client: Board.client, offset: 0x0136, bytes: [0x00, 0x04, 0x52, 0x29]))
        let buffer = SvalProtocol.keymapBufferRequest(client: Board.client, offset: 0x0136, size: 54)
        #expect(SvalProtocol.parse(Board.unhandled(request: buffer))
            == .unhandled(client: Board.client, echo: [0x01, 0x36, 54]))
    }

    @Test func keydefsRequest() {
        let r = SvalProtocol.keydefsRequest(client: Board.client)
        #expect(r.count == 64)
        #expect(Array(r.prefix(9)) == header + [0xFE, 0x08, 0x57, 0xF8])
        #expect(r[9...].allSatisfy { $0 == 0 })
    }

    @Test func parsesKeydefsReplies() {
        #expect(SvalProtocol.parse(Board.keydefs(0x1234_ABCD))
            == .keydefs(client: Board.client, fingerprint: 0x1234_ABCD))
        #expect(SvalProtocol.parse(Board.keydefs(nil)) == .keydefs(client: Board.client, fingerprint: nil))
        #expect(SvalProtocol.parse(Board.stockKeydefs()) == .keydefs(client: Board.client, fingerprint: nil))
    }
}

@Suite struct LayerStateTests {
    @Test(arguments: [
        (UInt32(0), UInt32(0), 0),
        (UInt32(0), UInt32(1), 0),
        (UInt32(0x8001), UInt32(1), 15),
        (UInt32(1 << 5), UInt32(1 << 1), 5),
        (UInt32(0), UInt32(1 << 1), 1),
        (UInt32(1) << 31, UInt32(1), 31),
    ])
    func topLayer(active: UInt32, def: UInt32, top: Int) {
        #expect(LayerState(active: active, default: def).topLayer == top)
    }
}

@Suite struct WatchEventTests {
    @Test func jsonLines() {
        #expect(WatchEvent.connected(serial: "sval:\"x\"", protocolVersion: 3).jsonLine
            == #"{"type":"connected","serial":"sval:\"x\"","protocol":3}"#)
        #expect(WatchEvent.layer(LayerState(active: 0x8001, default: 1, mods: 2, status: 3)).jsonLine
            == #"{"type":"layer","top":15,"active":32769,"default":1,"mods":2,"caps_word":true,"caps_lock":true}"#)
        #expect(WatchEvent.keys([KeyPosition(row: 0, col: 3), KeyPosition(row: 6, col: 1)]).jsonLine
            == #"{"type":"keys","pressed":[[0,3],[6,1]]}"#)
        #expect(WatchEvent.keys([]).jsonLine == #"{"type":"keys","pressed":[]}"#)
        #expect(WatchEvent.disconnected.jsonLine == #"{"type":"disconnected"}"#)
        #expect(WatchEvent.keymap(Keymap(layers: 1, rows: 1, cols: 3, keycodes: [4, 0x5229, 0]),
                                  keydefs: 0x0B68_17F4).jsonLine
            == #"{"type":"keymap","layers":1,"keydefs":"0x0B6817F4","keycodes":[4,21033,0]}"#)
        #expect(WatchEvent.keymap(Keymap(layers: 1, rows: 1, cols: 3, keycodes: [4, 0, 0]), keydefs: nil).jsonLine
            == #"{"type":"keymap","layers":1,"keydefs":null,"keycodes":[4,0,0]}"#)
        #expect(WatchEvent.pointer(PointerState(leftDPI: 400, rightDPI: 1600, leftScroll: true,
                                                rightScroll: false, scrollKey: false)).jsonLine
            == #"{"type":"pointer","left":{"dpi":400,"scroll":true},"right":{"dpi":1600,"scroll":false},"scroll_key":false,"sniper":1,"boost":1}"#)
        #expect(WatchEvent.pointer(PointerState(leftDPI: 400, rightDPI: 1600, leftScroll: true,
                                                rightScroll: false, scrollKey: false, sniperDiv: 15, boostMult: 2)).jsonLine
            == #"{"type":"pointer","left":{"dpi":400,"scroll":true},"right":{"dpi":1600,"scroll":false},"scroll_key":false,"sniper":15,"boost":2}"#)
    }

    @Test func trackerReportsFirstReadingAndChangesOnly() {
        var t = ChangeTracker()
        #expect(t.update(keys: []) == .keys([]))
        #expect(t.update(keys: []) == nil)
        #expect(t.update(keys: [KeyPosition(row: 2, col: 1), KeyPosition(row: 0, col: 4)])
            == .keys([KeyPosition(row: 0, col: 4), KeyPosition(row: 2, col: 1)]))
        t.reset()
        #expect(t.update(keys: []) == .keys([]))
    }
}
