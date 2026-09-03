# Values crossing back from C through ctypes are necessarily untyped, and the
# recorded-event struct lives in a git submodule that no static analyser can
# follow. Those four checks are switched off for this file alone rather than
# repo-wide; everything past the FFI boundary is converted to a typed Event
# below, so the rest of the module still type-checks normally.
# pyright: reportAny=false, reportUnknownVariableType=false
# pyright: reportImplicitOverride=false, reportMissingImports=false
# pyright: reportUnknownMemberType=false, reportUnknownArgumentType=false
"""Host tests for the MB_* dual-role mouse/modifier engine in townk_mouse.c.

Compiles the real users/townk/townk_mouse.c into a shared library (see
tests/townk_mouse_layout.c) and drives it directly, asserting against the event
history recorded by sm_td's test shim. Runs in milliseconds -- no firmware
build, no flashing.

    python3 tests/run_tests.py
"""

import ctypes
import os
import subprocess
import sys
import unittest
from typing import NamedTuple

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
SUBMODULE = os.path.join(REPO, "modules", "stasmarkin")

# The recorded-event struct comes from the vendored shim rather than being
# retyped here, so a change to it upstream surfaces as a test failure instead of
# silently misreading memory.
sys.path.insert(0, os.path.join(SUBMODULE, "tests", "unit"))
from sm_td_bindings import CHistory  # noqa: E402

MAX_HISTORY = 100

# MOD_BIT() masks the keycode with 0x07, so these match the firmware's bits.
MOD_LCTL = 1 << 0
MOD_LSFT = 1 << 1
MOD_LALT = 1 << 2
MOD_LGUI = 1 << 3
MOD_MASK_SHIFT = (1 << 1) | (1 << 5)  # MOD_BIT(KC_LSFT) | MOD_BIT(KC_RSFT)


class Event(NamedTuple):
    """One recorded register/unregister, with the mods held at the time."""

    keycode: int
    pressed: bool
    mods: int


