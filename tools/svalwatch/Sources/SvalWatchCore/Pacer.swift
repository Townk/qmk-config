/// How long to wait before the next poll.
///
/// The board only answers questions, so the cost of watching it is the
/// polling rate. Polls run fast while something is changing (or the Mac
/// just saw input from the board) and for `linger` seconds after, then
/// drop to the idle rate. The idle rate is what catches a change that
/// sends nothing to the Mac on its own, such as holding a layer key.
public struct Pacer: Sendable {
    public enum Mode: Equatable, Sendable { case active, idle }

    public var activeInterval: Double
    public var idleInterval: Double
    public var linger: Double

    private var lastActivity = -Double.infinity

    public init(activeHz: Double, idleHz: Double, linger: Double) {
        precondition(activeHz > 0 && idleHz > 0 && linger >= 0)
        activeInterval = 1 / activeHz
        idleInterval = 1 / idleHz
        self.linger = linger
    }

    public mutating func noteActivity(at now: Double) {
        lastActivity = max(lastActivity, now)
    }

    public func mode(at now: Double) -> Mode {
        now - lastActivity < linger ? .active : .idle
    }

    public func interval(at now: Double) -> Double {
        mode(at: now) == .active ? activeInterval : idleInterval
    }
}
