# pyright: reportAny=false, reportUnknownVariableType=false
# pyright: reportImplicitOverride=false, reportMissingImports=false
# pyright: reportUnknownMemberType=false, reportUnknownArgumentType=false
"""Host tests for the host pointing-profile detector in townk_hostos.c.

Drives the module through the same fixture as the mouse tests (see
tests/townk_mouse_layout.c): boot, string-descriptor reads tagged with the
USB configuration state they arrived in, and the configured edge. Asserts
on the one output that matters -- which profile the keymap was told to
apply -- and on the SV_SOUT status line.

    python3 tests/run_tests.py
"""

import ctypes
import unittest

from test_townk_mouse import LIB

MAC: int = int(LIB.T_hostos_mac())
PC: int = int(LIB.T_hostos_pc())
UNKNOWN: int = int(LIB.T_hostos_unknown())
V_NONE: int = int(LIB.T_hostos_v_none())
V_MAC: int = int(LIB.T_hostos_v_mac())
V_WINDOWS: int = int(LIB.T_hostos_v_windows())
V_LINUX: int = int(LIB.T_hostos_v_linux())

# Real wLength sequences, from QMK's os_detection tests. The split marks
# SET_CONFIGURATION: strings before it are the enumeration, after it driver
# and application chatter.
MAC_SEQUOIA_PRE = [0x02, 0x4E, 0x02, 0x1C, 0x02, 0x1A]
MAC_SEQUOIA_POST = [0xFF, 0xFF]
MAC_OLD_PRE = [0x02, 0x24, 0x02, 0x28]
MAC_OLD_POST = [0xFF]
IOS_PRE = [0x02, 0x24, 0x02, 0x28]
LINUX_PRE = [0xFF, 0xFF, 0xFF, 0xFF]
LINUX_POST = [0xFF, 0xFF]
WINDOWS_PRE = [0xFF, 0xFF, 0x04, 0x24, 0x04, 0x24, 0x04, 0xFF, 0x24]
WINDOWS_POST = [0x20A] * 6
# What a cached macOS re-attach leaves: no length probes, only 0xFF reads.
CACHED_MAC_POST = [0xFF, 0xFF, 0xFF]
# This Mac (macOS 26), captured by SV_SOUT: three length-probe pairs, then
# the 4-byte LANGID read after configuration, and never a 0xFF. QMK files
# it under "PS5" and reports Linux.
MAC_TAHOE_PRE = [0x02, 0x10, 0x02, 0x14, 0x02, 0x1C]
MAC_TAHOE_POST = [0x04]


def _counts(seq: list[int]) -> tuple[int, int, int, int]:
    return (len(seq), seq.count(0x02), seq.count(0x04), seq.count(0xFF))


class HostOsClassifyTest(unittest.TestCase):
    """hostos_classify(): the fingerprint over enumeration-time counts."""

    def classify(self, seq: list[int]) -> int:
        return int(LIB.T_hostos_classify(*_counts(seq)))

    def test_macos_pairs_without_trailing_ff(self):
        # QMK needs the trailing 0xFF to say macOS; that read is
        # post-configuration, so it must not be required here.
        self.assertEqual(self.classify(MAC_SEQUOIA_PRE), V_MAC)
        self.assertEqual(self.classify(MAC_OLD_PRE), V_MAC)
        self.assertEqual(self.classify([0x02, 0x10, 0x02, 0x0E]), V_MAC)

    def test_ios_is_mac(self):
        self.assertEqual(self.classify(IOS_PRE), V_MAC)

    def test_linux_all_ff(self):
        self.assertEqual(self.classify([0xFF, 0xFF, 0xFF]), V_LINUX)
        self.assertEqual(self.classify(LINUX_PRE), V_LINUX)

    def test_quest_and_switch_are_linux(self):
        self.assertEqual(self.classify([0xFF, 0xFF, 0xFF, 0xFE]), V_LINUX)
        self.assertEqual(self.classify([0x82, 0xFF, 0x40, 0x40]), V_LINUX)

    def test_this_mac_with_its_langid_read_is_mac(self):
        # The captured sequence, with the 0x04 on either side of the
        # configuration edge: QMK's "PS5" rule must not apply.
        self.assertEqual(self.classify(MAC_TAHOE_PRE), V_MAC)
        self.assertEqual(self.classify(MAC_TAHOE_PRE + MAC_TAHOE_POST), V_MAC)

    def test_length_probes_outrank_everything(self):
        # PS5-shaped and Windows-shaped tails after paired probes are still
        # a mac: no Windows or Linux enumeration reads with wLength 2.
        self.assertEqual(self.classify([0x02, 0x04, 0x02, 0x28, 0x02, 0x24]), V_MAC)
        self.assertEqual(self.classify(MAC_SEQUOIA_PRE + [0x04, 0xFF, 0xFF]), V_MAC)

    def test_windows(self):
        self.assertEqual(self.classify(WINDOWS_PRE), V_WINDOWS)
        self.assertEqual(self.classify([0xFF, 0xFF, 0x04, 0x10, 0x04, 0x10]), V_WINDOWS)
        self.assertEqual(self.classify([0x12, 0xFF, 0xFF, 0x04, 0x10]), V_WINDOWS)

    def test_too_little_evidence(self):
        self.assertEqual(self.classify([]), V_NONE)
        self.assertEqual(self.classify([0xFF]), V_NONE)
        self.assertEqual(self.classify([0x12, 0xFF]), V_NONE)
        self.assertEqual(self.classify([0x02, 0x24]), V_NONE)
        self.assertEqual(self.classify([0x10, 0x20, 0x30]), V_NONE)


