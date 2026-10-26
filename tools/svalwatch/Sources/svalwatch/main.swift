import AppKit
import SvalWatchCore

let usage = """
    usage: svalwatch [--serial S] [--active-hz N] [--idle-hz N] [--linger SECONDS]
                     [--no-passive-wake] [--poll] [--stats SECONDS]

    Prints the Svalboard's layer and pressed keys as JSON lines on stdout,
    one line per change. Exits when stdin closes, so a parent that dies
    takes svalwatch with it. Diagnostics go to stderr.

    Firmware with users/townk/townk_announce.c announces changes itself
    and svalwatch only renews its subscription; other firmware is polled.
    The polling options below apply only then.

      --serial S         watch this board only (default: the first "sval:" board)
      --active-hz N      polls per second while the board is in use (default 60)
      --idle-hz N        polls per second otherwise (default 4)
      --linger SECONDS   stay at the active rate this long after a change (default 2)
      --no-passive-wake  do not listen to the board's key/mouse reports
                         (avoids the Input Monitoring permission)
      --poll             poll even if the firmware can announce changes
      --stats SECONDS    log CPU use and poll rate to stderr every SECONDS
    """

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data("svalwatch: \(message)\n\n\(usage)\n".utf8))
    exit(2)
}

func parseOptions(_ args: ArraySlice<String>) -> Watcher.Options {
    var options = Watcher.Options()
    var args = args
    func value(_ flag: String) -> String {
        guard let v = args.popFirst() else { fail("\(flag) needs a value") }
        return v
    }
    func positive(_ flag: String) -> Double {
        guard let v = Double(value(flag)), v > 0, v.isFinite else { fail("\(flag) needs a positive number") }
        return v
    }
    while let arg = args.popFirst() {
        switch arg {
        case "--serial": options.serial = value(arg)
        case "--active-hz": options.session.activeHz = positive(arg)
        case "--idle-hz": options.session.idleHz = positive(arg)
        case "--linger": options.session.linger = positive(arg)
        case "--no-passive-wake": options.passiveWake = false
        case "--poll": options.session.useAnnouncements = false
        case "--stats": options.statsEvery = positive(arg)
        case "-h", "--help":
            print(usage)
            exit(0)
        default: fail("unknown option \(arg)")
        }
    }
    return options
}

let stdout = FileHandle.standardOutput
let stderr = FileHandle.standardError

let watcher = Watcher(
    options: parseOptions(CommandLine.arguments.dropFirst()),
    output: { stdout.write(Data(($0 + "\n").utf8)) },
    log: { stderr.write(Data(($0 + "\n").utf8)) })
watcher.start()

// Stop polling while nobody can see an overlay: screen locked, displays
// asleep, or the Mac going to sleep.
let workspace = NSWorkspace.shared.notificationCenter
let distributed = DistributedNotificationCenter.default()
var blockers: Set<String> = []
@MainActor func block(_ reason: String, _ on: Bool) {
    if on { blockers.insert(reason) } else { blockers.remove(reason) }
    blockers.isEmpty ? watcher.resume() : watcher.pause()
}
let observers: [(NotificationCenter, Notification.Name, String, Bool)] = [
    (workspace, NSWorkspace.screensDidSleepNotification, "displays", true),
    (workspace, NSWorkspace.screensDidWakeNotification, "displays", false),
    (workspace, NSWorkspace.willSleepNotification, "sleep", true),
    (workspace, NSWorkspace.didWakeNotification, "sleep", false),
    (distributed, Notification.Name("com.apple.screenIsLocked"), "locked", true),
    (distributed, Notification.Name("com.apple.screenIsUnlocked"), "locked", false),
]
for (center, name, reason, on) in observers {
    center.addObserver(forName: name, object: nil, queue: .main) { _ in
        MainActor.assumeIsolated { block(reason, on) }
    }
}

// Exit when stdin reaches EOF (the parent closed the pipe or died). An
// interactive terminal never sends EOF unless asked to (Ctrl-D).
let stdinSource = DispatchSource.makeReadSource(fileDescriptor: STDIN_FILENO, queue: .main)
stdinSource.setEventHandler {
    var buffer = [UInt8](repeating: 0, count: 512)
    if read(STDIN_FILENO, &buffer, buffer.count) <= 0 {
        let (user, system) = StatsReporter.cpuSplit()
        if ProcessInfo.processInfo.environment["SVALWATCH_CPU"] != nil {
            stderr.write(Data(String(format: "cpu user %.3fs system %.3fs\n", user, system).utf8))
        }
        exit(0)
    }
}
stdinSource.resume()

RunLoop.main.run()
