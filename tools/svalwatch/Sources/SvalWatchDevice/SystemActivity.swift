import AppKit

/// Tells a `Watcher` when nobody can be looking: the screen is locked,
/// the displays sleep, or the Mac is going to sleep. It pauses on the
/// first of those and resumes once all of them have cleared.
@MainActor
public final class SystemActivity {
    private let watcher: Watcher
    private var blockers: Set<String> = []
    private var tokens: [(NotificationCenter, NSObjectProtocol)] = []

    public init(watcher: Watcher) {
        self.watcher = watcher
        let workspace = NSWorkspace.shared.notificationCenter
        let distributed = DistributedNotificationCenter.default()
        let observed: [(NotificationCenter, Notification.Name, String, Bool)] = [
            (workspace, NSWorkspace.screensDidSleepNotification, "displays", true),
            (workspace, NSWorkspace.screensDidWakeNotification, "displays", false),
            (workspace, NSWorkspace.willSleepNotification, "sleep", true),
            (workspace, NSWorkspace.didWakeNotification, "sleep", false),
            (distributed, Notification.Name("com.apple.screenIsLocked"), "locked", true),
            (distributed, Notification.Name("com.apple.screenIsUnlocked"), "locked", false),
        ]
        for (center, name, reason, on) in observed {
            let token = center.addObserver(forName: name, object: nil, queue: .main) { [weak self] _ in
                MainActor.assumeIsolated { self?.block(reason, on) }
            }
            tokens.append((center, token))
        }
    }

    private func block(_ reason: String, _ on: Bool) {
        if on { blockers.insert(reason) } else { blockers.remove(reason) }
        blockers.isEmpty ? watcher.resume() : watcher.pause()
    }
}

/// Exits the process when stdin reaches EOF: the parent closed the pipe
/// or died. An interactive terminal never sends EOF unless asked (Ctrl-D).
/// Each complete line read before that goes to `onLine`.
@MainActor
public final class StdinWatch {
    private let source = DispatchSource.makeReadSource(fileDescriptor: STDIN_FILENO, queue: .main)
    private var pending = Data()

    public init(onEOF: @escaping @MainActor () -> Void = { exit(0) },
                onLine: @escaping @MainActor (String) -> Void = { _ in }) {
        source.setEventHandler { [weak self] in
            MainActor.assumeIsolated {
                var buffer = [UInt8](repeating: 0, count: 512)
                let n = read(STDIN_FILENO, &buffer, buffer.count)
                if n <= 0 {
                    onEOF()
                    return
                }
                guard let self else { return }
                self.pending.append(contentsOf: buffer[0..<n])
                while let newline = self.pending.firstIndex(of: UInt8(ascii: "\n")) {
                    let line = String(decoding: self.pending[self.pending.startIndex..<newline], as: UTF8.self)
                    self.pending.removeSubrange(self.pending.startIndex...newline)
                    onLine(line.trimmingCharacters(in: .whitespaces))
                }
            }
        }
        source.resume()
    }
}
