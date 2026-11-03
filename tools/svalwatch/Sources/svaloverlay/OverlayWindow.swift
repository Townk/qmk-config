import AppKit
import SvalWatchCore
import WebKit

/// The overlay: a borderless panel in the bottom-left corner of the main
/// display, showing the page built by `make overlay-assets` (skim's layer
/// images plus the which-key panel styling) in a transparent web view.
/// Once a board reports its keymap, the panel swaps to a page drawn from
/// that keymap (`PageRenderer`), so it shows the keys as flashed, not as
/// they were when the overlay was built. Only when the keyboard's
/// key-definitions fingerprint agrees with the render bundle's, though: a
/// keyboard whose custom keycodes or layers were renumbered since the
/// bundle was built would get keys spelled as other keys, so it keeps the
/// page it has, with "out of date" in the header.
///
/// It shows only while a board is connected: hidden until the first one
/// connects, hidden again when it is unplugged, so a Mac without a
/// Svalboard never sees it.
///
/// It lives at one of two window levels, never in between: the desktop
/// icons' level (every window covers it, like a widget) or the highest
/// level below the cursor (it covers everything). It never takes clicks,
/// focus or a Dock icon, so in either place the windows around and under
/// it behave as if it were not there.
@MainActor
final class OverlayWindow: NSObject, WKScriptMessageHandler {
    struct Options {
        var assets: URL
        /// Width of the whole panel: a fraction of the main display's
        /// width, or a fixed number of points.
        var width: Width = .screenFraction(0.25)
        /// Gap to the screen's visible edges (Dock, menu bar excluded).
        var margin: Double = 50
    }

    enum Width {
        case screenFraction(Double)
        case points(Double)
    }

