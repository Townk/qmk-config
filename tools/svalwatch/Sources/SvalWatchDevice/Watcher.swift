import Foundation
import IOKit.hid
import SvalWatchCore

/// Finds the board's Raw HID interface, runs a `Session` against it, and
/// turns the session's actions into IOKit calls and timers. Everything
/// runs on the main run loop: IOKit callbacks, timers and notifications
/// alike, so there is no locking anywhere.
@MainActor
public final class Watcher {
    public struct Options: Sendable {
        public var serial: String?
        public var session = Session.Config()
        public var passiveWake = true
        public var statsEvery: Double?

        public init() {}
    }

    private let options: Options
    private let output: (WatchEvent) -> Void
    private let log: (String) -> Void

    private let manager = IOHIDManagerCreate(kCFAllocatorDefault, IOOptionBits(kIOHIDOptionsTypeNone))
    private var device: IOHIDDevice?
    private var session: Session?
    private let reportBuffer = UnsafeMutablePointer<UInt8>.allocate(capacity: SvalProtocol.reportSize)
    private var timer: Timer?
    private var passive: PassiveInput?
    private var paused = false
    private var stats: StatsReporter?
    private var console: ConsoleReader?
    private var parser = ConsoleParser()

    public init(options: Options, output: @escaping (WatchEvent) -> Void, log: @escaping (String) -> Void) {
        self.options = options
        self.output = output
        self.log = log
    }

    public func start() {
        IOHIDManagerSetDeviceMatching(manager, [
            kIOHIDDeviceUsagePageKey: SvalProtocol.usagePage,
            kIOHIDDeviceUsageKey: SvalProtocol.usage,
        ] as CFDictionary)
        let context = Unmanaged.passUnretained(self).toOpaque()
        IOHIDManagerRegisterDeviceMatchingCallback(manager, { context, _, _, device in
            MainActor.assumeIsolated { Watcher.from(context).attach(device) }
        }, context)
        IOHIDManagerRegisterDeviceRemovalCallback(manager, { context, _, _, device in
            MainActor.assumeIsolated { Watcher.from(context).detach(device) }
        }, context)
        IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), CFRunLoopMode.defaultMode.rawValue)
        let result = IOHIDManagerOpen(manager, IOOptionBits(kIOHIDOptionsTypeNone))
        if result != kIOReturnSuccess {
            log("svalwatch: cannot open the HID manager (IOReturn \(result))")
        }
        if options.session.useAnnouncements {
            console = ConsoleReader { [unowned self] report in
                guard session != nil else { return }
                for line in parser.feed(report) {
                    run(session!.console(line, at: now()))
                }
            }
        }
        if let every = options.statsEvery {
            stats = StatsReporter(every: every, log: log) { [unowned self] in
                (session?.stats, session?.mode, passive?.armed)
            }
        }
    }

    /// Lock, display sleep or system sleep: stop polling entirely. With
    /// announcements, stopping the renewals lets the lease run out, and
    /// the board goes quiet on its own.
    public func pause() {
        guard !paused else { return }
        paused = true
        timer?.invalidate()
        timer = nil
        passive?.arm(false)
    }

    public func resume() {
        guard paused else { return }
        paused = false
        if session != nil { run(session!.poll(at: now())) }
    }

    // MARK: Device lifecycle

    private func attach(_ candidate: IOHIDDevice) {
        guard device == nil else { return }
        let serial = IOHIDDeviceGetProperty(candidate, kIOHIDSerialNumberKey as CFString) as? String ?? ""
        guard serial.hasPrefix(SvalProtocol.serialPrefix) else { return }
        if let wanted = options.serial, wanted != serial { return }

        device = candidate
        IOHIDDeviceRegisterInputReportCallback(
            candidate, reportBuffer, SvalProtocol.reportSize,
            { context, _, _, _, _, report, length in
                let bytes = Array(UnsafeBufferPointer(start: report, count: length))
                MainActor.assumeIsolated { Watcher.from(context).received(bytes) }
            },
            Unmanaged.passUnretained(self).toOpaque())

        session = Session(serial: serial, config: options.session) {
            var g = SystemRandomNumberGenerator()
            return (0..<SvalProtocol.nonceSize).map { _ in UInt8.random(in: .min ... .max, using: &g) }
        }
        if options.passiveWake {
            let vendor = IOHIDDeviceGetProperty(candidate, kIOHIDVendorIDKey as CFString) as? Int ?? 0
            let product = IOHIDDeviceGetProperty(candidate, kIOHIDProductIDKey as CFString) as? Int ?? 0
            passive = PassiveInput(vendor: vendor, product: product, log: log) { [unowned self] in
                guard !paused, session != nil else { return }
                run(session!.passiveInput(at: now()))
            }
        }
        parser.reset()
        console?.follow(serial)
        log("svalwatch: watching \(serial)")
        if !paused { run(session!.poll(at: now())) }
    }

    private func detach(_ gone: IOHIDDevice) {
        guard let current = device, CFEqual(current, gone) else { return }
        timer?.invalidate()
        timer = nil
        passive = nil
        device = nil
        console?.follow(nil)
        let hadConnected = session.map { $0.stats.polls > 0 } ?? false
        session = nil
        if hadConnected { output(.disconnected) }
        log("svalwatch: board disconnected")
        // The manager calls attach() again when it (or another) comes back.
    }

    // MARK: Session plumbing

    private func received(_ report: [UInt8]) {
        guard session != nil else { return }
        run(session!.receive(report, at: now()))
    }

    private func run(_ actions: [Session.Action]) {
        for action in actions {
            switch action {
            case let .send(bytes):
                send(bytes)
            case let .emit(event):
                output(event)
            case let .pollAfter(delay):
                schedule(after: delay) { s, t in s.poll(at: t) }
            case let .timeoutAfter(delay):
                schedule(after: delay) { s, t in s.timeout(at: t) }
            case let .passiveWake(on):
                passive?.arm(on && !paused)
            }
        }
    }

    private func send(_ bytes: [UInt8]) {
        guard let device else { return }
        let result = bytes.withUnsafeBufferPointer {
            IOHIDDeviceSetReport(device, kIOHIDReportTypeOutput, 0, $0.baseAddress!, $0.count)
        }
        if result != kIOReturnSuccess {
            // The removal callback normally follows; the reply timeout
            // covers the case where it does not.
            log("svalwatch: write failed (IOReturn \(result))")
        }
    }

    private func schedule(after delay: Double, _ fire: @escaping @Sendable (inout Session, Double) -> [Session.Action]) {
        timer?.invalidate()
        timer = nil
        guard !paused else { return }
        let t = Timer(timeInterval: delay, repeats: false) { [unowned self] _ in
            MainActor.assumeIsolated {
                timer = nil
                guard session != nil else { return }
                run(fire(&session!, now()))
            }
        }
        // Let macOS batch this wakeup with others. The idle poll can slip
        // a lot; the fast poll and the reply timeout only a little.
        t.tolerance = delay >= 0.1 ? delay * 0.2 : delay * 0.1
        RunLoop.main.add(t, forMode: .default)
        timer = t
    }

    private func now() -> Double {
        ProcessInfo.processInfo.systemUptime
    }

    nonisolated private static func from(_ context: UnsafeMutableRawPointer?) -> Watcher {
        Unmanaged<Watcher>.fromOpaque(context!).takeUnretainedValue()
    }
}
