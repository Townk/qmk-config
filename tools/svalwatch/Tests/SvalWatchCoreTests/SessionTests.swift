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

    static func unhandled(id: UInt32 = client) -> [UInt8] {
        report([0xDD] + SvalProtocol.le32(id) + [0xFE, 0xFF, 0x57, 0xF7])
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

/// Runs the first poll through bootstrap, info, matrix and layer.
@discardableResult
func connect(_ s: inout Session, at t: Double = 0, active: UInt32 = 1,
             rows: [UInt8] = Array(repeating: 0, count: 10)) -> [Session.Action] {
    var all = s.poll(at: t)
    all += s.receive(Board.bootstrap(echo: nonce), at: t)
    all += s.receive(Board.info(), at: t)
    all += s.receive(Board.matrix(rows: rows), at: t)
    all += s.receive(Board.layer(active: active), at: t)
    return all
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
            header + [0xFE, 0x02],       // matrix
            header + [0xDF, 0x16],       // layer
        ]
        #expect(sent(actions).map { Array($0.prefix(7)) } == expected)
        #expect(emitted(actions) == [
            .connected(serial: "sval:TEST", protocolVersion: 3),
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
        actions += s.receive(Board.matrix(), at: 0)
        actions += s.receive(Board.layer(active: 1, default: 1, mods: 0x02, status: 0x01), at: 0)
        #expect(emitted(actions).last == .layer(LayerState(active: 1, default: 0)))
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
