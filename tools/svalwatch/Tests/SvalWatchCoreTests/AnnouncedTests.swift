import Testing
@testable import SvalWatchCore

func announcedSession() -> Session {
    Session(serial: "sval:TEST", makeNonce: { nonce })
}

/// Bootstrap, info, then the subscribe that townk_announce.c grants.
@discardableResult
func subscribe(_ s: inout Session, at t: Double = 0, granted: Bool = true) -> [Session.Action] {
    var all = s.poll(at: t)
    all += s.receive(Board.bootstrap(echo: nonce), at: t)
    all += s.receive(Board.info(), at: t)
    all += s.receive(Board.subscription(granted: granted), at: t)
    return all
}

let idleRows = ConsoleParser.Line.keys([])
let baseLayer = ConsoleParser.Line.layer(LayerState(active: 1, default: 1))

@Suite struct AnnouncedTests {
    @Test func subscribesAfterInfoAndThenOnlyRenews() {
        var s = announcedSession()
        let actions = subscribe(&s)
        #expect(sent(actions).map { Array($0.prefix(11)) }.last
            == header + [0xFE, 0x08, 0x57, 0xF7, 0x88, 0x13])  // 5000 ms lease
        #expect(s.mode == .announced)
        #expect(nextPoll(actions) == 2)
        #expect(emitted(actions) == [.connected(serial: "sval:TEST", protocolVersion: 3)])
        // The renewal is one subscribe, no reads.
        let renewal = s.poll(at: 2)
        #expect(sent(renewal).map { $0[6] } == [0x08])
    }

    @Test func consoleLinesBecomeEvents() {
        var s = announcedSession()
        subscribe(&s)
        #expect(emitted(s.console(idleRows, at: 0.01)) == [.keys([])])
        #expect(emitted(s.console(baseLayer, at: 0.01)) == [.layer(LayerState(active: 1, default: 1))])
        #expect(s.console(baseLayer, at: 2.01).isEmpty)  // a renewal's re-announcement
        let held = ConsoleParser.Line.keys([KeyPosition(row: 6, col: 1)])
        #expect(emitted(s.console(held, at: 2.5)) == [.keys([KeyPosition(row: 6, col: 1)])])
    }

    @Test func stockFirmwareFallsBackToPolling() {
        var s = announcedSession()
        let actions = subscribe(&s, granted: false)
        #expect(s.mode == .polling)
        #expect(sent(actions).last?[5] == 0xFE)
        #expect(sent(actions).last?[6] == 0x02)  // the matrix read
    }

    @Test func anUnhandledReplyFallsBackToo() {
        var s = announcedSession()
        _ = s.poll(at: 0)
        _ = s.receive(Board.bootstrap(echo: nonce), at: 0)
        _ = s.receive(Board.info(), at: 0)
        _ = s.receive(Board.unhandled(), at: 0)
        #expect(s.mode == .polling)
    }

    @Test func withoutAConsoleItNeverSubscribes() {
        var s = Session(serial: "sval:TEST", config: {
            var c = Session.Config()
            c.useAnnouncements = false
            return c
        }(), makeNonce: { nonce })
        let actions = connect(&s)
        #expect(!sent(actions).contains { $0[6] == 0x08 })
        #expect(s.mode == .polling)
    }

    @Test func aSilentSubscriptionFallsBackAfterTwoRenewals() {
        var s = announcedSession()
        subscribe(&s)
        _ = s.poll(at: 2)
        _ = s.receive(Board.subscription(), at: 2)
        #expect(s.mode == .announced)
        let third = s.poll(at: 4)
        #expect(s.mode == .polling)
        #expect(sent(third).map { $0[6] } == [0x02])
    }

    @Test func aHeardSubscriptionKeepsGoing() {
        var s = announcedSession()
        subscribe(&s)
        for t in [2.0, 4.0, 6.0] {
            _ = s.console(baseLayer, at: t - 0.5)
            _ = s.poll(at: t)
            _ = s.receive(Board.subscription(), at: t)
        }
        #expect(s.mode == .announced)
        #expect(s.stats.renewals == 3)
        #expect(s.stats.polls == 1)  // only the connecting one
    }

    @Test func pointerLinesBecomeEvents() {
        var s = announcedSession()
        subscribe(&s)
        let idle = PointerState(leftDPI: 400, rightDPI: 1600, leftScroll: true, rightScroll: false, scrollKey: false)
        #expect(emitted(s.console(.pointer(idle), at: 0.01)) == [.pointer(idle)])
        #expect(s.console(.pointer(idle), at: 2.01).isEmpty)  // a renewal's re-announcement
        var faster = idle
        faster.rightDPI = 2400
        #expect(emitted(s.console(.pointer(faster), at: 3)) == [.pointer(faster)])
    }

    @Test func linesBeforeSubscribingAreIgnored() {
        var s = announcedSession()
        _ = s.poll(at: 0)
        #expect(s.console(baseLayer, at: 0).isEmpty)
    }

    @Test func passiveInputDoesNotRenewEarly() {
        var s = announcedSession()
        subscribe(&s)
        #expect(s.passiveInput(at: 1).isEmpty)
    }

    @Test func anExpiredIdRebootstrapsBeforeRenewing() {
        var s = announcedSession()
        subscribe(&s)
        for t in stride(from: 2.0, through: 44, by: 2) {
            _ = s.console(baseLayer, at: t - 0.5)
            _ = s.poll(at: t)
            _ = s.receive(Board.subscription(), at: t)
        }
        _ = s.console(baseLayer, at: 45.5)
        let renewal = s.poll(at: 46)
        #expect(sent(renewal).first.map { Array($0.prefix(5)) } == [0xDD, 0, 0, 0, 0])
        let after = s.receive(Board.bootstrap(echo: nonce), at: 46)
        #expect(sent(after).first?[6] == 0x08)
    }

    @Test func aRejectedRenewalRetries() {
        var s = announcedSession()
        subscribe(&s)
        _ = s.console(baseLayer, at: 1)
        _ = s.poll(at: 2)
        #expect(nextPoll(s.receive(Board.rejected(), at: 2)) == 0)
        #expect(s.mode == .announced)
    }
}