def _build() -> ctypes.CDLL:
    """Compile the fixture into a shared library and load it."""
    src = os.path.join(REPO, "tests", "townk_mouse_layout.c")
    ext = ".dylib" if sys.platform == "darwin" else ".so"
    lib_path = os.path.join(REPO, "tests", "libtownk_mouse" + ext)

    cmd = [
        "clang", "-shared", "-o", lib_path, "-fPIC", src,
        "-I" + SUBMODULE,
        "-I" + os.path.join(SUBMODULE, "sm_td"),  # townk_smtd.c includes "sm_td.h"
        "-I" + os.path.join(REPO, "tests", "stubs"),
        "-DSMTD_UNIT_TEST",
        "-std=c11",
        # -Werror on purpose: a warning in this firmware is a defect, and the
        # host build is the cheapest place to catch one.
        "-Wall", "-Wextra", "-Werror",
        "-Wno-sign-compare", "-Wno-missing-braces", "-Wno-unused-parameter",
    ]

    result = subprocess.run(cmd, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise RuntimeError(
            "failed to compile the test fixture:\n" + result.stderr.decode()
        )

    lib = ctypes.CDLL(lib_path)

    lib.T_key.argtypes = [ctypes.c_uint16, ctypes.c_bool]
    lib.T_smtd_touch.argtypes = [ctypes.c_uint16]
    lib.T_smtd_tap.argtypes = [ctypes.c_uint16, ctypes.c_uint8]
    lib.T_smtd_hold.argtypes = [ctypes.c_uint16, ctypes.c_uint8]
    lib.T_smtd_release.argtypes = [ctypes.c_uint16, ctypes.c_uint8]
    # x/y are 16-bit (MOUSE_EXTENDED_REPORT, as on the Svalboard); h/v are not.
    lib.T_pointing.argtypes = [ctypes.c_int16, ctypes.c_int16,
                               ctypes.c_int8, ctypes.c_int8]
    lib.T_set_external_mods.argtypes = [ctypes.c_uint8]
    lib.T_reset.argtypes = []
    lib.T_hold_backspace.argtypes = [ctypes.c_bool]
    lib.T_mouse_layer.argtypes = [ctypes.c_bool]
    lib.T_layer_is.argtypes = [ctypes.c_uint8]
    lib.T_layer_is.restype = ctypes.c_bool
    lib.T_layer_nav.restype = ctypes.c_uint8
    lib.T_layer_mbo.restype = ctypes.c_uint8
    lib.T_mods_acquire.argtypes = [ctypes.c_uint8]
    lib.T_mods_release.argtypes = [ctypes.c_uint8]
    lib.T_mods_claims.argtypes = [ctypes.c_uint8]
    lib.T_mods_claims.restype = ctypes.c_uint8
    lib.T_mouse_mode_calls.restype = ctypes.c_int
    lib.T_mouse_mode_state.restype = ctypes.c_bool
    lib.TEST_reset.argtypes = []
    lib.TEST_advance_time.argtypes = [ctypes.c_uint32]
    # The fixture's one-shot model, driven directly: SMART_SHIFT's tap is
    # set_oneshot_mods(MOD_LSFT), so a test can stand in for that tap.
    lib.set_oneshot_mods.argtypes = [ctypes.c_uint8]
    lib.get_oneshot_mods.restype = ctypes.c_uint8
    # Layer moves route through the REAL layer_state_set_user (townk_layers.c
    # is compiled into the fixture), which is how the game-layer tests drive
    # the code under test the same way a TO(_GAM1) keypress would.
    lib.layer_move.argtypes = [ctypes.c_uint8]
    lib.T_layer_gam1.restype = ctypes.c_uint8
    lib.T_layer_base.restype = ctypes.c_uint8
    lib.T_auto_mouse.restype = ctypes.c_bool
    lib.T_set_auto_mouse.argtypes = [ctypes.c_bool]
    lib.T_mouse_mode_saw_auto_mouse.restype = ctypes.c_bool
    # TEST_get_record_history deliberately has no argtypes: ctypes already
    # passes an array and a byref() correctly, and declaring them would mean
    # ctypes.POINTER(), which is deprecated.
    lib.get_mods.restype = ctypes.c_uint8

    # The switcher-chord housekeeping, as housekeeping_task_user() would run it.
    lib.T_switcher_task.argtypes = []
    # The switcher's process_record seam, called for every plain key event.
    lib.switcher_process_record.argtypes = [ctypes.c_uint16, ctypes.c_bool]
    lib.switcher_process_record.restype = ctypes.c_bool
    # The LED idle engine and the fixture's model of QMK input-activity time.
    lib.led_idle_task.argtypes = []
    lib.T_set_input_idle_ms.argtypes = [ctypes.c_uint32]
    lib.T_rgb_enabled.restype = ctypes.c_bool
    lib.T_rgb_enable_calls.restype = ctypes.c_int
    lib.T_rgb_disable_calls.restype = ctypes.c_int
    # The host-profile detector (townk_hostos.c), driven event by event.
    lib.T_hostos_boot.argtypes = []
    lib.T_hostos_forget.argtypes = [ctypes.c_bool]
    lib.T_hostos_read.argtypes = [ctypes.c_uint16, ctypes.c_bool]
    lib.T_hostos_usb.argtypes = [ctypes.c_bool]
    lib.T_hostos_active.restype = ctypes.c_uint8
    lib.T_hostos_classify.argtypes = [ctypes.c_uint8] * 4
    lib.T_hostos_classify.restype = ctypes.c_uint8
    lib.T_hostos_profile_calls.restype = ctypes.c_int
    lib.T_hostos_last_profile.restype = ctypes.c_uint8
    lib.T_hostos_status.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
    lib.T_hostos_status.restype = ctypes.c_int
    for name in ("T_hostos_mac", "T_hostos_pc", "T_hostos_unknown",
                 "T_hostos_v_none", "T_hostos_v_mac", "T_hostos_v_windows",
                 "T_hostos_v_linux"):
        getattr(lib, name).restype = ctypes.c_uint8

    for name in ("T_kc_mb_sft", "T_kc_mb_alt", "T_kc_mb_gui", "T_kc_mb_ctl",
                 "T_kc_ckc_spc", "T_kc_ckc_bspc", "T_kc_btn1", "T_kc_btn2",
                 "T_kc_btn3", "T_kc_del", "T_kc_bspc", "T_kc_plain",
                 "T_kc_ckc_tab", "T_kc_ckc_bktab", "T_kc_tab", "T_kc_bktab",
                 "T_kc_spc", "T_kc_grv", "T_kc_bkgrv", "T_kc_b"):
        getattr(lib, name).restype = ctypes.c_uint16

    return lib


LIB = _build()

MB_GUI: int = int(LIB.T_kc_mb_gui())
MB_SFT: int = int(LIB.T_kc_mb_sft())
MB_ALT: int = int(LIB.T_kc_mb_alt())
CKC_SPC: int = int(LIB.T_kc_ckc_spc())
CKC_BSPC: int = int(LIB.T_kc_ckc_bspc())
KC_DEL: int = int(LIB.T_kc_del())
KC_BSPC: int = int(LIB.T_kc_bspc())
MOD_RSFT = 1 << 5  # MOD_BIT(KC_RSFT); R1 double-south is a plain KC_RIGHT_SHIFT
KC_BTN1: int = int(LIB.T_kc_btn1())
KC_BTN2: int = int(LIB.T_kc_btn2())
KC_BTN3: int = int(LIB.T_kc_btn3())
KC_PLAIN: int = int(LIB.T_kc_plain())

CKC_TAB: int = int(LIB.T_kc_ckc_tab())
CKC_BKTAB: int = int(LIB.T_kc_ckc_bktab())
KC_TAB: int = int(LIB.T_kc_tab())
KC_BKTAB: int = int(LIB.T_kc_bktab())  # S(KC_TAB), the 16-bit shifted keycode
KC_SPC: int = int(LIB.T_kc_spc())
KC_GRV: int = int(LIB.T_kc_grv())
KC_BKGRV: int = int(LIB.T_kc_bkgrv())  # S(KC_GRV), the 16-bit shifted keycode
KC_B: int = int(LIB.T_kc_b())
MOUSE_BUTTONS = (KC_BTN1, KC_BTN2, KC_BTN3)
LAYER_NAV: int = int(LIB.T_layer_nav())
LAYER_MBO: int = int(LIB.T_layer_mbo())
LAYER_GAM1: int = int(LIB.T_layer_gam1())
LAYER_BASE: int = int(LIB.T_layer_base())


class TownkMouseTest(unittest.TestCase):
    def setUp(self) -> None:
        LIB.TEST_reset()  # sm_td state + recorded history
        LIB.T_reset()     # mb_states, mouse-mode counters, mods

    def tearDown(self) -> None:
        # Every path must leave the modifier state clean. A leaked modifier is
        # its own class of bug -- the user feels it as a keyboard that has
        # "gone weird" until some unrelated key clears it.
        self.assertEqual(int(LIB.get_mods()), 0, "modifiers leaked past key release")

    # -- helpers ----------------------------------------------------------

    def history(self) -> list[Event]:
        """Every event the engine emitted since the last reset."""
        records = (CHistory * MAX_HISTORY)()
        count = ctypes.c_uint8()
        LIB.TEST_get_record_history(records, ctypes.byref(count))
        return [
            Event(
                keycode=int(records[i].keycode),
                pressed=bool(records[i].pressed),
                mods=int(records[i].mods),
            )
            for i in range(count.value)
        ]

    def mods(self) -> int:
        return int(LIB.get_mods())

    def assertNoMouseButton(self, msg: str) -> None:
        for event in self.history():
            self.assertNotIn(
                event.keycode, MOUSE_BUTTONS, f"{msg}: unexpected {event}"
            )

    # -- the role each key resolves to ------------------------------------

    def test_tap_alone_sends_a_mouse_button(self) -> None:
        """Pressed and released with nothing else: a mouse click."""
        LIB.T_key(MB_GUI, True)
        LIB.T_key(MB_GUI, False)

        self.assertEqual(
            self.history(),
            [
                Event(KC_BTN3, pressed=True, mods=0),
                Event(KC_BTN3, pressed=False, mods=0),
            ],
        )

    def test_press_alone_commits_to_nothing(self) -> None:
        """The key is a BUTTON first: pressing it must claim no modifier.

        The modifier is the exceptional role and has to be earned by something
        happening afterwards. Registering it up-front is what forced the old
        design to prove a negative at release time -- "nothing contradicted
        me" -- which it could only do by observing every other keypress, and
        it could not observe the ones SM_TD consumed.
        """
        LIB.T_key(MB_GUI, True)

        self.assertEqual(self.mods(), 0, "no modifier may be claimed on press")
        self.assertEqual(self.history(), [], "and nothing may be emitted yet")

        LIB.T_key(MB_GUI, False)

    def test_ordinary_key_makes_it_a_modifier(self) -> None:
        """Another key pressed while held: a modifier, and NO click."""
        LIB.T_key(MB_GUI, True)

        LIB.T_key(KC_PLAIN, True)
        self.assertEqual(
            self.mods(), MOD_LGUI,
            "Cmd must be held by the time the other key is emitted",
        )

        LIB.T_key(KC_PLAIN, False)
        LIB.T_key(MB_GUI, False)

        self.assertNoMouseButton("a key used as a modifier must not also click")

    def test_smtd_key_makes_it_a_modifier(self) -> None:
        """Regression test for 3b82f67 -- the phantom middle click.

        An SM_TD key (CKC_SPC) reaches on_smtd_action() and never
        process_record_user(), so before the fix the held MB_* key never
        learned it had been used as a modifier and fired a bare mouse-button
        tap on release. That stray click was a real click to the OS: it pasted
        in any app bound to middle-click paste, and opened context menus over
        the Dock.
        """
        LIB.T_key(MB_GUI, True)

        LIB.T_smtd_touch(CKC_SPC)  # Cmd+Space -- never reaches process_record_user
        self.assertEqual(
            self.mods(), MOD_LGUI, "Cmd must be held for the Space that follows"
        )

        LIB.T_key(MB_GUI, False)

        self.assertNoMouseButton("Cmd+Space must not emit a phantom click")
        self.assertEqual(self.history(), [], "no events at all should be emitted")

    def test_pointer_movement_converts_to_a_held_button(self) -> None:
        """Trackball motion while held: becomes a held button, for dragging."""
        LIB.T_key(MB_GUI, True)
        LIB.T_pointing(5, 5, 0, 0)

        self.assertEqual(self.mods(), 0, "the modifier must be given up")
        self.assertEqual(self.history(), [Event(KC_BTN3, pressed=True, mods=0)])

        LIB.T_key(MB_GUI, False)
        self.assertEqual(self.history()[-1], Event(KC_BTN3, pressed=False, mods=0))

    def test_fast_flick_in_one_extended_report_converts(self) -> None:
        """A single 256-count report is a deliberate drag, not silence.

        The Svalboard defines MOUSE_EXTENDED_REPORT, so a report's x/y are
        int16_t and a fast flick (or a delayed task tick accumulating PMW3389
        counts) can arrive as one large report. Narrowing it to int8_t on the
        way into the motion test folds 256 to 0 -- a real drag reads as no
        motion at all, and the accumulated distance of sub-multiples is
        miscounted (200 becomes -56).
        """
        LIB.T_key(MB_GUI, True)
        LIB.T_pointing(256, 0, 0, 0)

        self.assertEqual(
            self.history(), [Event(KC_BTN3, pressed=True, mods=0)],
            "a 256-count report is far past the threshold and must convert",
        )

        LIB.T_key(MB_GUI, False)

    def test_resting_jitter_does_not_convert(self) -> None:
        """A one-count blip is a resting hand, not a drag.

        The hand resting on the ball emits sparse tiny reports. Before the
        motion threshold, each one converted a held key to its button on the
        spot: holding Cmd while touching the ball produced a middle click.
        The key must stay undecided, so releasing it is still a plain click.
        """
        LIB.T_key(MB_GUI, True)
        LIB.T_pointing(1, 0, 0, 0)

        self.assertNoMouseButton("a jitter blip must not press a button")
        self.assertEqual(self.mods(), 0, "nor claim the modifier")

        LIB.T_key(MB_GUI, False)
        self.assertEqual(
            self.history(),
            [
                Event(KC_BTN3, pressed=True, mods=0),
                Event(KC_BTN3, pressed=False, mods=0),
            ],
            "released with nothing earned, the key is still a click",
        )

    def test_sparse_jitter_never_accumulates(self) -> None:
        """Blips separated by quiet gaps reset the accumulator.

        Four two-count blips total 8 -- enough to cross the threshold if they
        merely added up. They must not: a quiet gap longer than
        MB_MOVE_RESET_MS forgets the distance, so resting on the ball for a
        long time is no closer to a drag than touching it once.
        """
        LIB.T_key(MB_GUI, True)
        for _ in range(4):
            LIB.T_pointing(2, 0, 0, 0)
            LIB.TEST_advance_time(100)  # > MB_MOVE_RESET_MS: goes quiet

        self.assertNoMouseButton("sparse jitter must never become a drag")

        LIB.T_key(MB_GUI, False)

    def test_slow_dense_motion_still_converts(self) -> None:
        """A slow but continuous drag crosses the threshold and converts.

        Per-report distance depends on CPI and polling rate, so a deliberate
        precision drag may only produce a few counts per report. What makes
        it deliberate is density: back-to-back reports with no quiet gap.
        """
        LIB.T_key(MB_GUI, True)
        LIB.T_pointing(3, 0, 0, 0)
        LIB.T_pointing(3, 0, 0, 0)
        self.assertNoMouseButton("6 counts is still below the threshold")

        LIB.T_pointing(3, 0, 0, 0)  # 9 counts: deliberate
        self.assertEqual(self.history(), [Event(KC_BTN3, pressed=True, mods=0)])

        LIB.T_key(MB_GUI, False)
        self.assertEqual(self.history()[-1], Event(KC_BTN3, pressed=False, mods=0))

    def test_scroll_with_motion_noise_is_a_scroll(self) -> None:
        """Sub-threshold x/y riding on a scroll report resolves as scroll.

        Motion only outranks scroll once it is deliberate. A wiggle of the
        resting hand during Cmd+scroll must keep the zoom, not turn it into
        a middle-button drag.
        """
        LIB.T_key(MB_GUI, True)
        LIB.T_pointing(1, 0, 3, 3)

        self.assertEqual(self.mods(), MOD_LGUI, "the scroll claims the modifier")
        self.assertNoMouseButton("the wiggle must not press a button")

        LIB.T_key(MB_GUI, False)
        self.assertNoMouseButton("releasing after the scroll must not click")

    def test_external_modifier_makes_it_a_button(self) -> None:
        """Pressed while another modifier is already down: a mouse button."""
        LIB.T_set_external_mods(MOD_LSFT)

        LIB.T_key(MB_GUI, True)
        self.assertEqual(
            self.history(), [Event(KC_BTN3, pressed=True, mods=MOD_LSFT)]
        )

        LIB.T_key(MB_GUI, False)
        LIB.T_set_external_mods(0)  # release the stand-in for the real Shift key

    def test_hold_alone_then_release_still_clicks(self) -> None:
        """Duration alone decides nothing: a slow, deliberate click still clicks.

        There is no tapping term here on purpose. Only a competing signal --
        motion, scroll, another key -- can take the button role away, so a
        held-then-released key with nothing else happening is a click no
        matter how long it was held.
        """
        LIB.T_key(MB_GUI, True)
        LIB.T_key(MB_GUI, False)

        self.assertEqual(
            self.history(),
            [
                Event(KC_BTN3, pressed=True, mods=0),
                Event(KC_BTN3, pressed=False, mods=0),
            ],
        )

    def test_scroll_resolves_to_a_modifier(self) -> None:
        """Scrolling while held: a modifier, so Cmd+scroll stays zoom.

        Deliberately the opposite of pointer motion. Treating scroll as
        movement would turn every Cmd+scroll-to-zoom into a middle-button
        drag; leaving it unresolved would fire a stray click on release, which
        is the same defect class as the phantom click on Cmd+Space.
        """
        LIB.T_key(MB_GUI, True)
        LIB.T_pointing(0, 0, 3, 3)

        self.assertEqual(self.mods(), MOD_LGUI, "scroll must claim the modifier")
        self.assertEqual(self.history(), [], "and must not press a button")

        LIB.T_key(MB_GUI, False)
        self.assertNoMouseButton("releasing after a scroll must not click")

    def test_two_modifiers_then_click(self) -> None:
        """Cmd+Option+click: two keys held as modifiers, a third clicks."""
        LIB.T_key(MB_GUI, True)
        LIB.T_key(MB_ALT, True)   # resolves MB_GUI -> Cmd
        LIB.T_key(MB_SFT, True)   # resolves MB_ALT -> Option
        self.assertEqual(
            self.mods(), MOD_LGUI | MOD_LALT, "both modifiers must be held"
        )

        LIB.T_key(MB_SFT, False)
        self.assertEqual(
            self.history(),
            [
                Event(KC_BTN1, pressed=True, mods=MOD_LGUI | MOD_LALT),
                Event(KC_BTN1, pressed=False, mods=MOD_LGUI | MOD_LALT),
            ],
            "the click must carry both modifiers",
        )

        LIB.T_key(MB_ALT, False)
        LIB.T_key(MB_GUI, False)

    def test_two_modifiers_then_drag(self) -> None:
        """Cmd+Option+drag: the same, with motion instead of a tap."""
        LIB.T_key(MB_GUI, True)
        LIB.T_key(MB_ALT, True)
        LIB.T_key(MB_SFT, True)

        LIB.T_pointing(5, 5, 0, 0)
        self.assertEqual(
            self.mods(), MOD_LGUI | MOD_LALT,
            "motion must not strip the modifiers off the other keys",
        )
        self.assertEqual(
            self.history(),
            [Event(KC_BTN1, pressed=True, mods=MOD_LGUI | MOD_LALT)],
        )

        LIB.T_key(MB_SFT, False)
        LIB.T_key(MB_ALT, False)
        LIB.T_key(MB_GUI, False)

    # -- the shift-inverted Backspace / Delete thumb pad -------------------

    def test_plain_tap_sends_backspace(self) -> None:
        """No shift: the pad is Backspace."""
        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        self.assertEqual(
            [e.keycode for e in self.history()], [KC_BSPC, KC_BSPC]
        )

    def test_shift_tap_sends_forward_delete(self) -> None:
        """Shift+tap is forward Delete, and the Shift must NOT reach it.

        The user's Shift is R1 double-south, a plain KC_RIGHT_SHIFT. The
        inversion has to strip it before tapping KC_DEL, or the host receives
        Shift+Delete instead of Delete -- which most apps ignore, looking
        exactly like "forward delete does nothing".
        """
        LIB.T_set_external_mods(MOD_RSFT)

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        emitted = [e for e in self.history() if e.keycode in (KC_DEL, KC_BSPC)]
        self.assertTrue(emitted, "something must be emitted")
        self.assertEqual(
            emitted[0].keycode, KC_DEL, "Shift+tap must send forward Delete"
        )
        self.assertEqual(
            emitted[0].mods & MOD_MASK_SHIFT, 0,
            f"Delete must go out WITHOUT shift, got {emitted[0]}",
        )

        LIB.T_set_external_mods(0)

    def test_oneshot_shift_tap_sends_delete_and_consumes_the_oneshot(self) -> None:
        """Smart Shift tap then pad tap: Delete, with the one-shot spent.

        The one-shot shift set by a Smart Shift tap counts as shift for the
        inversion (mods reads get_mods() | get_oneshot_mods()), so the pad
        must send forward Delete. But the one-shot is PENDING, not held:
        unregister_mods() cannot consume it, and re-registering it as a real
        modifier afterwards would leave a Shift no key release ever clears.
        The inversion has to spend the one-shot itself -- the Delete IS the
        "next key" the one-shot was armed for.
        """
        LIB.set_oneshot_mods(MOD_LSFT)  # a Smart Shift tap

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        emitted = [e for e in self.history() if e.keycode in (KC_DEL, KC_BSPC)]
        self.assertTrue(emitted, "something must be emitted")
        self.assertEqual(
            emitted[0].keycode, KC_DEL, "one-shot shift must invert the pad too"
        )
        self.assertEqual(
            int(LIB.get_oneshot_mods()), 0,
            "the one-shot must be consumed by the inverted tap",
        )
        self.assertEqual(
            self.mods() & MOD_MASK_SHIFT, 0,
            "a one-shot must not become a real shift: nothing would release it",
        )

    def test_ctrl_shift_tap_sends_ctrl_delete(self) -> None:
        """Ctrl+Shift+Backspace is how you reach Ctrl+Delete.

        The inversion consumes ONLY shift -- unregister_mods(MOD_MASK_SHIFT) --
        so every other modifier you are holding rides along to the KC_DEL.
        That makes Ctrl+Delete (and Alt+Delete, Cmd+Delete) reachable by adding
        the modifier to the Shift+Backspace gesture, with no extra firmware
        support needed.
        """
        LIB.T_set_external_mods(MOD_LCTL | MOD_RSFT)

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        deletes = [e for e in self.history() if e.keycode == KC_DEL]
        self.assertTrue(deletes, "a Delete must be emitted")
        self.assertEqual(
            deletes[0].mods & MOD_MASK_SHIFT, 0, "shift is consumed by the inversion"
        )
        self.assertEqual(
            deletes[0].mods & MOD_LCTL, MOD_LCTL,
            f"but Ctrl must survive onto the Delete, got {deletes[0]}",
        )

        LIB.T_set_external_mods(0)

    def test_holding_the_pad_keeps_the_mouse_layer(self) -> None:
        """Holding the pad must not take the mouse layer away.

        It used to. `mouse_mode(false)` turned _MBO off, and LAYER_PUSH is
        `layer_move()`, which REPLACES the layer state rather than adding to
        it -- either alone left no MB_* key on any active layer while the pad
        was held. So the pad could not contribute Option to a click, because
        no click was reachable to contribute to. The feature was unreachable
        by construction, and no test at the engine seam could see it: the
        engine is driven directly here, bypassing keymap and layer lookup.
        """
        LIB.T_mouse_layer(True)  # trackball activity raised _MBO
        self.assertTrue(LIB.T_layer_is(LAYER_MBO))

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_hold(CKC_BSPC, 0)

        self.assertTrue(LIB.T_layer_is(LAYER_NAV), "the pad must raise _NAV")
        self.assertTrue(
            LIB.T_layer_is(LAYER_MBO),
            "and the mouse layer must survive, or there is nothing to click",
        )

        LIB.T_smtd_release(CKC_BSPC, 0)
        self.assertFalse(LIB.T_layer_is(LAYER_NAV), "_NAV goes away on release")

        LIB.T_mouse_layer(False)

    def test_holding_the_pad_gives_nav_not_delete(self) -> None:
        """A press long enough to resolve as HOLD is the layer, not a Delete.

        Documents the trap rather than a defect: SM_TD picks TAP or HOLD by
        timing, and the hold branch is `mouse_mode(false); LAYER_PUSH(_NAV)`.
        So a *deliberate*, slightly slow Shift+Backspace -- a two-handed
        gesture, so easily slower than a reflexive Backspace -- silently
        becomes "activate the navigation layer" and emits nothing at all.
        """
        LIB.T_set_external_mods(MOD_RSFT)

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_hold(CKC_BSPC, 0)

        self.assertNotIn(
            KC_DEL, [e.keycode for e in self.history()],
            "the hold branch emits no Delete -- it raises the layer instead",
        )

        LIB.T_smtd_release(CKC_BSPC, 0)
        LIB.T_set_external_mods(0)

    def test_shift_is_restored_after_the_delete(self) -> None:
        """The user is still holding Shift; it must come back afterwards."""
        LIB.T_set_external_mods(MOD_RSFT)

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        self.assertEqual(
            self.mods() & MOD_MASK_SHIFT, MOD_RSFT,
            "shift must be re-registered after the inverted tap",
        )

        LIB.T_set_external_mods(0)

    # -- modifiers contributed to a click by a held key -------------------

    def test_held_backspace_contributes_alt_to_a_click(self) -> None:
        """Hold the left thumb pad, click: the click carries Option."""
        LIB.T_hold_backspace(True)

        LIB.T_key(MB_SFT, True)
        LIB.T_key(MB_SFT, False)

        self.assertEqual(
            self.history(),
            [
                Event(KC_BTN1, pressed=True, mods=MOD_LALT),
                Event(KC_BTN1, pressed=False, mods=MOD_LALT),
            ],
            "the click must carry Option contributed by the held key",
        )

        LIB.T_hold_backspace(False)

    def test_contributed_modifier_does_not_outlive_the_click(self) -> None:
        """Option is scoped to the click, not to how long the key is held.

        If it stayed on for as long as the pad was down it would leak into
        _NAV, silently turning every navigation keypress into Option+key --
        Option+arrow being word-jump rather than character-move.
        """
        LIB.T_hold_backspace(True)

        LIB.T_key(MB_SFT, True)
        LIB.T_key(MB_SFT, False)
        self.assertEqual(
            self.mods(), 0, "Option must be gone once the click is over"
        )

        LIB.T_hold_backspace(False)

    def test_contributed_modifier_refcounts_against_a_held_key(self) -> None:
        """The case reference counting exists for.

        MB_ALT is acting as Option while the thumb pad also contributes
        Option to a click. When the click ends, Option must stay held --
        MB_ALT still wants it. A bare unregister_mods() here would drop the
        modifier out from under a key the user is still holding.
        """
        LIB.T_key(MB_ALT, True)
        LIB.T_key(KC_PLAIN, True)   # resolves MB_ALT to its modifier role
        LIB.T_key(KC_PLAIN, False)
        self.assertEqual(self.mods(), MOD_LALT, "MB_ALT holds Option")

        LIB.T_hold_backspace(True)
        LIB.T_key(MB_SFT, True)
        LIB.T_key(MB_SFT, False)    # a click that also contributes Option

        self.assertEqual(
            self.mods(), MOD_LALT,
            "Option must survive the click -- MB_ALT is still holding it",
        )

        LIB.T_hold_backspace(False)
        LIB.T_key(MB_ALT, False)
        self.assertEqual(self.mods(), 0, "and released once MB_ALT lets go")

    def test_drag_carries_the_contributed_modifier(self) -> None:
        """Option+drag: the modifier is live for the whole gesture."""
        LIB.T_hold_backspace(True)

        LIB.T_key(MB_SFT, True)
        LIB.T_pointing(5, 5, 0, 0)
        self.assertEqual(
            self.history(), [Event(KC_BTN1, pressed=True, mods=MOD_LALT)]
        )

        LIB.T_key(MB_SFT, False)
        self.assertEqual(
            self.history()[-1], Event(KC_BTN1, pressed=False, mods=MOD_LALT),
            "the drop must still carry Option",
        )
        self.assertEqual(self.mods(), 0, "and Option is released after it")

        LIB.T_hold_backspace(False)

    # -- reference-counted modifier ownership -----------------------------

    def test_two_claims_need_two_releases(self) -> None:
        """A modifier stays held until the LAST owner gives it up.

        Once a held key can contribute a modifier to a click, two things can
        want the same bit at once -- MB_ALT acting as Alt while CKC_BSPC also
        contributes Alt. Whoever finishes first must not switch it off under
        the other, which a bare unregister_mods() would do silently.
        """
        LIB.T_mods_acquire(MOD_LALT)
        self.assertEqual(self.mods(), MOD_LALT)

        LIB.T_mods_acquire(MOD_LALT)
        self.assertEqual(int(LIB.T_mods_claims(MOD_LALT)), 2)

        LIB.T_mods_release(MOD_LALT)
        self.assertEqual(
            self.mods(), MOD_LALT, "still wanted by the second owner"
        )

        LIB.T_mods_release(MOD_LALT)
        self.assertEqual(self.mods(), 0, "last owner gone, modifier released")

    def test_claims_are_tracked_per_modifier(self) -> None:
        """Claims on one modifier must not affect another."""
        LIB.T_mods_acquire(MOD_LALT | MOD_LGUI)
        LIB.T_mods_acquire(MOD_LALT)

        LIB.T_mods_release(MOD_LALT)
        self.assertEqual(
            self.mods(), MOD_LALT | MOD_LGUI, "Cmd untouched, Alt still claimed"
        )

        LIB.T_mods_release(MOD_LALT | MOD_LGUI)
        self.assertEqual(self.mods(), 0)

    def test_releasing_an_unclaimed_modifier_is_ignored(self) -> None:
        """A stray release must not steal a modifier somebody else holds."""
        LIB.T_mods_acquire(MOD_LGUI)

        LIB.T_mods_release(MOD_LALT)  # never claimed
        self.assertEqual(self.mods(), MOD_LGUI, "Cmd must survive")

        LIB.T_mods_release(MOD_LGUI)
        self.assertEqual(self.mods(), 0)

    # -- a gesture already in flight --------------------------------------

    def test_key_pressed_during_a_drag_becomes_a_modifier(self) -> None:
        """Finder copy-on-drop: hold Option mid-drag to copy instead of move.

        A drag is already in flight, so a key pressed during it is qualifying
        that drag, not starting a click of its own -- and it has to be live
        immediately, because no further keypress is coming to earn it. The
        drop itself is the next event.

        This is the mirror of the external-modifier rule: a modifier held at
        press time makes the key a button, and a button held at press time
        makes it a modifier.
        """
        LIB.T_key(MB_SFT, True)
        LIB.T_pointing(5, 5, 0, 0)  # motion -> BTN1 held: the drag
        self.assertEqual(self.history(), [Event(KC_BTN1, pressed=True, mods=0)])

        LIB.T_key(MB_ALT, True)
        self.assertEqual(
            self.mods(), MOD_LALT, "Option must be live while the button is down"
        )
        self.assertNotIn(
            KC_BTN2, [e.keycode for e in self.history()],
            "Option must not also press its own mouse button mid-drag",
        )

        # Drop while Option is still held -- that is what makes it a copy.
        LIB.T_key(MB_SFT, False)
        self.assertEqual(
            self.history()[-1], Event(KC_BTN1, pressed=False, mods=MOD_LALT),
            "the drop must carry Option, or Finder moves instead of copying",
        )

        LIB.T_key(MB_ALT, False)

    def test_key_pressed_during_a_held_click_becomes_a_modifier(self) -> None:
        """Same rule when the button is held by external mods rather than motion."""
        LIB.T_set_external_mods(MOD_LSFT)
        LIB.T_key(MB_GUI, True)  # external mods -> immediately a button
        LIB.T_set_external_mods(0)

        LIB.T_key(MB_ALT, True)
        self.assertEqual(self.mods(), MOD_LALT, "Option qualifies the held button")

        LIB.T_key(MB_ALT, False)
        LIB.T_key(MB_GUI, False)

    # -- multiple keys ----------------------------------------------------

    def test_second_mb_key_makes_the_first_a_modifier(self) -> None:
        """Hold MB_GUI, tap MB_SFT: Cmd held, and MB_SFT clicks -> Cmd+click."""
        LIB.T_key(MB_GUI, True)
        LIB.T_key(MB_SFT, True)
        LIB.T_key(MB_SFT, False)

        clicks = [e for e in self.history() if e.keycode == KC_BTN1]
        self.assertTrue(clicks, "the second key should still produce its click")
        self.assertTrue(
            all(e.mods == MOD_LGUI for e in clicks),
            f"the click should carry Cmd, got {clicks}",
        )

        # MB_GUI resolved to a modifier, so its own release must stay silent.
        LIB.T_key(MB_GUI, False)
        self.assertNotIn(
            KC_BTN3, [e.keycode for e in self.history()],
            "MB_GUI acted as a modifier and must not also click",
        )


class TownkLayersTest(unittest.TestCase):
    """The game-layer auto-mouse handling in townk_layers.c.

    Drives the real layer_state_set_user() through the shim's layer_move(),
    the same route a TO(_GAM1) keypress takes on-device.
    """

    def setUp(self) -> None:
        LIB.TEST_reset()
        LIB.T_reset()

    def tearDown(self) -> None:
        LIB.layer_move(LAYER_BASE)  # leave no game state for the next test

    def test_entering_a_game_layer_tears_down_mouse_mode(self) -> None:
        """Game layers kill the pointer: mouse_mode(false), while it can act.

        Svalboard's mouse_mode() body is GATED on
        global_saved_values.auto_mouse -- with the flag already cleared the
        call returns without touching mouse_mode_enabled, the pressed-key
        count, or the _MBO layer. So the teardown must be issued while the
        flag is still up, exactly as keymap_support.c does it ("needs to go
        first to avoid the lockout"). Clearing the flag first turns the
        teardown into a no-op on-device, invisibly: the layer_move to _GAM1
        happens to sweep _MBO away today, which is why this must be pinned
        by a test rather than by symptom.
        """
        LIB.layer_move(LAYER_GAM1)

        self.assertGreater(int(LIB.T_mouse_mode_calls()), 0)
        self.assertFalse(bool(LIB.T_mouse_mode_state()), "must be a teardown")
        self.assertTrue(
            bool(LIB.T_mouse_mode_saw_auto_mouse()),
            "mouse_mode(false) must run BEFORE auto_mouse is cleared",
        )
        self.assertFalse(bool(LIB.T_auto_mouse()), "and then the flag goes down")

    def test_leaving_a_game_layer_restores_the_saved_preference(self) -> None:
        """A user who disabled auto-mouse must find it still disabled.

        auto_mouse is a PERSISTED preference (SV_TOGGLE_AUTOMOUSE writes it
        to EEPROM). Game layers borrow the flag; they do not own it. On the
        way out it must be restored to what the user had, not force-enabled.
        """
        LIB.T_set_auto_mouse(False)  # the user's persisted choice

        LIB.layer_move(LAYER_GAM1)
        LIB.layer_move(LAYER_BASE)

        self.assertFalse(
            bool(LIB.T_auto_mouse()),
            "leaving the game layer must restore the preference, not force it on",
        )

    def test_ordinary_layer_changes_leave_the_preference_alone(self) -> None:
        """A thumb-key hold is not permission to rewrite a saved setting.

        Every layer change runs layer_state_set_user(). If each non-game
        change force-enables auto_mouse, the next _NAV hold silently undoes
        SV_TOGGLE_AUTOMOUSE, and a later EEPROM write persists the override.
        """
        LIB.T_set_auto_mouse(False)

        LIB.T_hold_backspace(True)   # _NAV on -- an ordinary layer change
        LIB.T_hold_backspace(False)

        self.assertFalse(
            bool(LIB.T_auto_mouse()),
            "a non-game layer change must not touch auto_mouse",
        )


class SwitcherChordTest(unittest.TestCase):
    """The MOD+Tab switcher chord: each thumb Pad reverses its own Nail.

    Pressing Tab (LT-Nail, CKC_TAB) with Ctrl/Alt/Gui held -- the app-switcher
    or browser-tab gesture -- arms the LEFT pad (CKC_BSPC) to tap Shift+Tab
    for as long as the MOD stays held, so the same hand can cycle backwards.
    Entering with Shift+Tab (RT-Nail, CKC_BKTAB) arms the RIGHT pad (CKC_SPC)
    to tap Tab instead. Trigger-specific by design: only the pad on the hand
    whose nail started the chord changes role; the other keeps its normal tap.
    """

    def setUp(self) -> None:
        LIB.TEST_reset()
        LIB.T_reset()

    def tearDown(self) -> None:
        self.assertEqual(int(LIB.get_mods()), 0, "modifiers leaked past key release")

    def _tap(self, keycode: int) -> None:
        LIB.T_smtd_touch(keycode)
        LIB.T_smtd_tap(keycode, 0)

    def _pressed(self) -> list[Event]:
        records = (CHistory * MAX_HISTORY)()
        count = ctypes.c_uint8()
        LIB.TEST_get_record_history(records, ctypes.byref(count))
        return [
            Event(
                keycode=int(records[i].keycode),
                pressed=bool(records[i].pressed),
                mods=int(records[i].mods),
            )
            for i in range(count.value)
            if records[i].pressed
        ]

    # -- arming the left pad (forward chord) -------------------------------

    def test_cmd_tab_arms_left_pad_to_backtab(self) -> None:
        """Cmd+Tab, Cmd still held: the left pad taps Shift+Tab, not Backspace."""
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_TAB)

        self._tap(CKC_BSPC)

        emitted = [e.keycode for e in self._pressed() if e.keycode != KC_TAB]
        self.assertIn(KC_BKTAB, emitted, "armed pad must tap Shift+Tab")
        self.assertNotIn(KC_BSPC, emitted, "and must NOT tap Backspace")

        LIB.T_set_external_mods(0)

    def test_ctrl_and_alt_also_arm(self) -> None:
        """MOD is any of Ctrl/Alt/Gui -- Ctrl+Tab (browser tabs) included."""
        for mod in (MOD_LCTL, MOD_LALT):
            LIB.TEST_reset()
            LIB.T_reset()
            LIB.T_set_external_mods(mod)
            self._tap(CKC_TAB)

            self._tap(CKC_BSPC)

            emitted = [e.keycode for e in self._pressed() if e.keycode != KC_TAB]
            self.assertIn(
                KC_BKTAB, emitted, f"mod {mod:#x} must arm the pad"
            )

            LIB.T_set_external_mods(0)

    def test_repeated_pad_taps_keep_cycling(self) -> None:
        """The pad's own Shift+Tab must not re-classify the direction."""
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_TAB)

        self._tap(CKC_BSPC)
        self._tap(CKC_BSPC)

        backtabs = [e for e in self._pressed() if e.keycode == KC_BKTAB]
        self.assertEqual(
            len(backtabs), 2, "every tap while armed must cycle backwards"
        )

        LIB.T_set_external_mods(0)

    # -- disarming ---------------------------------------------------------

    def test_releasing_the_mod_disarms(self) -> None:
        """Cmd+Tab, release Cmd, re-hold Cmd: the pad is Backspace again.

        The stale-flag case: the chord ends when its MOD is released, and a
        LATER hold of the same modifier -- for a click, a shortcut -- must not
        resurrect it. Housekeeping observes the release, whatever its source.
        """
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_TAB)

        LIB.T_set_external_mods(0)  # switcher dismissed
        LIB.T_switcher_task()       # the housekeeping loop notices

        LIB.T_set_external_mods(MOD_LGUI)  # Cmd again, for something else
        self._tap(CKC_BSPC)

        emitted = [e.keycode for e in self._pressed() if e.keycode != KC_TAB]
        self.assertIn(KC_BSPC, emitted, "a fresh Cmd hold must not re-arm the pad")
        self.assertNotIn(KC_BKTAB, emitted)

        LIB.T_set_external_mods(0)

    def test_tab_without_mod_never_arms(self) -> None:
        """A bare Tab is just typing; a later Cmd hold must find the pad normal."""
        self._tap(CKC_TAB)

        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_BSPC)

        emitted = [e.keycode for e in self._pressed() if e.keycode != KC_TAB]
        self.assertIn(KC_BSPC, emitted)
        self.assertNotIn(KC_BKTAB, emitted)

        LIB.T_set_external_mods(0)

    def test_shift_only_does_not_arm(self) -> None:
        """Shift alone is not a switcher MOD: Shift+Tab then pad = Delete.

        The pad's existing shift inversion must win -- Shift was held, so the
        tap is forward Delete, exactly as before this feature existed.
        """
        LIB.T_set_external_mods(MOD_RSFT)
        self._tap(CKC_TAB)

        self._tap(CKC_BSPC)

        emitted = [e.keycode for e in self._pressed()]
        self.assertIn(KC_DEL, emitted, "shift inversion must be untouched")
        self.assertNotIn(KC_BKTAB, emitted)

        LIB.T_set_external_mods(0)

    # -- the backward chord and trigger specificity ------------------------

    def test_cmd_backtab_arms_right_pad_to_tab(self) -> None:
        """Entering backwards (Cmd+Shift+Tab): the right pad taps Tab, not Space."""
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_BKTAB)

        self._tap(CKC_SPC)

        emitted = [e.keycode for e in self._pressed() if e.keycode != KC_BKTAB]
        self.assertIn(KC_TAB, emitted, "armed right pad must tap Tab")
        self.assertNotIn(KC_SPC, emitted, "and must NOT tap Space")

        LIB.T_set_external_mods(0)

    def test_forward_chord_leaves_right_pad_alone(self) -> None:
        """Armed by Tab: only the LEFT pad changes; the right still taps Space."""
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_TAB)

        self._tap(CKC_SPC)

        emitted = [e.keycode for e in self._pressed() if e.keycode != KC_TAB]
        self.assertIn(KC_SPC, emitted)

        LIB.T_set_external_mods(0)

    def test_backward_chord_leaves_left_pad_alone(self) -> None:
        """Armed by Shift+Tab: only the RIGHT pad changes; the left is Backspace."""
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_BKTAB)

        self._tap(CKC_BSPC)

        emitted = [
            e.keycode for e in self._pressed() if e.keycode != KC_BKTAB
        ]
        self.assertIn(KC_BSPC, emitted)

        LIB.T_set_external_mods(0)

    # -- the pads' other roles are untouched -------------------------------

    def test_hold_still_opens_nav_while_armed(self) -> None:
        """Only the TAP changes: an armed left pad held down still opens _NAV."""
        LIB.T_set_external_mods(MOD_LGUI)
        self._tap(CKC_TAB)

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_hold(CKC_BSPC, 0)
        self.assertTrue(
            bool(LIB.T_layer_is(LAYER_NAV)), "hold must still be the _NAV layer"
        )
        LIB.T_smtd_release(CKC_BSPC, 0)

        LIB.T_set_external_mods(0)


