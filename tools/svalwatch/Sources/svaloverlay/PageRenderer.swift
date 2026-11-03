import Foundation
import SvalWatchCore

/// Draws the overlay page for the keymap a board reports, with the render
/// bundle that ships next to the default page (`<assets>/render`, from
/// `make overlay-assets`): `render_page.py` run through `uv`, so the page
/// goes through the same skim version and config as the default one.
///
/// Pages are cached in `~/Library/Caches/svaloverlay/pages/<key>/`, named
/// by `PageCache.key`, the five most recently used kept. A render runs
/// only on a cache miss, one at a time, at utility priority, and is given
/// up after two minutes: a keymap that arrives meanwhile waits its turn,
/// replacing any other one waiting. A failed render is never retried; the
/// overlay keeps the page it has and the next connect tries again.
@MainActor
final class PageRenderer {
    static let keep = 5
    nonisolated static let timeout: TimeInterval = 120
    /// How long a `uv --version` probe may take before that uv counts as
    /// broken (a shim that hangs must not hold the render slot forever).
    nonisolated static let probeTimeout: TimeInterval = 5

    let bundle: URL
    let pages: URL
    private let log: (String) -> Void
    /// The key being drawn right now, if any.
    private var running: String?
    private var queued: Job?
    private let current = RunningProcess()

    /// `log` gets one line per render, drawn or failed.
    init(bundle: URL, log: @escaping (String) -> Void) {
        self.bundle = bundle
        pages = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("svaloverlay/pages", isDirectory: true)
        self.log = log
        sweep()
    }

    /// Removes what a render killed with the process left behind: a
    /// half-drawn `pages/<key>.tmp` and its scratch keymap directory.
    private func sweep() {
        let fm = FileManager.default
        for name in (try? fm.contentsOfDirectory(atPath: pages.path)) ?? [] where name.hasSuffix(".tmp") {
            try? fm.removeItem(at: pages.appendingPathComponent(name))
        }
        let temp = fm.temporaryDirectory
        for name in (try? fm.contentsOfDirectory(atPath: temp.path)) ?? [] where name.hasPrefix(Self.scratchPrefix) {
            try? fm.removeItem(at: temp.appendingPathComponent(name))
        }
    }

    /// The bundle's VERSION, or nil while there is no bundle to render
    /// with (an old install, or an overlay rebuild swapping it right now).
    func bundleVersion() -> String? {
        guard let data = FileManager.default.contents(atPath: bundle.appendingPathComponent("VERSION").path),
              FileManager.default.fileExists(atPath: bundle.appendingPathComponent("render_page.py").path)
        else { return nil }
        let version = String(decoding: data, as: UTF8.self).trimmingCharacters(in: .whitespacesAndNewlines)
        return version.isEmpty ? nil : version
    }

    /// The page already drawn for `key`, marked as just used so pruning
    /// keeps it.
    func cachedPage(_ key: String) -> URL? {
        let page = pages.appendingPathComponent(key, isDirectory: true)
        guard FileManager.default.fileExists(atPath: page.appendingPathComponent("index.html").path) else {
            return nil
        }
        try? FileManager.default.setAttributes([.modificationDate: Date()], ofItemAtPath: page.path)
        return page
    }

    /// Draws `keymap` into `pages/<key>`, then calls `done` with the page,
    /// or with nil if it could not be drawn (already logged). The page
    /// being drawn already (a reconnect mid-render) is not drawn twice.
    func render(_ keymap: Keymap, key: String, done: @escaping @MainActor (String, URL?) -> Void) {
        let job = Job(keymap: keymap, key: key, bundle: bundle, pages: pages, done: done)
        if running == key {
            queued = nil
        } else if running != nil {
            queued = job
        } else {
            start(job)
        }
    }

    /// Forgets the render waiting its turn: a newer keymap no longer needs
    /// it (its page was cached).
    func dropQueued() {
        queued = nil
    }

    /// Stops the render in flight, if any: the process is exiting, and a
    /// render left behind would race the next start's.
    func cancel() {
        queued = nil
        current.terminate()
    }

    private func start(_ job: Job) {
        running = job.key
        let current = current
        let started = Date()
        DispatchQueue.global(qos: .utility).async {
            var notes: [String] = []
            let outcome = Self.run(job, current, notes: &notes)
            let said = notes
            Task { @MainActor in self.finish(job, outcome, notes: said, seconds: -started.timeIntervalSinceNow) }
        }
    }

