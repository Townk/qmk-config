import CryptoKit
import Foundation

/// How svaloverlay names and trims the pages it draws from a board's own
/// keymap. A page depends on three things only, the render bundle it was
/// drawn with, the skim and Python that drew it (`renderPin`) and the
/// keymap, so together they name it: the same board on the same overlay
/// build finds the page it drew last time and draws nothing.
public enum PageCache {
    /// The fixed part of the render command: which Python and which skim
    /// draw the page. Part of the cache key, so bumping either one here
    /// redraws pages drawn by the old ones.
    public static let renderPin = ["--python", "3.13", "--with", "qmk-skim==0.7.5"]

    /// The page's directory name: the SHA-256, in lowercase hex, of the
    /// bundle's VERSION, the render pin and the keymap (its shape, then
    /// every keycode big-endian, as the board stores them).
    public static func key(bundleVersion: String, keymap: Keymap,
                           renderPin pin: [String] = renderPin) -> String {
        var bytes = Array(bundleVersion.utf8)
        bytes.append(0)  // the version ends here, whatever its length
        for arg in pin {
            bytes += Array(arg.utf8)
            bytes.append(0)
        }
        bytes.append(0)  // and the pin here
        for n in [keymap.layers, keymap.rows, keymap.cols] {
            bytes += [UInt8(truncatingIfNeeded: n >> 8), UInt8(truncatingIfNeeded: n)]
        }
        for kc in keymap.keycodes {
            bytes += [UInt8(kc >> 8), UInt8(kc & 0xFF)]
        }
        return SHA256.hash(data: bytes).map { String(format: "%02x", $0) }.joined()
    }

    /// The entries to delete so that only the `keep` most recently used
    /// pages remain, newest by modification date, plus every `<key>.tmp`
    /// left by a render that died, except the one for `rendering`.
    public static func prune(dirs: [(name: String, modified: Date)], keep: Int,
                             rendering: String? = nil) -> [String] {
        let busy = rendering.map { $0 + ".tmp" }
        let stale = dirs.filter { $0.name.hasSuffix(".tmp") && $0.name != busy }.map(\.name)
        let pages = dirs.filter { !$0.name.hasSuffix(".tmp") }
        return stale + pages.sorted { $0.modified > $1.modified }.dropFirst(max(keep, 0)).map(\.name)
    }

    /// `uv`'s arguments for drawing a page (run with the bundle as the
    /// working directory). skim needs Python 3.10 or newer, and without
    /// `--python` uv takes the first python on PATH. `-B`: the bundle ships
    /// in a chezmoi external, so importing page_build from it must not
    /// write __pycache__ there.
    public static func renderArguments(bundle: String, keymap: String, out: String) -> [String] {
        ["run", "--no-project", "--quiet"] + renderPin + [
            "python", "-I", "-B", bundle + "/render_page.py",
            "--bundle", bundle, "--keymap", keymap, "--out", out,
        ]
    }
}
