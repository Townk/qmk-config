import AppKit
import SvalWatchCore
import SvalWatchDevice

let usage = """
    usage: svaloverlay [--assets DIR] [--width W] [--margin POINTS]
                       [--on-top] [--serial S] [--poll] [--stats SECONDS]

    Shows the Svalboard's current layer in the bottom-left corner of the
    main display, highlighting the keys being held. It sits below every
    window until told to stay on top. Meant to be started by Hammerspoon,
    which controls it through stdin, one command per line:

      top       stay above every window
      bottom    sit below every window (the start state)
      toggle    switch between the two
      quit      exit (as does closing stdin)

      --assets DIR      the page from `make overlay-assets`
                        (default ~/.local/share/svaloverlay)
      --width W         panel width: points, or a share of the main
                        display's width like 25% (the default)
      --margin POINTS   gap to the screen's edges (default 50)
      --on-top          start above every window
      --serial S        watch this board only
      --poll            poll even if the firmware can announce changes
      --stats SECONDS   log CPU use to stderr every SECONDS
    """

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("svaloverlay: \(message)\n\n\(usage)\n".utf8))
    exit(2)
}

var overlayOptions = OverlayWindow.Options(
    assets: FileManager.default.homeDirectoryForCurrentUser
        .appendingPathComponent(".local/share/svaloverlay"))
var watcherOptions = Watcher.Options()
var startOnTop = false

var args = CommandLine.arguments.dropFirst()
@MainActor func value(_ flag: String) -> String {
    guard let v = args.popFirst() else { fail("\(flag) needs a value") }
    return v
}
@MainActor func positive(_ flag: String) -> Double {
    guard let v = Double(value(flag)), v > 0, v.isFinite else { fail("\(flag) needs a positive number") }
    return v
}
while let arg = args.popFirst() {
    switch arg {
    case "--assets": overlayOptions.assets = URL(fileURLWithPath: (value(arg) as NSString).expandingTildeInPath)
    case "--width":
        let w = value(arg)
        if w.hasSuffix("%"), let pct = Double(w.dropLast()), pct > 0, pct <= 100 {
            overlayOptions.width = .screenFraction(pct / 100)
        } else if let points = Double(w), points > 0 {
            overlayOptions.width = .points(points)
        } else {
            fail("--width needs points or a percentage, like 25%")
        }
    case "--margin": overlayOptions.margin = positive(arg)
    case "--on-top": startOnTop = true
    case "--serial": watcherOptions.serial = value(arg)
    case "--poll": watcherOptions.session.useAnnouncements = false
    case "--stats": watcherOptions.statsEvery = positive(arg)
    case "-h", "--help":
        print(usage)
        exit(0)
    default: fail("unknown option \(arg)")
    }
}

let index = overlayOptions.assets.appendingPathComponent("index.html")
guard FileManager.default.fileExists(atPath: index.path) else {
    fail("no overlay page at \(index.path); run `make overlay-assets` first")
}

let stderr = FileHandle.standardError
func log(_ line: String) { stderr.write(Data((line + "\n").utf8)) }

let app = NSApplication.shared
app.setActivationPolicy(.accessory)  // no Dock icon, no menu bar, never key

let overlay = OverlayWindow(options: overlayOptions)
overlay.load()
if startOnTop { overlay.setOnTop(true) }

let watcher = Watcher(options: watcherOptions, output: { overlay.show($0) }, log: log)
watcher.start()
let activity = SystemActivity(watcher: watcher)

let commands = StdinWatch(onLine: { command in
    switch command {
    case "top": overlay.setOnTop(true)
    case "bottom": overlay.setOnTop(false)
    case "toggle": overlay.setOnTop(!overlay.onTop)
    case "quit": exit(0)
    case "": break
    default: log("svaloverlay: unknown command \(command)")
    }
})

app.run()