    private func finish(_ job: Job, _ outcome: Outcome, notes: [String], seconds: Double) {
        running = nil
        for note in notes { log("svaloverlay: \(note)") }
        switch outcome {
        case let .rendered(page):
            log("svaloverlay: drew the board's keymap in \(String(format: "%.1f", seconds)) s: \(page.path)")
            prune()
            job.done(job.key, page)
        case let .failed(reason):
            log("svaloverlay: page render failed, keeping the current page: \(reason)")
            job.done(job.key, nil)
        }
        if let next = queued {
            queued = nil
            start(next)
        }
    }

    private func prune() {
        let fm = FileManager.default
        let names = (try? fm.contentsOfDirectory(atPath: pages.path)) ?? []
        let dirs = names.compactMap { name -> (name: String, modified: Date)? in
            let attrs = try? fm.attributesOfItem(atPath: pages.appendingPathComponent(name).path)
            guard let modified = attrs?[.modificationDate] as? Date else { return nil }
            return (name, modified)
        }
        for name in PageCache.prune(dirs: dirs, keep: Self.keep, rendering: running) {
            try? fm.removeItem(at: pages.appendingPathComponent(name))
        }
    }

    // MARK: The render itself, off the main thread

    struct Job: Sendable {
        let keymap: Keymap
        let key: String
        let bundle: URL
        let pages: URL
        let done: @MainActor (String, URL?) -> Void
    }

    enum Outcome: Sendable {
        case rendered(URL)
        case failed(String)
    }

    nonisolated static let scratchPrefix = "svaloverlay-render-"

    /// Writes the keymap where render_page.py reads it, runs it into
    /// `pages/<key>.tmp` and renames that to `pages/<key>` once it is
    /// whole, so a page directory is never half-written. Anything worth a
    /// log line besides the outcome goes to `notes`.
    nonisolated private static func run(_ job: Job, _ current: RunningProcess, notes: inout [String]) -> Outcome {
        let fm = FileManager.default
        let page = job.pages.appendingPathComponent(job.key, isDirectory: true)
        let tmp = job.pages.appendingPathComponent(job.key + ".tmp", isDirectory: true)
        let scratch = fm.temporaryDirectory.appendingPathComponent(scratchPrefix + job.key, isDirectory: true)
        defer { try? fm.removeItem(at: scratch) }
        do {
            try fm.createDirectory(at: job.pages, withIntermediateDirectories: true)
            try? fm.removeItem(at: tmp)  // left by a render that was killed
            try? fm.removeItem(at: scratch)
            try fm.createDirectory(at: scratch, withIntermediateDirectories: true)
        } catch {
            return .failed("cannot prepare \(job.pages.path): \(error.localizedDescription)")
        }

        guard let uv = findUV(notes: &notes) else {
            return .failed("no uv on PATH, at ~/.local/share/mise or at /opt/homebrew/bin")
        }
        let keymapFile = scratch.appendingPathComponent("keymap.json")
        let errFile = scratch.appendingPathComponent("stderr")
        let k = job.keymap
        let json = #"{"layers":\#(k.layers),"rows":\#(k.rows),"cols":\#(k.cols),"keycodes":["#
            + k.keycodes.map(String.init).joined(separator: ",") + "]}"
        guard fm.createFile(atPath: keymapFile.path, contents: Data(json.utf8)),
              fm.createFile(atPath: errFile.path, contents: nil),
              let errHandle = FileHandle(forWritingAtPath: errFile.path)
        else { return .failed("cannot write to \(scratch.path)") }
        defer { try? errHandle.close() }

        let process = Process()
        process.executableURL = uv
        process.arguments = PageCache.renderArguments(bundle: job.bundle.path, keymap: keymapFile.path, out: tmp.path)
        process.currentDirectoryURL = job.bundle
        process.standardInput = FileHandle.nullDevice
        process.standardOutput = FileHandle.nullDevice
        process.standardError = errHandle  // a file, so a chatty child never blocks on a pipe
        process.qualityOfService = .utility
        let exited = DispatchSemaphore(value: 0)
        process.terminationHandler = { _ in exited.signal() }
        do {
            guard try current.launch(process) else { return .failed("cancelled: svaloverlay is exiting") }
        } catch {
            return .failed("cannot run \(uv.path): \(error.localizedDescription)")
        }
        defer { current.finished() }

        if exited.wait(timeout: .now() + timeout) == .timedOut {
            process.terminate()
            if exited.wait(timeout: .now() + 5) == .timedOut { kill(process.processIdentifier, SIGKILL) }
            try? fm.removeItem(at: tmp)
            return .failed("gave up after \(Int(timeout)) s")
        }
        guard process.terminationReason == .exit, process.terminationStatus == 0,
              fm.fileExists(atPath: tmp.appendingPathComponent("index.html").path)
        else {
            try? fm.removeItem(at: tmp)
            let said = (try? String(contentsOf: errFile, encoding: .utf8))?
                .split(whereSeparator: \.isNewline).last.map(String.init) ?? ""
            let how = process.terminationReason == .exit
                ? "exit \(process.terminationStatus)" : "signal \(process.terminationStatus)"
            return .failed(said.isEmpty ? "render_page.py: \(how)" : "\(said) (\(how))")
        }
        do {
            try? fm.removeItem(at: page)
            try fm.moveItem(at: tmp, to: page)
        } catch {
            try? fm.removeItem(at: tmp)
            return .failed("cannot move the page into \(page.path): \(error.localizedDescription)")
        }
        return .rendered(page)
    }

