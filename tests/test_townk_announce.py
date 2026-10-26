# pyright: reportAny=false, reportImplicitOverride=false, reportMissingImports=false
# pyright: reportUnknownMemberType=false, reportUnknownArgumentType=false
# pyright: reportImplicitRelativeImport=false, reportUnusedCallResult=false
"""Host tests for the layer/switch announcements in townk_announce.c.

Drives the subscribe command the way it arrives from the host (a VIA
custom-value GET, already unwrapped from the client wrapper) and the
per-pass poll with explicit state and time, and asserts on the console
lines the board would print.

    python3 tests/run_tests.py
"""

import ctypes
import unittest

from test_townk_mouse import LIB

LIB.T_announce_command.argtypes = [ctypes.c_char_p, ctypes.c_uint8, ctypes.c_uint32]
LIB.T_announce_command.restype = ctypes.c_bool
LIB.T_announce_subscribed.restype = ctypes.c_bool
LIB.T_announce_poll.argtypes = [
    ctypes.c_uint32, ctypes.c_uint32, ctypes.c_uint8, ctypes.c_uint8,
    ctypes.c_char_p, ctypes.c_uint16, ctypes.c_uint16, ctypes.c_uint8,
    ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint32, ctypes.c_char_p, ctypes.c_size_t,
]
LIB.T_announce_poll.restype = ctypes.c_size_t

CHANNEL = 0x57
SUBSCRIBE = 0xF7
ROWS = 10
IDLE_ROWS = bytes(ROWS)

# Pointer flags (townk_announce.h): which trackball scrolls right now, and
# whether a scroll key (hold or toggle) is what flipped it.
LEFT_SCROLL = 0x01
RIGHT_SCROLL = 0x02
SCROLL_KEY = 0x04

# This keymap's boot state: left 400 DPI scrolling, right 1600 DPI pointing.
IDLE_POINTER = "svw P 0190 0640 01 01 01\n"


def command(data: bytes, now: int, length: int = 58) -> tuple[bool, bytes]:
    buf = ctypes.create_string_buffer(data.ljust(length, b"\0"), length)
    ours = bool(LIB.T_announce_command(buf, length, now))
    return ours, buf.raw


def subscribe(now: int, lease_ms: int = 5000) -> bytes:
    ours, reply = command(bytes([0x08, CHANNEL, SUBSCRIBE, lease_ms & 0xFF, lease_ms >> 8]), now)
    assert ours
    return reply


def poll(now: int, active: int = 1, default: int = 1, mods: int = 0, status: int = 0,
         rows: bytes = IDLE_ROWS, left_dpi: int = 400, right_dpi: int = 1600,
         pointer: int = LEFT_SCROLL, sniper: int = 1, boost: int = 1, size: int = 128) -> str:
    out = ctypes.create_string_buffer(size)
    n = int(LIB.T_announce_poll(active, default, mods, status, rows,
                                left_dpi, right_dpi, pointer, sniper, boost, now, out, size))
    assert n == len(out.value), (n, out.value)
    return out.value.decode()


