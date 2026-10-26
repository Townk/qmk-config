import Foundation
import IOKit.hid
import SvalWatchCore

/// Reads the board's QMK debug console, where townk_announce.c prints its
/// lines. Purely event-driven: the callback runs only when the board sends
/// a report, so a quiet board costs nothing.
///
/// The console is a separate HID interface of the same USB device, so it
/// is matched by the Raw HID interface's serial number. It may enumerate a
/// moment before or after that one; whichever comes second links them.
@MainActor
final class ConsoleReader {
    private let manager = IOHIDManagerCreate(kCFAllocatorDefault, IOOptionBits(kIOHIDOptionsTypeNone))
    private let onReport: ([UInt8]) -> Void
    private var serial: String?
    private var device: IOHIDDevice?
    private var buffer: UnsafeMutablePointer<UInt8>?
    private var bufferSize = 0

    init(onReport: @escaping ([UInt8]) -> Void) {
        self.onReport = onReport
        IOHIDManagerSetDeviceMatching(manager, [
            kIOHIDDeviceUsagePageKey: SvalProtocol.consoleUsagePage,
            kIOHIDDeviceUsageKey: SvalProtocol.consoleUsage,
        ] as CFDictionary)
        let context = Unmanaged.passUnretained(self).toOpaque()
        IOHIDManagerRegisterDeviceMatchingCallback(manager, { context, _, _, device in
            MainActor.assumeIsolated { ConsoleReader.from(context).matched(device) }
        }, context)
        IOHIDManagerRegisterDeviceRemovalCallback(manager, { context, _, _, device in
            MainActor.assumeIsolated { ConsoleReader.from(context).removed(device) }
        }, context)
        IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), CFRunLoopMode.defaultMode.rawValue)
        IOHIDManagerOpen(manager, IOOptionBits(kIOHIDOptionsTypeNone))
    }

    /// Read the console of the board with this serial (nil: stop reading).
    func follow(_ serial: String?) {
        self.serial = serial
        stop()
        guard serial != nil, let devices = IOHIDManagerCopyDevices(manager) as? Set<IOHIDDevice> else { return }
        for d in devices { matched(d) }
    }

    private func matched(_ candidate: IOHIDDevice) {
        guard device == nil, let serial,
              IOHIDDeviceGetProperty(candidate, kIOHIDSerialNumberKey as CFString) as? String == serial
        else { return }
        device = candidate
        bufferSize = IOHIDDeviceGetProperty(candidate, kIOHIDMaxInputReportSizeKey as CFString) as? Int ?? 64
        buffer = .allocate(capacity: bufferSize)
        IOHIDDeviceRegisterInputReportCallback(candidate, buffer!, bufferSize, { context, _, _, _, _, report, length in
            let bytes = Array(UnsafeBufferPointer(start: report, count: length))
            MainActor.assumeIsolated { ConsoleReader.from(context).onReport(bytes) }
        }, Unmanaged.passUnretained(self).toOpaque())
    }

    private func removed(_ gone: IOHIDDevice) {
        if let device, CFEqual(device, gone) { stop() }
    }

    private func stop() {
        if let device {
            IOHIDDeviceRegisterInputReportCallback(device, buffer!, bufferSize, nil, nil)
        }
        device = nil
        buffer?.deallocate()
        buffer = nil
    }

    nonisolated private static func from(_ context: UnsafeMutableRawPointer?) -> ConsoleReader {
        Unmanaged<ConsoleReader>.fromOpaque(context!).takeUnretainedValue()
    }
}
