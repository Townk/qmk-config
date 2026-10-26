import AppKit
import SvalWatchCore
import WebKit

/// The overlay: a borderless panel in the bottom-left corner of the main
/// display, showing the page built by `make overlay-assets` (skim's layer
/// images plus the which-key panel styling) in a transparent web view.
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
    private var ready = false
    private var pending: [String] = []
    private(set) var onTop = false

    init(options: Options) {
        self.options = options
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

    func load() {
        let index = options.assets.appendingPathComponent("index.html")
        web.loadFileURL(index, allowingReadAccessTo: options.assets)
    }

    // MARK: State from the board

    func show(_ event: WatchEvent) {
        switch event {
        case .connected:
            call("overlay.connected(true)")
        case .disconnected:
            call("overlay.connected(false)")
        case let .layer(state):
            call("overlay.layer(\(state.active), \(state.default))")
        case let .keys(pressed):
            let pairs = pressed.map { "[\($0.row),\($0.col)]" }.joined(separator: ",")
            call("overlay.keys([\(pairs)])")
        }
    }

    // MARK: Z-order

    func setOnTop(_ on: Bool) {
        onTop = on
        panel.level = on ? Self.topLevel : Self.bottomLevel
        panel.orderFrontRegardless()
        call("overlay.onTop(\(on))")
    }

    // MARK: Page bridge

    func userContentController(_ controller: WKUserContentController, didReceive message: WKScriptMessage) {
        guard let body = message.body as? [String: Any], let kind = body["type"] as? String else { return }
        switch kind {
        case "ready":
            ready = true
            configure()
            let queued = pending
            pending.removeAll()
            for script in queued { web.evaluateJavaScript(script) }
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

    private func call(_ script: String) {
        if ready {
            web.evaluateJavaScript(script)
        } else {
            pending.append(script)
        }
    }

    // MARK: Placement

    private func resize(width: Double, height: Double) {
        panel.setContentSize(NSSize(width: width, height: height))
        place()
        if !panel.isVisible { panel.orderFrontRegardless() }
    }

    /// Bottom-left of the primary display (the one with the menu bar).
    private func place() {
        guard let screen = NSScreen.screens.first else { return }
        let visible = screen.visibleFrame
        panel.setFrameOrigin(NSPoint(x: visible.minX + options.margin, y: visible.minY + options.margin))
    }
}