    /// `uv`, wherever this Mac has it. Hammerspoon starts the overlay with
    /// a minimal PATH, so after PATH come mise's shim and Homebrew's, then
    /// mise's installs themselves in case the shim cannot resolve without
    /// mise's environment. The first that answers `uv --version` within
    /// `probeTimeout` wins; one that hangs is noted and skipped.
    nonisolated static func findUV(notes: inout [String]) -> URL? {
        let fm = FileManager.default
        let home = fm.homeDirectoryForCurrentUser
        let path = ProcessInfo.processInfo.environment["PATH"] ?? ""
        var candidates = path.split(separator: ":").map { URL(fileURLWithPath: String($0)).appendingPathComponent("uv") }
        candidates.append(home.appendingPathComponent(".local/share/mise/shims/uv"))
        candidates.append(URL(fileURLWithPath: "/opt/homebrew/bin/uv"))
        let installs = home.appendingPathComponent(".local/share/mise/installs/uv")
        for version in ((try? fm.contentsOfDirectory(atPath: installs.path)) ?? []).sorted().reversed() {
            let dir = installs.appendingPathComponent(version)
            for sub in (try? fm.contentsOfDirectory(atPath: dir.path)) ?? [] where sub.hasPrefix("uv") {
                candidates.append(dir.appendingPathComponent(sub).appendingPathComponent("uv"))
            }
        }
        for uv in candidates where fm.isExecutableFile(atPath: uv.path) {
            switch probe(uv) {
            case .answers: return uv
            case .fails: continue
            case .hangs: notes.append("\(uv.path) --version did not answer in \(Int(probeTimeout)) s; skipped it")
            }
        }
        return nil
    }

    private enum Probe { case answers, fails, hangs }

    nonisolated private static func probe(_ uv: URL) -> Probe {
        let probe = Process()
        probe.executableURL = uv
        probe.arguments = ["--version"]
        probe.standardInput = FileHandle.nullDevice
        probe.standardOutput = FileHandle.nullDevice
        probe.standardError = FileHandle.nullDevice
        let exited = DispatchSemaphore(value: 0)
        probe.terminationHandler = { _ in exited.signal() }
        guard (try? probe.run()) != nil else { return .fails }
        if exited.wait(timeout: .now() + probeTimeout) == .timedOut {
            probe.terminate()
            if exited.wait(timeout: .now() + 1) == .timedOut { kill(probe.processIdentifier, SIGKILL) }
            return .hangs
        }
        return probe.terminationReason == .exit && probe.terminationStatus == 0 ? .answers : .fails
    }
}

/// The render process in flight, so `cancel()` can reach it from the main
/// thread while the render's own thread waits on it. Launching happens
/// under the same lock, so a cancel lands either before the launch (which
/// then does not happen) or after it (and stops the child).
private final class RunningProcess: @unchecked Sendable {
    private let lock = NSLock()
    private var process: Process?
    private var cancelled = false

    /// Runs `p` unless cancelled; false if it was not run.
    func launch(_ p: Process) throws -> Bool {
        try lock.withLock {
            guard !cancelled else { return false }
            try p.run()
            process = p
            return true
        }
    }

    func finished() {
        lock.withLock { process = nil }
    }

    /// Stops the child, if any, and every later launch.
    func terminate() {
        lock.withLock {
            cancelled = true
            if let process, process.isRunning { process.terminate() }
        }
    }
}