@Suite struct ConsoleParserTests {
    func report(_ text: String, size: Int = 32) -> [UInt8] {
        let bytes = Array(text.utf8)
        return bytes + [UInt8](repeating: 0, count: max(0, size - bytes.count))
    }

    @Test func parsesBothLineKinds() {
        var p = ConsoleParser()
        #expect(p.feed(report("svw L 00008001 00000001 02 01\n")) == [
            .layer(LayerState(active: 0x8001, default: 1, mods: 2, status: 1)),
        ])
        #expect(p.feed(report("svw K 00040000000000000002\n")) == [
            .keys([KeyPosition(row: 1, col: 2), KeyPosition(row: 9, col: 1)]),
        ])
    }

    @Test func parsesThePointerLine() {
        var p = ConsoleParser()
        #expect(p.feed(report("svw P 0190 0640 01\n")) == [
            .pointer(PointerState(leftDPI: 400, rightDPI: 1600, leftScroll: true, rightScroll: false, scrollKey: false)),
        ])
        #expect(p.feed(report("svw P 2EE0 00C8 06\n")) == [
            .pointer(PointerState(leftDPI: 12000, rightDPI: 200, leftScroll: false, rightScroll: true, scrollKey: true)),
        ])
        for bad in ["svw P 190 0640 01\n", "svw P 0190 0640\n", "svw P 0190 0640 0G\n"] {
            #expect(p.feed(report(bad)).isEmpty, "\(bad)")
        }
    }

    @Test func joinsLinesSplitAcrossReports() {
        var p = ConsoleParser()
        #expect(p.feed(report("svw L 0000")).isEmpty)
        #expect(p.feed(report("0020 00000001 00 00\nsvw K 0000")) == [
            .layer(LayerState(active: 0x20, default: 1)),
        ])
        #expect(p.feed(report("0000000000000000\n")) == [.keys([])])
    }

    @Test func skipsEverythingElse() {
        var p = ConsoleParser()
        let noise = [
            "hello from the firmware\n",
            "svw L 1 2 3 4\n",
            "svw K 0000\n",
            "svw K 00C00000000000000000\n",  // column past the matrix
            "svw X 00\n",
            "svw L 0000000G 00000001 00 00\n",
        ]
        for line in noise { #expect(p.feed(report(line, size: 0)).isEmpty, "\(line)") }
        #expect(p.feed(report("svw K 00000000000000000000\n")) == [.keys([])])
    }

    @Test func anOverlongLineIsDroppedWhole() {
        var p = ConsoleParser()
        let long = String(repeating: "x", count: 200) + "svw K 00000000000000000000\n"
        #expect(p.feed(Array(long.utf8)).isEmpty)
        #expect(p.feed(report("svw K 00000000000000000000\n")) == [.keys([])])
    }

    @Test func subscribeRepliesParse() {
        #expect(SvalProtocol.parse(Board.subscription())
            == .subscription(client: Board.client, granted: Announcement(version: 1, leaseMs: 5000)))
        #expect(SvalProtocol.parse(Board.subscription(granted: false))
            == .subscription(client: Board.client, granted: nil))
        #expect(SvalProtocol.parse(Board.unhandled()) == .subscription(client: Board.client, granted: nil))
    }
}
