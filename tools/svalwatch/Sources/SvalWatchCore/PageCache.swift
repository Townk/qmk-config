import CryptoKit
import Foundation

/// How svaloverlay names and trims the pages it draws from a board's own
/// keymap. A page depends on two things only, the render bundle it was
/// drawn with and the keymap, so the pair names it: the same board on the
/// same overlay build finds the page it drew last time and draws nothing.
public enum PageCache {
    /// The page's directory name: the SHA-256, in lowercase hex, of the
    /// bundle's VERSION and the keymap (its shape, then every keycode
    /// big-endian, as the board stores them).
    public static func key(bundleVersion: String, keymap: Keymap) -> String {
        var bytes = Array(bundleVersion.utf8)
        bytes.append(0)  // the version ends here, whatever its length
        for n in [keymap.layers, keymap.rows, keymap.cols] {
            bytes += [UInt8(truncatingIfNeeded: n >> 8), UInt8(truncatingIfNeeded: n)]
        }
        for kc in keymap.keycodes {
            bytes += [UInt8(kc >> 8), UInt8(kc & 0xFF)]
        }
        return SHA256.hash(data: bytes).map { String(format: "%02x", $0) }.joined()
    }

    /// The pages to delete so that only the `keep` most recently used ones
    /// remain, newest by modification date.
    public static func prune(dirs: [(name: String, modified: Date)], keep: Int) -> [String] {
        dirs.sorted { $0.modified > $1.modified }.dropFirst(max(keep, 0)).map(\.name)
    }
}