class GraveChordTest(unittest.TestCase):
    """The MOD+Grave window-switcher chord: L2-East reverses L2-West.

    macOS cycles a single app's windows with Cmd+Grave and reverses with
    Cmd+Shift+Grave. On this keymap Grave is L2-West and B is L2-East, both
    plain keycodes -- so the chord is observed and reversed in the
    process_record path (switcher_process_record()), not in an SM_TD dance.
    While a MOD+Grave chord is held open, pressing B taps Shift+Grave
    instead of typing a b; everything disarms with the modifier, exactly
    like the Tab chord.
    """

    def setUp(self) -> None:
        LIB.TEST_reset()
        LIB.T_reset()

    def tearDown(self) -> None:
        self.assertEqual(int(LIB.get_mods()), 0, "modifiers leaked past key release")

    def _pressed(self) -> list[Event]:
        records = (CHistory * MAX_HISTORY)()
        count = ctypes.c_uint8()
        LIB.TEST_get_record_history(records, ctypes.byref(count))
        return [
            Event(
                keycode=int(records[i].keycode),
                pressed=bool(records[i].pressed),
                mods=int(records[i].mods),
            )
            for i in range(count.value)
            if records[i].pressed
        ]

    def test_cmd_grave_arms_b_to_backgrave(self) -> None:
        """Cmd+Grave, Cmd still held: B taps Shift+Grave and is consumed."""
        LIB.T_set_external_mods(MOD_LGUI)
        self.assertTrue(
            bool(LIB.switcher_process_record(KC_GRV, True)),
            "the arming Grave itself must still reach the host",
        )

        consumed = not bool(LIB.switcher_process_record(KC_B, True))

        self.assertTrue(consumed, "the armed B press must be consumed")
        self.assertIn(
            KC_BKGRV, [e.keycode for e in self._pressed()],
            "and must tap Shift+Grave in its place",
        )

        LIB.T_set_external_mods(0)

    def test_repeated_b_presses_keep_cycling(self) -> None:
        """Every B press while armed cycles backwards; nothing re-classifies."""
        LIB.T_set_external_mods(MOD_LGUI)
        LIB.switcher_process_record(KC_GRV, True)

        LIB.switcher_process_record(KC_B, True)
        LIB.switcher_process_record(KC_B, True)

        backgraves = [e for e in self._pressed() if e.keycode == KC_BKGRV]
        self.assertEqual(len(backgraves), 2)

        LIB.T_set_external_mods(0)

    def test_b_release_always_passes_through(self) -> None:
        """Releases are never consumed.

        A B pressed BEFORE the chord armed is a real, registered b; eating
        its release would leave the key stuck down. The release of a
        consumed press unregisters a key that was never registered, which
        QMK treats as a no-op -- so passing every release through is safe
        in both directions.
        """
        LIB.T_set_external_mods(MOD_LGUI)
        LIB.switcher_process_record(KC_GRV, True)

        self.assertTrue(bool(LIB.switcher_process_record(KC_B, False)))

        LIB.T_set_external_mods(0)

    def test_b_without_chord_passes_through(self) -> None:
        """No chord: B is just the letter b, untouched and unconsumed."""
        LIB.T_set_external_mods(MOD_LGUI)  # Cmd held, but no Grave pressed

        self.assertTrue(bool(LIB.switcher_process_record(KC_B, True)))
        self.assertEqual(self._pressed(), [], "nothing may be emitted")

        LIB.T_set_external_mods(0)

    def test_grave_without_mod_never_arms(self) -> None:
        """A bare Grave is typing; a later Cmd hold must find B normal."""
        LIB.switcher_process_record(KC_GRV, True)

        LIB.T_set_external_mods(MOD_LGUI)
        self.assertTrue(bool(LIB.switcher_process_record(KC_B, True)))

        LIB.T_set_external_mods(0)

    def test_releasing_the_mod_disarms(self) -> None:
        """Chord over on Cmd release; a fresh Cmd hold must not resurrect it."""
        LIB.T_set_external_mods(MOD_LGUI)
        LIB.switcher_process_record(KC_GRV, True)

        LIB.T_set_external_mods(0)
        LIB.T_switcher_task()

        LIB.T_set_external_mods(MOD_LGUI)
        self.assertTrue(bool(LIB.switcher_process_record(KC_B, True)))
        self.assertEqual(self._pressed(), [])

        LIB.T_set_external_mods(0)

    def test_tab_chord_does_not_arm_b(self) -> None:
        """The two chords are separate: Cmd+Tab must leave B as the letter b."""
        LIB.T_set_external_mods(MOD_LGUI)
        LIB.switcher_process_record(KC_TAB, True)

        self.assertTrue(bool(LIB.switcher_process_record(KC_B, True)))

        LIB.T_set_external_mods(0)

    def test_both_chords_coexist_under_one_mod_hold(self) -> None:
        """Cmd+Tab then Cmd+Grave, all under one Cmd: both reversers live.

        The real macOS flow -- switch app with Cmd+Tab, then cycle that
        app's windows with Cmd+Grave without ever releasing Cmd. The pad
        must still reverse the app switcher AND B must reverse the window
        cycle.
        """
        LIB.T_set_external_mods(MOD_LGUI)
        LIB.switcher_process_record(KC_TAB, True)
        LIB.switcher_process_record(KC_GRV, True)

        self.assertFalse(bool(LIB.switcher_process_record(KC_B, True)))

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        emitted = [e.keycode for e in self._pressed()]
        self.assertIn(KC_BKGRV, emitted, "B must reverse the window cycle")
        self.assertIn(KC_BKTAB, emitted, "the pad must still reverse the app switcher")

        LIB.T_set_external_mods(0)

    def test_plain_tab_press_arms_the_pad_via_the_record_path(self) -> None:
        """The _NAV layer's plain KC_TAB arms the Tab chord the same way."""
        LIB.T_set_external_mods(MOD_LGUI)
        LIB.switcher_process_record(KC_TAB, True)

        LIB.T_smtd_touch(CKC_BSPC)
        LIB.T_smtd_tap(CKC_BSPC, 0)

        self.assertIn(KC_BKTAB, [e.keycode for e in self._pressed()])

        LIB.T_set_external_mods(0)