class AnnounceTest(unittest.TestCase):
    def setUp(self) -> None:
        LIB.T_announce_reset()

    def test_silent_without_a_subscriber(self) -> None:
        self.assertEqual(poll(0, active=0x20), "")
        self.assertFalse(LIB.T_announce_subscribed())

    def test_subscribe_reply_carries_signature_and_granted_lease(self) -> None:
        reply = subscribe(1000, lease_ms=5000)
        self.assertEqual(reply[:9], bytes([0x08, CHANNEL, SUBSCRIBE]) + b"svw" + bytes([3, 0x88, 0x13]))
        self.assertTrue(LIB.T_announce_subscribed())

    def test_lease_is_clamped(self) -> None:
        self.assertEqual(subscribe(0, lease_ms=10)[7:9], (500).to_bytes(2, "little"))
        self.assertEqual(subscribe(0, lease_ms=60000)[7:9], (30000).to_bytes(2, "little"))

    def test_first_poll_announces_the_full_state(self) -> None:
        subscribe(0)
        rows = bytes([0, 0x04] + [0] * 8)
        self.assertEqual(
            poll(10, active=0x8001, default=1, mods=0x02, status=0x01, rows=rows),
            "svw L 00008001 00000001 02 01\nsvw K 00040000000000000000\n" + IDLE_POINTER)

    def test_only_changes_are_announced(self) -> None:
        subscribe(0)
        poll(10)
        self.assertEqual(poll(20), "")
        self.assertEqual(poll(30, rows=bytes([0, 0, 0x20] + [0] * 7)), "svw K 00002000000000000000\n")
        self.assertEqual(poll(40, active=1 << 5, rows=bytes([0, 0, 0x20] + [0] * 7)),
                         "svw L 00000020 00000001 00 00\n")
        self.assertEqual(poll(50, active=1 << 5, mods=0x01, rows=bytes([0, 0, 0x20] + [0] * 7)),
                         "svw L 00000020 00000001 01 00\n")

    def test_pointer_changes_are_announced(self) -> None:
        subscribe(0)
        poll(10)
        # A DPI key on the right trackball: 1600 -> 2400.
        self.assertEqual(poll(20, right_dpi=2400), "svw P 0190 0960 01 01 01\n")
        # Scroll toggle latched: both sides flip, flagged as a scroll key.
        self.assertEqual(poll(30, right_dpi=2400, pointer=RIGHT_SCROLL | SCROLL_KEY),
                         "svw P 0190 0960 06 01 01\n")
        self.assertEqual(poll(40, right_dpi=2400, pointer=RIGHT_SCROLL | SCROLL_KEY), "")

    def test_the_largest_dpi_fits(self) -> None:
        subscribe(0)
        poll(10)
        self.assertEqual(poll(20, left_dpi=12000, right_dpi=12000), "svw P 2EE0 2EE0 01 01 01\n")

    def test_sniper_and_boost_are_announced(self) -> None:
        subscribe(0)
        poll(10)
        # Sniper ×3 held: both trackballs move at a third.
        self.assertEqual(poll(20, sniper=3), "svw P 0190 0640 01 03 01\n")
        # ×3 and ×5 together: 15. Released: back to 1.
        self.assertEqual(poll(30, sniper=15), "svw P 0190 0640 01 0F 01\n")
        self.assertEqual(poll(40), "svw P 0190 0640 01 01 01\n")
        self.assertEqual(poll(50, boost=2), "svw P 0190 0640 01 01 02\n")

    def test_renewal_reannounces_everything(self) -> None:
        subscribe(0)
        poll(10)
        subscribe(2000)
        self.assertEqual(poll(2010),
                         "svw L 00000001 00000001 00 00\nsvw K 00000000000000000000\n" + IDLE_POINTER)

    def test_goes_quiet_when_the_lease_runs_out(self) -> None:
        subscribe(0, lease_ms=1000)
        poll(10)
        self.assertEqual(poll(999, active=4), "svw L 00000004 00000001 00 00\n")
        self.assertEqual(poll(1000, active=8), "")
        self.assertFalse(LIB.T_announce_subscribed())

    def test_lease_survives_the_timer_wrapping(self) -> None:
        start = 0xFFFF_FF00
        subscribe(start, lease_ms=1000)
        self.assertNotEqual(poll(start + 10), "")
        self.assertEqual(poll((start + 900) & 0xFFFF_FFFF, active=2), "svw L 00000002 00000001 00 00\n")
        self.assertEqual(poll((start + 1000) & 0xFFFF_FFFF, active=4), "")

    def test_other_channels_are_not_ours(self) -> None:
        # Svalboard's own settings (channel 0) and tools (0x49, 0x53-0x55).
        for channel in (0x00, 0x49, 0x53, 0x54, 0x55):
            ours, reply = command(bytes([0x08, channel, SUBSCRIBE, 0x88, 0x13]), 0)
            self.assertFalse(ours, hex(channel))
            self.assertEqual(reply[:5], bytes([0x08, channel, SUBSCRIBE, 0x88, 0x13]))
        self.assertFalse(LIB.T_announce_subscribed())

    def test_unknown_commands_on_our_channel_are_unhandled_not_passed_on(self) -> None:
        # A SET with value id 1 would be Svalboard's left DPI if it got through.
        ours, reply = command(bytes([0x07, CHANNEL, 0x01, 0x03]), 0)
        self.assertTrue(ours)
        self.assertEqual(reply[0], 0xFF)
        ours, reply = command(bytes([0x08, CHANNEL, 0x01]), 0)
        self.assertTrue(ours)
        self.assertEqual(reply[0], 0xFF)
        self.assertFalse(LIB.T_announce_subscribed())

    def test_short_commands_are_ignored(self) -> None:
        ours, _ = command(bytes([0x08, CHANNEL, SUBSCRIBE, 0x88, 0x13]), 0, length=8)
        self.assertFalse(ours)

    def test_a_small_buffer_truncates_cleanly(self) -> None:
        subscribe(0)
        text = poll(10, size=32)
        self.assertLessEqual(len(text), 31)
        self.assertTrue(text.startswith("svw L 00000001"))


if __name__ == "__main__":
    unittest.main()
