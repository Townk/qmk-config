/// The few Raw HID exchanges svalwatch needs, all read-only.
///
/// Every request goes through the svalboard/core client wrapper
/// (modules/svalboard/core/client_wrapper.c): `DD <client:u32 LE>
/// <protocol> <payload>`. Protocol `DF` is Sval, `FE` is a VIA command,
/// and the board answers `FF <code>` when it rejects the wrapper. A client
/// ID comes from a bootstrap exchange and expires on the board's clock, so
/// the session asks for a new one well before that (see `Session`).
///
/// The wrapper ID is also what keeps this tool and Keybard apart when
/// both have the board open: macOS hands every input report to every
/// reader, and a reply carrying someone else's ID is not ours.
public enum SvalProtocol {
    public static let reportSize = 64
    public static let usagePage = 0xFF61
    public static let usage = 0x62
    public static let serialPrefix = "sval:"

    public static let matrixRows = 10  // five per half
    public static let matrixCols = 6

    static let wrapperPrefix: UInt8 = 0xDD
    static let svalProtocol: UInt8 = 0xDF
    static let viaProtocol: UInt8 = 0xFE
    static let errorProtocol: UInt8 = 0xFF

    static let svalGetInfo: UInt8 = 0x00
    static let svalLayerState: UInt8 = 0x16
    static let viaGetKeyboardValue: UInt8 = 0x02
    static let viaSwitchMatrixState: UInt8 = 0x03

    public static let nonceSize = 20

    /// townk_announce.c's subscribe command, a VIA custom-value GET on its
    /// own channel. Stock firmware answers it without the signature.
    static let viaCustomGetValue: UInt8 = 0x08
    static let viaUnhandled: UInt8 = 0xFF
    static let announceChannel: UInt8 = 0x57
    static let announceSubscribe: UInt8 = 0xF7
    static let announceSignature: [UInt8] = Array("svw".utf8)

    /// QMK's debug console, where the announcements arrive.
    public static let consoleUsagePage = 0xFF31
    public static let consoleUsage = 0x74

    public static func bootstrapRequest(nonce: [UInt8]) -> [UInt8] {
        precondition(nonce.count == nonceSize)
        return pad([wrapperPrefix, 0, 0, 0, 0] + nonce)
    }

    public static func infoRequest(client: UInt32) -> [UInt8] {
        wrapped(client, [svalProtocol, svalGetInfo])
    }

    public static func layerStateRequest(client: UInt32) -> [UInt8] {
        wrapped(client, [svalProtocol, svalLayerState])
    }

    /// VIA's switch matrix read, row offset 0. It answers only because the
    /// Svalboard keyboard builds with VIA_INSECURE.
    public static func matrixRequest(client: UInt32) -> [UInt8] {
        wrapped(client, [viaProtocol, viaGetKeyboardValue, viaSwitchMatrixState, 0])
    }

    public static func subscribeRequest(client: UInt32, leaseMs: UInt16) -> [UInt8] {
        wrapped(client, [viaProtocol, viaCustomGetValue, announceChannel, announceSubscribe,
                         UInt8(leaseMs & 0xFF), UInt8(leaseMs >> 8)])
    }

    /// Classifies one input report. Returns nil for anything that is not
    /// one of the four replies this tool asks for, including other
    /// clients' traffic; the caller still has to check the client ID.
    public static func parse(_ r: [UInt8]) -> Reply? {
        guard r.count >= 31, r[0] == wrapperPrefix else { return nil }
        let client = u32(r, 1)
        if client == 0 {
            return .bootstrap(nonce: Array(r[5..<25]), client: u32(r, 25))
        }
        switch r[5] {
        case errorProtocol:
            return .error(client: client, code: r[6])
        case svalProtocol where r[6] == svalGetInfo:
            // Inner Sval offsets (after DD + ID): 2 version, 14 the first
            // feature byte, 19 the second (LAYER_STATE_PROTOCOL.md).
            return .info(client: client, InfoFlags(
                version: r[7],
                reportsDefaultLayer: r[5 + 14] & 0x40 != 0,
                reportsModifiers: r[5 + 19] & 0x04 != 0))
        case svalProtocol where r[6] == svalLayerState:
            return .layerState(client: client, LayerState(
                active: u32(r, 7), default: u32(r, 11), mods: r[15], status: r[16]))
        case viaProtocol where r[6] == viaGetKeyboardValue && r[7] == viaSwitchMatrixState:
            // VIA's reply is shifted six bytes: echo at 6-7, offset at 8,
            // one byte per row from 9.
            var pressed: [KeyPosition] = []
            for row in 0..<matrixRows {
                let bits = r[9 + row]
                if bits >> matrixCols != 0 { return .malformed(client: client) }
                for col in 0..<matrixCols where bits & (1 << col) != 0 {
                    pressed.append(KeyPosition(row: row, col: col))
                }
            }
            return .matrix(client: client, pressed)
        case viaProtocol where r[7] == announceChannel && r[8] == announceSubscribe
            && (r[6] == viaCustomGetValue || r[6] == viaUnhandled):
            // VIA reply at 6: command, channel, value id, then ours.
            guard r[6] == viaCustomGetValue, Array(r[9..<12]) == announceSignature else {
                return .subscription(client: client, granted: nil)
            }
            return .subscription(client: client, granted: Announcement(
                version: r[12], leaseMs: UInt16(r[13]) | UInt16(r[14]) << 8))
        default:
            return nil
        }
    }

