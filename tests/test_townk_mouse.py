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
sys.path.insert(0, os.path.join(SUBMODULE, "tests"))
from sm_td_bindings import CHistory  # noqa: E402

MAX_HISTORY = 100

# MOD_BIT() masks the keycode with 0x07, so these match the firmware's bits.
MOD_LCTL = 1 << 0
MOD_LSFT = 1 << 1
MOD_LALT = 1 << 2
MOD_LGUI = 1 << 3


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
    lib.T_pointing.argtypes = [ctypes.c_int8] * 4
    lib.T_set_external_mods.argtypes = [ctypes.c_uint8]
    lib.T_reset.argtypes = []
    lib.T_mouse_mode_calls.restype = ctypes.c_int
    lib.T_mouse_mode_state.restype = ctypes.c_bool
    lib.TEST_reset.argtypes = []
    # TEST_get_record_history deliberately has no argtypes: ctypes already
    # passes an array and a byref() correctly, and declaring them would mean
    # ctypes.POINTER(), which is deprecated.
    lib.get_mods.restype = ctypes.c_uint8

    for name in ("T_kc_mb_sft", "T_kc_mb_alt", "T_kc_mb_gui", "T_kc_mb_ctl",
                 "T_kc_ckc_spc", "T_kc_btn1", "T_kc_btn3", "T_kc_plain"):
        getattr(lib, name).restype = ctypes.c_uint16

    return lib


LIB = _build()

MB_GUI: int = int(LIB.T_kc_mb_gui())
MB_SFT: int = int(LIB.T_kc_mb_sft())
CKC_SPC: int = int(LIB.T_kc_ckc_spc())
KC_BTN1: int = int(LIB.T_kc_btn1())
KC_BTN3: int = int(LIB.T_kc_btn3())
KC_PLAIN: int = int(LIB.T_kc_plain())

MOUSE_BUTTONS = (KC_BTN1, KC_BTN3)


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


if __name__ == "__main__":
    _ = unittest.main(verbosity=2)