class HostOsProfileTest(unittest.TestCase):
    """The policy: what the keyboard is told to apply, boot after boot."""

    def setUp(self):
        LIB.T_reset()

    def tearDown(self):
        LIB.T_reset()

    # -- helpers --------------------------------------------------------

    def enumerate(self, pre: list[int], post: list[int] = []) -> None:
        for w in pre:
            LIB.T_hostos_read(w, False)
        LIB.T_hostos_usb(True)
        for w in post:
            LIB.T_hostos_read(w, True)

    def cold_boot(self) -> None:
        LIB.T_hostos_forget(False)
        LIB.T_hostos_boot()

    def warm_boot(self) -> None:
        LIB.T_hostos_forget(True)
        LIB.T_hostos_boot()

    def status(self) -> str:
        buf = ctypes.create_string_buffer(256)
        n = int(LIB.T_hostos_status(buf, 256))
        self.assertGreater(n, 0)
        return buf.value.decode()

    # -- cold boots -----------------------------------------------------

    def test_cold_boot_seeds_mac_before_any_evidence(self):
        self.cold_boot()
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertEqual(int(LIB.T_hostos_last_profile()), MAC)
        self.assertEqual(int(LIB.T_hostos_profile_calls()), 1)

    def test_cold_boot_on_mac_keeps_mac_without_extra_apply(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertEqual(int(LIB.T_hostos_profile_calls()), 1)

    def test_cold_boot_on_linux_switches_to_pc(self):
        self.cold_boot()
        self.enumerate(LINUX_PRE, LINUX_POST)
        self.assertEqual(int(LIB.T_hostos_active()), PC)
        self.assertEqual(int(LIB.T_hostos_last_profile()), PC)

    def test_cold_boot_on_windows_switches_to_pc(self):
        self.cold_boot()
        self.enumerate(WINDOWS_PRE, WINDOWS_POST)
        self.assertEqual(int(LIB.T_hostos_active()), PC)

    def test_cold_boot_on_this_mac_is_mac(self):
        self.cold_boot()
        self.enumerate(MAC_TAHOE_PRE, MAC_TAHOE_POST)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertIn("verdict mac", self.status())
        self.assertIn("reads: 02 10 02 14 02 1C | 04", self.status())

    def test_cold_boot_on_ios_is_mac(self):
        self.cold_boot()
        self.enumerate(IOS_PRE)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)

    def test_mac_verdict_does_not_wait_for_post_config_chatter(self):
        # The profile is decided at the configured edge, from the pairs
        # alone -- the trailing 0xFF QMK waits for may never come.
        self.cold_boot()
        self.enumerate(MAC_OLD_PRE)
        self.assertIn("verdict mac", self.status())

    def test_post_config_reads_never_change_the_profile(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        # Mid-session chatter shaped like every other OS.
        for w in [0xFF, 0xFF, 0x04, 0x24, 0xFF, 0xFF, 0xFF, 0x20A]:
            LIB.T_hostos_read(w, True)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertEqual(int(LIB.T_hostos_profile_calls()), 1)

    def test_no_pre_config_reads_keeps_the_seed(self):
        # A cached re-attach on a cold boot: nothing but driver chatter.
        self.cold_boot()
        self.enumerate([], CACHED_MAC_POST)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertIn("verdict none", self.status())

    def test_configured_before_boot_is_still_evaluated(self):
        # The host configures the device while the split halves are still
        # electing a master; the edge lands before keyboard_post_init_user.
        LIB.T_hostos_forget(False)
        self.enumerate(LINUX_PRE, LINUX_POST)
        LIB.T_hostos_boot()
        self.assertEqual(int(LIB.T_hostos_active()), PC)

    # -- warm boots (the keyboard reset itself; same host) --------------

    def test_warm_boot_seeds_the_remembered_profile(self):
        self.cold_boot()
        self.enumerate(LINUX_PRE, LINUX_POST)
        self.warm_boot()
        self.assertEqual(int(LIB.T_hostos_active()), PC)
        self.assertEqual(int(LIB.T_hostos_last_profile()), PC)

    def test_warm_boot_cached_mac_reattach_keeps_mac(self):
        # THE bug: after the keyboard's own soft reset, macOS re-attaches
        # from cache with only 0xFF reads -- Linux's signature.
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.warm_boot()
        self.enumerate([0xFF, 0xFF, 0xFF], [0xFF])
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertEqual(int(LIB.T_hostos_last_profile()), MAC)
        self.assertIn("verdict linux", self.status())

    def test_warm_boot_cached_reattach_post_config_only_keeps_mac(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.warm_boot()
        self.enumerate([], CACHED_MAC_POST)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)

    def test_warm_boot_positive_windows_fingerprint_moves_the_profile(self):
        # A KVM that keeps the keyboard powered across a host switch: the
        # new host's real enumeration must still win.
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.warm_boot()
        self.enumerate(WINDOWS_PRE, WINDOWS_POST)
        self.assertEqual(int(LIB.T_hostos_active()), PC)

    def test_warm_boot_positive_mac_fingerprint_moves_the_profile(self):
        self.cold_boot()
        self.enumerate(LINUX_PRE, LINUX_POST)
        self.warm_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.assertEqual(int(LIB.T_hostos_active()), MAC)

    def test_warm_boot_linux_pattern_keeps_pc_too(self):
        self.cold_boot()
        self.enumerate(WINDOWS_PRE, WINDOWS_POST)
        self.warm_boot()
        self.enumerate(LINUX_PRE, LINUX_POST)
        self.assertEqual(int(LIB.T_hostos_active()), PC)

    def test_power_cycle_forgets_and_trusts_linux_again(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.cold_boot()
        self.enumerate(LINUX_PRE, LINUX_POST)
        self.assertEqual(int(LIB.T_hostos_active()), PC)

    # -- re-enumeration without a reset ---------------------------------

    def test_bus_reset_starts_a_fresh_enumeration(self):
        # Reads while unconfigured after having been configured belong to
        # a new enumeration; the old pairs must not leak into it.
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        LIB.T_hostos_usb(False)
        self.enumerate(WINDOWS_PRE, WINDOWS_POST)
        self.assertEqual(int(LIB.T_hostos_active()), PC)
        self.assertIn("verdict windows", self.status())

    def test_resume_without_reads_is_idempotent(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        LIB.T_hostos_usb(False)  # suspend
        LIB.T_hostos_usb(True)  # resume, no new reads
        self.assertEqual(int(LIB.T_hostos_active()), MAC)
        self.assertEqual(int(LIB.T_hostos_profile_calls()), 1)

    def test_repeated_configured_notifications_are_one_edge(self):
        # LED / idle-rate notifications repeat the configured state.
        self.cold_boot()
        self.enumerate(LINUX_PRE, LINUX_POST)
        LIB.T_hostos_usb(True)
        LIB.T_hostos_usb(True)
        self.assertEqual(int(LIB.T_hostos_profile_calls()), 2)  # seed + pc

    # -- diagnostics ----------------------------------------------------

    def test_status_line_reports_boot_kind_and_reads(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        line = self.status()
        self.assertTrue(line.startswith("hostos: cold boot #0, active mac, verdict mac"))
        self.assertIn("pre 6: 02x3 04x0 ffx0; post 2", line)
        self.assertIn("reads: 02 4E 02 1C 02 1A | FF FF", line)

    def test_status_line_carries_the_previous_boot(self):
        self.cold_boot()
        self.enumerate(MAC_SEQUOIA_PRE, MAC_SEQUOIA_POST)
        self.warm_boot()
        self.enumerate([0xFF, 0xFF, 0xFF])
        line = self.status()
        self.assertTrue(line.startswith("hostos: warm boot #1, active mac, verdict linux"))
        self.assertIn("prev mac (6/2)", line)
        self.assertIn("reads: FF FF FF", line)

    def test_status_line_fits_a_small_buffer(self):
        self.cold_boot()
        self.enumerate(WINDOWS_PRE, WINDOWS_POST)
        buf = ctypes.create_string_buffer(64)
        n = int(LIB.T_hostos_status(buf, 64))
        self.assertLessEqual(n, 63)
        self.assertEqual(len(buf.value), n)


if __name__ == "__main__":
    unittest.main()