    private static func wrapped(_ client: UInt32, _ payload: [UInt8]) -> [UInt8] {
        pad([wrapperPrefix] + le32(client) + payload)
    }

    private static func pad(_ bytes: [UInt8]) -> [UInt8] {
        bytes + [UInt8](repeating: 0, count: reportSize - bytes.count)
    }

    static func le32(_ v: UInt32) -> [UInt8] {
        [UInt8(v & 0xFF), UInt8(v >> 8 & 0xFF), UInt8(v >> 16 & 0xFF), UInt8(v >> 24)]
    }

    private static func u32(_ r: [UInt8], _ at: Int) -> UInt32 {
        UInt32(r[at]) | UInt32(r[at + 1]) << 8 | UInt32(r[at + 2]) << 16 | UInt32(r[at + 3]) << 24
    }
}

public enum Reply: Equatable, Sendable {
    case bootstrap(nonce: [UInt8], client: UInt32)
    case info(client: UInt32, InfoFlags)
    case layerState(client: UInt32, LayerState)
    case matrix(client: UInt32, [KeyPosition])
    case error(client: UInt32, code: UInt8)
    /// The subscribe reply; `granted` is nil when the firmware has no
    /// announcements (stock firmware, or a userspace without the module).
    case subscription(client: UInt32, granted: Announcement?)
    /// One of ours, but with column bits past the matrix: not trusted.
    case malformed(client: UInt32)
}

public struct Announcement: Equatable, Sendable {
    public var version: UInt8
    public var leaseMs: UInt16
}

public struct InfoFlags: Equatable, Sendable {
    public var version: UInt8
    public var reportsDefaultLayer: Bool
    public var reportsModifiers: Bool
}

public struct KeyPosition: Hashable, Comparable, Sendable {
    public var row: Int
    public var col: Int

    public init(row: Int, col: Int) {
        self.row = row
        self.col = col
    }

    public static func < (a: Self, b: Self) -> Bool {
        (a.row, a.col) < (b.row, b.col)
    }
}

public struct LayerState: Equatable, Sendable {
    public var active: UInt32
    public var `default`: UInt32
    public var mods: UInt8
    public var status: UInt8

    public init(active: UInt32, default: UInt32, mods: UInt8 = 0, status: UInt8 = 0) {
        self.active = active
        self.default = `default`
        self.mods = mods
        self.status = status
    }

    /// The layer whose legends are showing: the highest bit of active and
    /// default together. Both masks may be zero; QMK then falls back to
    /// layer 0, and so does this.
    public var topLayer: Int {
        let all = active | `default`
        return all == 0 ? 0 : 31 - all.leadingZeroBitCount
    }

    public var capsWord: Bool { status & 0x01 != 0 }
    public var capsLock: Bool { status & 0x02 != 0 }

    /// Drops what the firmware did not advertise, so a reply from older
    /// firmware with leftover bytes cannot fake a change.
    public func masked(by flags: InfoFlags) -> LayerState {
        LayerState(
            active: active,
            default: flags.reportsDefaultLayer ? `default` : 0,
            mods: flags.reportsModifiers ? mods : 0,
            status: flags.reportsModifiers ? status : 0)
    }
}
