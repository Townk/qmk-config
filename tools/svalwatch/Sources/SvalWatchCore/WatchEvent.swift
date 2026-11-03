/// What svalwatch prints: one JSON object per line on stdout, only when
/// something changed. This is the whole contract with whatever draws the
/// overlay; how the state was obtained (polling today, the board
/// announcing changes later) stays behind it.
///
///     {"type":"connected","serial":"sval:E464B4B2934E2C2A","protocol":3}
///     {"type":"keymap","layers":16,"keydefs":"0xB68B17F4","keycodes":[41,30,…]}
///     {"type":"layer","top":15,"active":32769,"default":1,"mods":0,"caps_word":false,"caps_lock":false}
///     {"type":"keys","pressed":[[0,3],[6,1]]}
///     {"type":"pointer","left":{"dpi":400,"scroll":true},"right":{"dpi":1600,"scroll":false},"scroll_key":false,"sniper":1,"boost":1}
///     {"type":"disconnected"}
///
/// `pressed` holds matrix [row, col] pairs; rows 0-4 are the left half,
/// 5-9 the right. After `connected`, a `layer` and a `keys` line always
/// follow, so a reader never has to guess the starting state. At most once
/// per connect, at any time after `connected` (an announcing board may
/// send its `layer` and `keys` first), comes `keymap`: every layer's
/// keycodes as the board holds them now, [layer][row][col] flattened, 60
/// per layer, with `keydefs`, the fingerprint of the key definitions the
/// firmware was built with (tools/keydefs.py), or null when the firmware
/// cannot say (it predates it, or the read failed). Both are read before
/// the line goes out, so a reader deciding whether its keycode tables fit
/// has everything in the one event. A board whose keymap cannot be read
/// sends none, and the other lines carry on as usual. `pointer`
/// (each trackball's DPI and whether it scrolls right now) comes only from
/// firmware that announces it; a polled board never sends one.
public enum WatchEvent: Equatable, Sendable {
    case connected(serial: String, protocolVersion: Int)
    /// The board's keymap and its key-definitions fingerprint, if known.
    case keymap(Keymap, keydefs: UInt32?)
    case layer(LayerState)
    case keys([KeyPosition])
    case pointer(PointerState)
    case disconnected

    public var jsonLine: String {
        switch self {
        case let .connected(serial, version):
            return #"{"type":"connected","serial":"\#(escape(serial))","protocol":\#(version)}"#
        case let .keymap(k, keydefs):
            let codes = k.keycodes.map(String.init).joined(separator: ",")
            let fp = keydefs.map { "\"\(Keydefs.format($0))\"" } ?? "null"
            return #"{"type":"keymap","layers":\#(k.layers),"keydefs":\#(fp),"keycodes":[\#(codes)]}"#
        case let .layer(s):
            return #"{"type":"layer","top":\#(s.topLayer),"active":\#(s.active),"default":\#(s.default),"#
                + #""mods":\#(s.mods),"caps_word":\#(s.capsWord),"caps_lock":\#(s.capsLock)}"#
        case let .keys(pressed):
            let pairs = pressed.map { "[\($0.row),\($0.col)]" }.joined(separator: ",")
            return #"{"type":"keys","pressed":[\#(pairs)]}"#
        case let .pointer(p):
            return #"{"type":"pointer","left":{"dpi":\#(p.leftDPI),"scroll":\#(p.leftScroll)},"#
                + #""right":{"dpi":\#(p.rightDPI),"scroll":\#(p.rightScroll)},"scroll_key":\#(p.scrollKey),"#
                + #""sniper":\#(p.sniperDiv),"boost":\#(p.boostMult)}"#
        case .disconnected:
            return #"{"type":"disconnected"}"#
        }
    }

    private func escape(_ s: String) -> String {
        var out = ""
        for u in s.unicodeScalars {
            switch u {
            case "\"": out += "\\\""
            case "\\": out += "\\\\"
            case _ where u.value < 0x20:
                let hex = String(u.value, radix: 16)
                out += "\\u" + String(repeating: "0", count: 4 - hex.count) + hex
            default: out.unicodeScalars.append(u)
            }
        }
        return out
    }
}

