/// Reassembles townk_announce.c's lines from QMK console reports.
///
/// The console is a byte stream cut into fixed-size, zero-padded reports,
/// so a line can span reports and a report can end mid-line. Anything
/// else the firmware prints there is skipped, as is any line that does not
/// parse exactly: the next lease renewal re-announces the full state, so a
/// dropped report costs at most one renewal period.
///
///     svw L <active:8 hex> <default:8 hex> <mods:2 hex> <status:2 hex>
///     svw K <one 2-hex byte per matrix row, no spaces>
///     svw P <left DPI:4 hex> <right DPI:4 hex> <pointer flags:2 hex>
public struct ConsoleParser: Sendable {
    public enum Line: Equatable, Sendable {
        case layer(LayerState)
        case keys([KeyPosition])
        case pointer(PointerState)
    }

    private var pending: [UInt8] = []
    private static let maxLine = 80

    public init() {}

    public mutating func reset() {
        pending.removeAll()
    }

    public mutating func feed(_ report: [UInt8]) -> [Line] {
        var lines: [Line] = []
        for byte in report where byte != 0 {
            if byte == UInt8(ascii: "\n") {
                if let line = Self.parse(pending) { lines.append(line) }
                pending.removeAll(keepingCapacity: true)
            } else if pending.count < Self.maxLine {
                pending.append(byte)
            } else {
                // Not one of ours; drop it but keep dropping until its end.
                pending = Array(repeating: 0, count: Self.maxLine)
            }
        }
        return lines
    }

    static func parse(_ bytes: [UInt8]) -> Line? {
        let text = String(decoding: bytes, as: UTF8.self)
        let fields = text.split(separator: " ", omittingEmptySubsequences: false)
        guard fields.count >= 3, fields[0] == "svw" else { return nil }
        switch fields[1] {
        case "L":
            guard fields.count == 6,
                  fields[2].count == 8, let active = UInt32(fields[2], radix: 16),
                  fields[3].count == 8, let def = UInt32(fields[3], radix: 16),
                  fields[4].count == 2, let mods = UInt8(fields[4], radix: 16),
                  fields[5].count == 2, let status = UInt8(fields[5], radix: 16)
            else { return nil }
            return .layer(LayerState(active: active, default: def, mods: mods, status: status))
        case "K":
            let hex = Array(fields[2].utf8)
            guard fields.count == 3, hex.count == 2 * SvalProtocol.matrixRows else { return nil }
            var pressed: [KeyPosition] = []
            for row in 0..<SvalProtocol.matrixRows {
                guard let bits = UInt8(String(decoding: hex[2 * row ..< 2 * row + 2], as: UTF8.self), radix: 16),
                      bits >> SvalProtocol.matrixCols == 0
                else { return nil }
                for col in 0..<SvalProtocol.matrixCols where bits & (1 << col) != 0 {
                    pressed.append(KeyPosition(row: row, col: col))
                }
            }
            return .keys(pressed)
        case "P":
            guard fields.count == 5,
                  fields[2].count == 4, let left = UInt16(fields[2], radix: 16),
                  fields[3].count == 4, let right = UInt16(fields[3], radix: 16),
                  fields[4].count == 2, let flags = UInt8(fields[4], radix: 16)
            else { return nil }
            // Flags, from townk_announce.h's ANNOUNCE_POINTER_*.
            return .pointer(PointerState(
                leftDPI: Int(left), rightDPI: Int(right),
                leftScroll: flags & 0x01 != 0, rightScroll: flags & 0x02 != 0,
                scrollKey: flags & 0x04 != 0))
        default:
            return nil
        }
    }
}