class LedIdleTest(unittest.TestCase):
    """LEDs off after an idle timeout, back at the first touch, on at boot.

    The engine is one task function run from housekeeping every scan pass.
    It reads QMK's own last_input_activity_elapsed() -- which covers keys,
    both trackballs, and scroll, on either half -- so there is no per-event
    plumbing to test, only the threshold behavior. The fixture models the
    elapsed time directly (T_set_input_idle_ms) and records the
    rgblight_disable/enable_noeeprom calls the engine makes.
    """

    TIMEOUT_MS: int = 60000  # must match TOWNK_LED_IDLE_TIMEOUT_MS in the fixture

    def setUp(self) -> None:
        LIB.TEST_reset()
        LIB.T_reset()

    def test_boot_state_leaves_leds_alone(self) -> None:
        """A fresh, active board must not touch the LEDs at all."""
        LIB.T_set_input_idle_ms(0)
        LIB.led_idle_task()

        self.assertTrue(bool(LIB.T_rgb_enabled()))
        self.assertEqual(int(LIB.T_rgb_disable_calls()), 0)
        self.assertEqual(int(LIB.T_rgb_enable_calls()), 0)

    def test_leds_turn_off_after_the_idle_timeout(self) -> None:
        LIB.T_set_input_idle_ms(self.TIMEOUT_MS + 1)
        LIB.led_idle_task()

        self.assertFalse(bool(LIB.T_rgb_enabled()), "LEDs must be off when idle")
        self.assertEqual(int(LIB.T_rgb_disable_calls()), 1)

    def test_exactly_at_the_threshold_is_not_yet_idle(self) -> None:
        LIB.T_set_input_idle_ms(self.TIMEOUT_MS)
        LIB.led_idle_task()

        self.assertTrue(bool(LIB.T_rgb_enabled()))

    def test_staying_idle_does_not_repeat_the_disable(self) -> None:
        """Housekeeping runs every scan pass; the off transition fires once."""
        LIB.T_set_input_idle_ms(self.TIMEOUT_MS + 1)
        LIB.led_idle_task()
        LIB.led_idle_task()
        LIB.led_idle_task()

        self.assertEqual(int(LIB.T_rgb_disable_calls()), 1)

    def test_activity_restores_the_leds(self) -> None:
        """The first touch after an idle-off brings the LEDs back."""
        LIB.T_set_input_idle_ms(self.TIMEOUT_MS + 1)
        LIB.led_idle_task()

        LIB.T_set_input_idle_ms(0)  # a key press / ball touch just happened
        LIB.led_idle_task()

        self.assertTrue(bool(LIB.T_rgb_enabled()), "activity must wake the LEDs")
        self.assertEqual(int(LIB.T_rgb_enable_calls()), 1)

    def test_active_use_never_toggles(self) -> None:
        """Continuous use must not generate enable/disable churn."""
        for idle_ms in (0, 500, 5000, self.TIMEOUT_MS - 1):
            LIB.T_set_input_idle_ms(idle_ms)
            LIB.led_idle_task()

        self.assertEqual(int(LIB.T_rgb_disable_calls()), 0)
        self.assertEqual(int(LIB.T_rgb_enable_calls()), 0)


if __name__ == "__main__":
    _ = unittest.main(verbosity=2)
