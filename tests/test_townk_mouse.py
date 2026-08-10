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
    lib.T_pointing.argtypes = [ctypes.c_int8] * 4
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
    # TEST_get_record_history deliberately has no argtypes: ctypes already
    # passes an array and a byref() correctly, and declaring them would mean
    # ctypes.POINTER(), which is deprecated.
    lib.get_mods.restype = ctypes.c_uint8

    for name in ("T_kc_mb_sft", "T_kc_mb_alt", "T_kc_mb_gui", "T_kc_mb_ctl",
                 "T_kc_ckc_spc", "T_kc_ckc_bspc", "T_kc_btn1", "T_kc_btn2",
                 "T_kc_btn3", "T_kc_del", "T_kc_bspc", "T_kc_plain"):
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

MOUSE_BUTTONS = (KC_BTN1, KC_BTN2, KC_BTN3)
LAYER_NAV: int = int(LIB.T_layer_nav())
LAYER_MBO: int = int(LIB.T_layer_mbo())


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


if __name__ == "__main__":
    _ = unittest.main(verbosity=2)