    static let bottomLevel = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.desktopIconWindow)))
    static let topLevel = NSWindow.Level(rawValue: Int(CGWindowLevelForKey(.assistiveTechHighWindow)))

    private let options: Options
    private let panel: NSPanel
    private let web: WKWebView
    let renderer: PageRenderer
    private let log: (String) -> Void
    /// The page in the web view has said "ready", so it takes calls.
    private var ready = false
    /// The page has reported its size, so the panel has its real frame.
    private var sized = false
    private var boardConnected = false
    /// The page directory loaded (or loading) in the web view.
    private var page: URL
    /// The page the latest keymap wants; a render finished for any other
    /// keymap is stale and not shown.
    private var wantedPage: String?
    /// The last call of each kind, which is the whole state a page shows:
    /// replayed in this order whenever a page becomes ready, so a page
    /// loaded mid-session starts where the old one was.
    private var state: [Kind: String] = [:]
    private enum Kind: CaseIterable { case onTop, connected, layer, keys, pointer, stale }
    private(set) var onTop = false

    init(options: Options, log: @escaping (String) -> Void) {
        self.options = options
        self.log = log
        page = options.assets
        renderer = PageRenderer(bundle: options.assets.appendingPathComponent("render", isDirectory: true), log: log)
        panel = NSPanel(contentRect: NSRect(x: 0, y: 0, width: 600, height: 200),
                        styleMask: [.borderless, .nonactivatingPanel], backing: .buffered, defer: false)
        let config = WKWebViewConfiguration()
        web = WKWebView(frame: panel.contentLayoutRect, configuration: config)
        super.init()

        config.userContentController.add(self, name: "overlay")
        web.setValue(false, forKey: "drawsBackground")
        web.autoresizingMask = [.width, .height]

        panel.isOpaque = false
        panel.backgroundColor = .clear
        panel.hasShadow = false
        panel.ignoresMouseEvents = true
        panel.isReleasedWhenClosed = false
        panel.hidesOnDeactivate = false
        panel.collectionBehavior = [.canJoinAllSpaces, .stationary, .ignoresCycle, .fullScreenAuxiliary]
        panel.level = Self.bottomLevel
        panel.contentView = web

        NotificationCenter.default.addObserver(
            forName: NSApplication.didChangeScreenParametersNotification, object: nil, queue: .main
        ) { [weak self] _ in
            MainActor.assumeIsolated {
                guard let self else { return }
                if self.ready { self.configure() }
                self.place()
            }
        }
    }

    /// Loads the default page, the one shipped in the assets directory.
    func load() {
        load(page: options.assets)
    }

    /// Swaps the web view to the page in `dir`. Until it says "ready" the
    /// old one stays up and calls are only remembered; then the new page
    /// gets the current state at once.
    func load(page dir: URL) {
        page = dir
        ready = false
        web.loadFileURL(dir.appendingPathComponent("index.html"), allowingReadAccessTo: dir)
    }

    // MARK: State from the board

    func show(_ event: WatchEvent) {
        switch event {
        case .connected:
            boardConnected = true
            call(.connected, "overlay.connected(true)")
            showIfReady()
        case .disconnected:
            boardConnected = false
            wantedPage = nil  // a render still running is for this board only
            call(.connected, "overlay.connected(false)")
            setStale(false)  // the next board's keymap decides again
            panel.orderOut(nil)
        case let .layer(state):
            call(.layer, "overlay.layer(\(state.active), \(state.default))")
        case let .keys(pressed):
            let pairs = pressed.map { "[\($0.row),\($0.col)]" }.joined(separator: ",")
            call(.keys, "overlay.keys([\(pairs)])")
        case .pointer:
            // The page reads the event as svalwatch prints it.
            call(.pointer, "overlay.pointer(\(event.jsonLine))")
        case let .keymap(keymap, keydefs):
            show(keymap, keydefs: keydefs)
        }
    }

    /// The page for this keymap: the cached one if this overlay build drew
    /// it before, otherwise drawn now and shown when done. Without a render
    /// bundle (an older install, or a rebuild swapping it) the current page
    /// stays. So it does when the keyboard's key-definitions fingerprint is
    /// not the bundle's, decided here before anything is looked up or
    /// queued; a keyboard that cannot say (firmware from before the
    /// fingerprint) is drawn as before, unchecked.
    private func show(_ keymap: Keymap, keydefs board: UInt32?) {
        guard let version = renderer.bundleVersion() else {
            wantedPage = nil  // nor show a render started for an older keymap
            log("svaloverlay: no render bundle at \(renderer.bundle.path); keeping the current page")
            return
        }
        switch Keydefs.verdict(board: board, bundle: renderer.bundleKeydefs()) {
        case let .outOfDate(board, bundled):
            wantedPage = nil
            renderer.dropQueued()
            log("svaloverlay: overlay out of date for this firmware: the keyboard's key definitions are "
                + "\(Keydefs.format(board)), the render bundle's \(bundled.map(Keydefs.format) ?? "unknown"); "
                + "keeping the current page (rebuild the overlay: `make` in the userspace)")
            setStale(true)
            return
        case let .matches(board):
            log("svaloverlay: key definitions \(Keydefs.format(board)) match the render bundle")
        case .unknown:
            log("svaloverlay: keyboard fingerprint unavailable (firmware predates it); drawing unchecked")
        }
        setStale(false)
        let key = PageCache.key(bundleVersion: version, keymap: keymap)
        wantedPage = key
        if let cached = renderer.cachedPage(key) {
            renderer.dropQueued()
            if cached != page { load(page: cached) }
            return
        }
        renderer.render(keymap, key: key) { [weak self] key, rendered in
            guard let self, let rendered, key == self.wantedPage else { return }
            self.load(page: rendered)
        }
    }

    /// The header's "out of date" note. Optional-called: a page cached by
    /// an older overlay build has no such function.
    private func setStale(_ on: Bool) {
        call(.stale, "overlay.stale?.(\(on))")
    }

    // MARK: Z-order

    func setOnTop(_ on: Bool) {
        onTop = on
        panel.level = on ? Self.topLevel : Self.bottomLevel
        if panel.isVisible { panel.orderFrontRegardless() }
        call(.onTop, "overlay.onTop(\(on))")
    }

    // MARK: Page bridge

    func userContentController(_ controller: WKUserContentController, didReceive message: WKScriptMessage) {
        guard let body = message.body as? [String: Any], let kind = body["type"] as? String else { return }
        switch kind {
        case "ready":
            ready = true
            configure()
            for kind in Kind.allCases {
                if let script = state[kind] { web.evaluateJavaScript(script) }
            }
        case "size":
            if let w = body["w"] as? Double, let h = body["h"] as? Double {
                resize(width: w, height: h)
            }
        default:
            break
        }
    }

    /// Tells the page how wide the panel is; it fits the drawing inside
    /// and reports the resulting size back.
    private func configure() {
        let width: Double
        switch options.width {
        case let .points(points):
            width = points
        case let .screenFraction(fraction):
            width = (NSScreen.screens.first?.visibleFrame.width ?? 1600) * fraction
        }
        web.evaluateJavaScript("overlay.configure({panelWidth: \(width.rounded())})")
    }

    private func call(_ kind: Kind, _ script: String) {
        state[kind] = script
        if ready { web.evaluateJavaScript(script) }
    }

    // MARK: Placement

    private func resize(width: Double, height: Double) {
        panel.setContentSize(NSSize(width: width, height: height))
        place()
        sized = true
        showIfReady()
    }

    private func showIfReady() {
        if boardConnected && sized && !panel.isVisible { panel.orderFrontRegardless() }
    }

    /// Bottom-left of the primary display (the one with the menu bar).
    private func place() {
        guard let screen = NSScreen.screens.first else { return }
        let visible = screen.visibleFrame
        panel.setFrameOrigin(NSPoint(x: visible.minX + options.margin, y: visible.minY + options.margin))
    }
}