/// The board's dynamic keymap: `layers * rows * cols` keycodes in
/// [layer][row][col] order, the matrix's rows and columns as in
/// `KeyPosition`.
public struct Keymap: Equatable, Sendable {
    public let layers: Int
    public let rows: Int
    public let cols: Int
    public let keycodes: [UInt16]

    public init(layers: Int, rows: Int, cols: Int, keycodes: [UInt16]) {
        precondition(keycodes.count == layers * rows * cols)
        self.layers = layers
        self.rows = rows
        self.cols = cols
        self.keycodes = keycodes
    }
}

/// Turns poll results into events: the first reading of each kind after a
/// (re)connect always counts as a change, later ones only when different.
public struct ChangeTracker: Sendable {
    private var layer: LayerState?
    private var keys: [KeyPosition]?
    private var pointer: PointerState?

    public init() {}

    public mutating func reset() {
        layer = nil
        keys = nil
        pointer = nil
    }

    public mutating func update(pointer new: PointerState) -> WatchEvent? {
        guard new != pointer else { return nil }
        pointer = new
        return .pointer(new)
    }

    public mutating func update(layer new: LayerState) -> WatchEvent? {
        guard new != layer else { return nil }
        layer = new
        return .layer(new)
    }

    public mutating func update(keys new: [KeyPosition]) -> WatchEvent? {
        let sorted = new.sorted()
        guard sorted != keys else { return nil }
        keys = sorted
        return .keys(sorted)
    }
}

/// Each trackball's speed and mode, as the board reports them.
public struct PointerState: Equatable, Sendable {
    public var leftDPI: Int
    public var rightDPI: Int
    /// Whether each trackball scrolls right now: its own setting, flipped
    /// by a held scroll key or a latched scroll toggle.
    public var leftScroll: Bool
    public var rightScroll: Bool
    /// A scroll hold or toggle key is active (it is what flipped them).
    public var scrollKey: Bool
    /// Sniper keys divide both trackballs' motion by this, boost keys
    /// multiply it; 1 when none is active. The DPIs stay the sensors' own.
    public var sniperDiv: Int
    public var boostMult: Int

    public init(leftDPI: Int, rightDPI: Int, leftScroll: Bool, rightScroll: Bool, scrollKey: Bool,
                sniperDiv: Int = 1, boostMult: Int = 1) {
        self.leftDPI = leftDPI
        self.rightDPI = rightDPI
        self.leftScroll = leftScroll
        self.rightScroll = rightScroll
        self.scrollKey = scrollKey
        self.sniperDiv = sniperDiv
        self.boostMult = boostMult
    }
}

/// The key-definitions fingerprint (tools/keydefs.py) as text: how the
/// render bundle's keydefs.json and the log lines spell it.
public enum Keydefs {
    public static func format(_ fingerprint: UInt32) -> String {
        let hex = String(fingerprint, radix: 16, uppercase: true)
        return "0x" + String(repeating: "0", count: 8 - hex.count) + hex
    }

    public enum Verdict: Equatable, Sendable {
        /// The keyboard's fingerprint is the bundle's: draw.
        case matches(UInt32)
        /// The firmware cannot say (it predates the fingerprint, or the
        /// read failed): draw unchecked, as before fingerprints.
        case unknown
        /// The bundle's tables were built for other key definitions (or
        /// it records none): drawing would name keys wrongly.
        case outOfDate(board: UInt32, bundle: UInt32?)
    }

    /// Whether a render bundle with fingerprint `bundle` may draw a keymap
    /// from a keyboard reporting `board`.
    public static func verdict(board: UInt32?, bundle: UInt32?) -> Verdict {
        guard let board else { return .unknown }
        return bundle == board ? .matches(board) : .outOfDate(board: board, bundle: bundle)
    }

    /// keydefs.json's `fingerprint`, or nil when it is not one.
    public static func parse(_ text: String) -> UInt32? {
        guard text.hasPrefix("0x") || text.hasPrefix("0X") else { return nil }
        return UInt32(text.dropFirst(2), radix: 16)
    }
}
