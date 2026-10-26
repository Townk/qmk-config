import Foundation
import SvalWatchCore

/// `--stats N`: every N seconds, one line on stderr with this process's
/// own CPU use and poll rate over the interval. This is the profiling
/// hook: run svalwatch, use the keyboard normally, read the numbers.
@MainActor
final class StatsReporter {
    private let every: Double
    private let log: (String) -> Void
    private let sample: () -> (Session.Stats?, Session.Mode?, Bool?)
    private var timer: Timer?
    private var last: (time: Double, cpu: Double, polls: Int, renewals: Int, lines: Int)

    init(every: Double, log: @escaping (String) -> Void,
         sample: @escaping () -> (Session.Stats?, Session.Mode?, Bool?)) {
        self.every = every
        self.log = log
        self.sample = sample
        last = (ProcessInfo.processInfo.systemUptime, Self.cpuSeconds(), 0, 0, 0)
        let t = Timer(timeInterval: every, repeats: true) { [unowned self] _ in
            MainActor.assumeIsolated { report() }
        }
        t.tolerance = every * 0.1
        RunLoop.main.add(t, forMode: .default)
        timer = t
    }

    private func report() {
        let now = ProcessInfo.processInfo.systemUptime
        let cpu = Self.cpuSeconds()
        let (stats, mode, armed) = sample()
        let polls = stats?.polls ?? last.polls
        let wall = now - last.time
        // A reconnect restarts the session's counters.
        let newPolls = polls >= last.polls ? polls - last.polls : polls
        log(String(
            format: "svalwatch stats: %@, cpu %.2f%% of a core, %.1f polls/s, %d renewals, %d lines, passive %@, timeouts %d, rejections %d",
            mode.map { "\($0)" } ?? "no board", (cpu - last.cpu) / wall * 100, Double(newPolls) / wall,
            (stats?.renewals ?? 0) - last.renewals, (stats?.announcements ?? 0) - last.lines,
            armed.map { $0 ? "armed" : "off" } ?? "n/a",
            stats?.timeouts ?? 0, stats?.rejections ?? 0))
        last = (now, cpu, polls, stats?.renewals ?? 0, stats?.announcements ?? 0)
    }

    private static func cpuSeconds() -> Double {
        let (user, system) = cpuSplit()
        return user + system
    }

    static func cpuSplit() -> (user: Double, system: Double) {
        var usage = rusage()
        getrusage(RUSAGE_SELF, &usage)
        func seconds(_ t: timeval) -> Double { Double(t.tv_sec) + Double(t.tv_usec) / 1e6 }
        return (seconds(usage.ru_utime), seconds(usage.ru_stime))
    }
}
