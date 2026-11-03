import Foundation
import Testing
@testable import SvalWatchCore

func keymap(layers: Int = 2, changing index: Int? = nil) -> Keymap {
    var codes = (0..<layers * 60).map { UInt16($0 + 0x100) }
    if let index { codes[index] &+= 1 }
    return Keymap(layers: layers, rows: 10, cols: 6, keycodes: codes)
}

@Suite struct PageCacheTests {
    @Test func keyChangesWithTheKeymap() {
        let base = PageCache.key(bundleVersion: "v1", keymap: keymap())
        #expect(PageCache.key(bundleVersion: "v1", keymap: keymap(changing: 0)) != base)
        #expect(PageCache.key(bundleVersion: "v1", keymap: keymap(changing: 119)) != base)
        #expect(PageCache.key(bundleVersion: "v1", keymap: keymap(layers: 3)) != base)
    }

    @Test func keyChangesWithTheBundleVersion() {
        #expect(PageCache.key(bundleVersion: "v1", keymap: keymap())
            != PageCache.key(bundleVersion: "v2", keymap: keymap()))
    }

    @Test func keyIsStableForEqualInput() {
        let key = PageCache.key(bundleVersion: "v1", keymap: keymap())
        #expect(key == PageCache.key(bundleVersion: "v1", keymap: keymap()))
        // A SHA-256, in lowercase hex: a directory name on any file system.
        #expect(key.count == 64)
        #expect(key.allSatisfy { "0123456789abcdef".contains($0) })
    }

    @Test func keydefsVerdict() {
        #expect(Keydefs.verdict(board: 0xB68B_17F4, bundle: 0xB68B_17F4) == .matches(0xB68B_17F4))
        #expect(Keydefs.verdict(board: 0xB68B_17F4, bundle: 0x1234_5678)
            == .outOfDate(board: 0xB68B_17F4, bundle: 0x1234_5678))
        // A bundle from before fingerprints cannot vouch for its tables.
        #expect(Keydefs.verdict(board: 0xB68B_17F4, bundle: nil) == .outOfDate(board: 0xB68B_17F4, bundle: nil))
        // Firmware from before them cannot say: drawn unchecked.
        #expect(Keydefs.verdict(board: nil, bundle: 0x1234_5678) == .unknown)
        #expect(Keydefs.verdict(board: nil, bundle: nil) == .unknown)
    }

    @Test func keydefsText() {
        #expect(Keydefs.format(0x0B68_17F4) == "0x0B6817F4")
        #expect(Keydefs.parse("0xB68B17F4") == 0xB68B_17F4)
        #expect(Keydefs.parse(Keydefs.format(7)) == 7)
        #expect(Keydefs.parse("B68B17F4") == nil)
        #expect(Keydefs.parse("0x1FFFFFFFF") == nil)
        #expect(Keydefs.parse("0xZZ") == nil)
    }

    @Test func pruneKeepsTheNewestFive() {
        let now = Date()
        let dirs = (0..<8).map { i in
            (name: "page\(i)", modified: now.addingTimeInterval(Double([3, 7, 1, 5, 0, 6, 2, 4][i])))
        }
        // Ages 0, 1, 2 are the oldest: page4, page2, page6.
        #expect(Set(PageCache.prune(dirs: dirs, keep: 5)) == ["page4", "page2", "page6"])
        #expect(PageCache.prune(dirs: Array(dirs.prefix(5)), keep: 5).isEmpty)
        #expect(PageCache.prune(dirs: [], keep: 5).isEmpty)
    }

    @Test func pruneSkipsTheRenderInProgress() {
        let now = Date()
        let dirs = [(name: "old", modified: now), (name: "busy.tmp", modified: now.addingTimeInterval(-9)),
                    (name: "stale.tmp", modified: now.addingTimeInterval(-9))]
        #expect(Set(PageCache.prune(dirs: dirs, keep: 5, rendering: "busy")) == ["stale.tmp"])
    }

    @Test func theRenderCommandWritesNoBytecode() {
        let args = PageCache.renderArguments(bundle: "/b", keymap: "/k.json", out: "/p/x.tmp")
        #expect(args == [
            "run", "--no-project", "--quiet", "--python", "3.13", "--with", "qmk-skim==0.7.5",
            "python", "-I", "-B", "/b/render_page.py", "--bundle", "/b", "--keymap", "/k.json", "--out", "/p/x.tmp",
        ])
    }
}
