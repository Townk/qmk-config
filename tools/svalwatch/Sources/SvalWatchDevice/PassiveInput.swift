import Foundation
import IOKit.hid

/// Listens to the board's ordinary keyboard and mouse reports, the ones
/// the Mac receives anyway, to tell the session "someone is typing" with
/// no polling. Armed only while the session is idle: while it polls fast,
/// trackball motion alone would add hundreds of wakeups a second.
///
/// Opening a keyboard this way needs the Input Monitoring permission (for
/// the process that launched svalwatch, e.g. Terminal or Hammerspoon).
/// Without it, svalwatch still works and simply relies on its idle poll.
@MainActor
final class PassiveInput {
    // nonisolated(unsafe) so deinit can reach it: deinit is nonisolated
    // before Swift 6.2's isolated deinit, and Watcher only ever drops this
    // object on the main thread anyway.
    nonisolated(unsafe) private let manager = IOHIDManagerCreate(kCFAllocatorDefault, IOOptionBits(kIOHIDOptionsTypeNone))
    private let onInput: () -> Void
    private var available = false
    private(set) var armed = false

    init(vendor: Int, product: Int, log: (String) -> Void, onInput: @escaping () -> Void) {
        self.onInput = onInput
        let base: [String: Int] = [kIOHIDVendorIDKey: vendor, kIOHIDProductIDKey: product]
        let match = [
            (kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard),
            (kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse),
        ].map { page, usage in
            base.merging([kIOHIDDeviceUsagePageKey: Int(page), kIOHIDDeviceUsageKey: Int(usage)]) { $1 }
        }
        IOHIDManagerSetDeviceMatchingMultiple(manager, match as CFArray)
        IOHIDManagerScheduleWithRunLoop(manager, CFRunLoopGetMain(), CFRunLoopMode.defaultMode.rawValue)
        let result = IOHIDManagerOpen(manager, IOOptionBits(kIOHIDOptionsTypeNone))
        available = result == kIOReturnSuccess
        if !available {
            log("svalwatch: no passive wake (IOReturn \(result); Input Monitoring not granted?)"
                + " -- falling back to the idle poll alone")
        }
    }

    deinit {
        IOHIDManagerRegisterInputValueCallback(manager, nil, nil)
        IOHIDManagerUnscheduleFromRunLoop(manager, CFRunLoopGetMain(), CFRunLoopMode.defaultMode.rawValue)
        IOHIDManagerClose(manager, IOOptionBits(kIOHIDOptionsTypeNone))
    }

    func arm(_ on: Bool) {
        guard available, on != armed else { return }
        armed = on
        if on {
            IOHIDManagerRegisterInputValueCallback(manager, { context, _, _, _ in
                MainActor.assumeIsolated {
                    let me = Unmanaged<PassiveInput>.fromOpaque(context!).takeUnretainedValue()
                    // One notice is enough; the session disarms us on it.
                    if me.armed { me.onInput() }
                }
            }, Unmanaged.passUnretained(self).toOpaque())
        } else {
            IOHIDManagerRegisterInputValueCallback(manager, nil, nil)
        }
    }
}
