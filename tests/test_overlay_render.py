"""Tests for the overlay's runtime renderer (tools/svalwatch/overlay/render_page.py).

The renderer draws the overlay page on the Mac from the keymap read off
the keyboard: numbers in `[layer][row][col]` order. These tests cover its
pure part, turning those numbers into the spellings skim's config is keyed
on, in layout order. Drawing itself needs skim, which these tests do not.

    python3 tests/run_tests.py
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "tools", "svalwatch", "overlay"))

import render_page  # noqa: E402

# A slice of QMK's keycode spec (`qmk.keycodes.load_spec`), in its shape:
# the ranges the decoder needs, and a few named keycodes.
SPEC = {
    "ranges": {
        "0x0000/0x00FF": {"define": "QK_BASIC"},
        "0x0100/0x1EFF": {"define": "QK_MODS"},
        "0x2000/0x1FFF": {"define": "QK_MOD_TAP"},
        "0x4000/0x0FFF": {"define": "QK_LAYER_TAP"},
        "0x5000/0x01FF": {"define": "QK_LAYER_MOD"},
        "0x5200/0x001F": {"define": "QK_TO"},
        "0x5220/0x001F": {"define": "QK_MOMENTARY"},
        "0x52A0/0x001F": {"define": "QK_ONE_SHOT_MOD"},
        "0x7E00/0x003F": {"define": "QK_KB"},
        "0x8000/0X3FFF": {"define": "QK_UNICODEMAP"},
    },
    "keycodes": {
        "0x0000": {"group": "internal", "key": "KC_NO", "aliases": ["XXXXXXX"]},
        "0x0004": {"group": "basic", "key": "KC_A", "label": "A"},
        "0x0005": {"group": "basic", "key": "KC_B", "label": "B"},
        "0x002C": {"group": "basic", "key": "KC_SPACE", "aliases": ["KC_SPC"]},
        "0x00E1": {"group": "modifiers", "key": "KC_LEFT_SHIFT", "aliases": ["KC_LSFT"]},
        "0x7E00": {"group": "kb", "key": "QK_KB_0"},
    },
}

# Two layers of a 1-row by 2-column matrix, with the layout listing the
# keys in the opposite order: layout key 0 sits at matrix column 1.
SWAPPED = [[0, 1], [0, 0]]


def numeric(layers, rows=1, cols=2):
    return {"layers": len(layers), "rows": rows, "cols": cols,
            "keycodes": [value for layer in layers for row in layer for value in row]}


class SpellTest(unittest.TestCase):
    def test_spell_uses_table_first(self):
        self.assertEqual(render_page.spell(0x0004, None, {}, {0x0004: "MY_A"}, SPEC), "MY_A")

    def test_spell_uses_the_position_before_the_table(self):
        positions = {"15,0,4": [0x00E3, "ML_CMD"], "4,0,1": [0x00E3, "KC_LCMD"]}
        table = {0x00E3: "KC_LCMD"}

        self.assertEqual(render_page.spell(0x00E3, (15, 0, 4), positions, table, SPEC), "ML_CMD")
        self.assertEqual(render_page.spell(0x00E3, (4, 0, 1), positions, table, SPEC), "KC_LCMD")
        # Elsewhere, or once the keyboard has another value there, the table decides.
        self.assertEqual(render_page.spell(0x00E3, (3, 0, 0), positions, table, SPEC), "KC_LCMD")
        self.assertEqual(render_page.spell(0x0004, (15, 0, 4), positions, {0x0004: "KC_A"}, SPEC),
                         "KC_A")

    def test_spells_basic_from_spec(self):
        self.assertEqual(render_page.spell(0x0004, None, {}, {}, SPEC), "KC_A")
        self.assertEqual(render_page.spell(0x7E00, None, {}, {}, SPEC), "QK_KB_0")

    def test_spells_an_unseen_layer_tap_generically(self):
        # LT(3, KC_A): QK_LAYER_TAP | (3 << 8) | KC_A
        self.assertEqual(render_page.spell(0x4304, None, {}, {}, SPEC), "LT(3, KC_A)")

    def test_spells_mod_tap_and_wrapped_mods(self):
        # MT(MOD_LSFT, KC_A) = QK_MOD_TAP | (MOD_LSFT << 8) | KC_A
        self.assertEqual(render_page.spell(0x2204, None, {}, {}, SPEC), "MT(MOD_LSFT, KC_A)")
        # LSFT(KC_A) = QK_LSFT | KC_A
        self.assertEqual(render_page.spell(0x0204, None, {}, {}, SPEC), "LSFT(KC_A)")
        # Several mods, and right-hand mods (bit 4 makes all of them right).
        self.assertEqual(render_page.spell(0x2304, None, {}, {}, SPEC),
                         "MT(MOD_LCTL|MOD_LSFT, KC_A)")
        self.assertEqual(render_page.spell(0x1204, None, {}, {}, SPEC), "RSFT(KC_A)")
        self.assertEqual(render_page.spell(0x0304, None, {}, {}, SPEC), "LCTL(LSFT(KC_A))")

    def test_spells_layer_functions(self):
        self.assertEqual(render_page.spell(0x5222, None, {}, {}, SPEC), "MO(2)")
        self.assertEqual(render_page.spell(0x5201, None, {}, {}, SPEC), "TO(1)")
        self.assertEqual(render_page.spell(0x52A2, None, {}, {}, SPEC), "OSM(MOD_LSFT)")
        # LM(1, MOD_LSFT) = QK_LAYER_MOD | (1 << 5) | MOD_LSFT
        self.assertEqual(render_page.spell(0x5022, None, {}, {}, SPEC), "LM(1, MOD_LSFT)")

    def test_layer_arguments_use_the_keymaps_layer_names(self):
        names = {"2": "_GAM2", "3": "_GAM2_TOO", "5": "_NUM"}
        # LT(_NUM, KC_SPACE): an unseen layer-tap, in the spelling (and
        # spacing) skim-config.yaml's pre_process keys use.
        self.assertEqual(render_page.spell(0x452C, None, {}, {}, SPEC, names), "LT(_NUM, KC_SPACE)")
        self.assertEqual(render_page.spell(0x5222, None, {}, {}, SPEC, names), "MO(_GAM2)")
        self.assertEqual(render_page.spell(0x50A2, None, {}, {}, SPEC, names), "LM(_NUM, MOD_LSFT)")
        # A layer without a name stays numeric.
        self.assertEqual(render_page.spell(0x5221, None, {}, {}, SPEC, names), "MO(1)")

    def test_composites_spell_their_inner_key_as_the_keymap_does(self):
        # The keymap spells 0x00E1 KC_LSFT; the spec's primary name is KC_LEFT_SHIFT.
        table = {0x00E1: "KC_LSFT"}
        self.assertEqual(render_page.spell(0x21E1, None, {}, table, SPEC), "MT(MOD_LCTL, KC_LSFT)")
        self.assertEqual(render_page.spell(0x21E1, None, {}, {}, SPEC), "MT(MOD_LCTL, KC_LEFT_SHIFT)")

    def test_an_unseen_combo_matches_a_real_pre_process_key(self):
        # keyboards/svalboard/keymaps/townk/skim-config.yaml has
        # `pre_process: - keycode: LSFT(KC_TAB)`; keymap.c does not place it.
        spec = dict(SPEC, keycodes=dict(SPEC["keycodes"], **{
            "0x002B": {"group": "basic", "key": "KC_TAB"}}))
        self.assertEqual(render_page.spell(0x022B, None, {}, {0x002B: "KC_TAB"}, spec),
                         "LSFT(KC_TAB)")

    def test_unknown_value_is_hex(self):
        self.assertEqual(render_page.spell(0x7E35, None, {}, {}, SPEC), "0x7E35")
        # A wrapper around a basic code the spec does not name.
        self.assertEqual(render_page.spell(0x43A5, None, {}, {}, SPEC), "LT(3, 0x00A5)")
        # A range whose meaning depends on the firmware's build is left as hex.
        self.assertEqual(render_page.spell(0x8001, None, {}, {}, SPEC), "0x8001")


class LayoutKeymapTest(unittest.TestCase):
    def test_layout_keymap_reorders_by_matrix(self):
        keymap = numeric([[[0x0005, 0x0004]], [[0x2204, 0x5222]]])

        out = render_page.layout_keymap(keymap, SWAPPED, [0, 1], {}, {}, SPEC)

        self.assertEqual(out["layers"], [["KC_A", "KC_B"], ["MO(2)", "MT(MOD_LSFT, KC_A)"]])
        self.assertEqual(out["layout"], "LAYOUT")
        self.assertIn("keyboard", out)
        self.assertIn("keymap", out)

    def test_renders_only_the_described_layers_at_their_firmware_index(self):
        keymap = numeric([[[0x0004, 0x0004]], [[0x0005, 0x0005]], [[0x002C, 0x0004]]])
        positions = {"2,0,0": [0x002C, "KC_SPC"]}

        out = render_page.layout_keymap(keymap, SWAPPED, [2, 0], positions, {}, SPEC)

        self.assertEqual(out["layers"], [["KC_A", "KC_SPC"], ["KC_A", "KC_A"]])

    def test_layout_keymap_passes_the_layer_names_on(self):
        keymap = numeric([[[0x5222, 0x0004]]])

        out = render_page.layout_keymap(keymap, SWAPPED, [0], {}, {}, SPEC, {"2": "_GAM2"})

        self.assertEqual(out["layers"], [["KC_A", "MO(_GAM2)"]])

    def test_rows_and_cols_default_to_the_matrix(self):
        keymap = numeric([[[0x0005, 0x0004]]])
        del keymap["rows"], keymap["cols"]

        out = render_page.layout_keymap(keymap, SWAPPED, [0], {}, {}, SPEC)

        self.assertEqual(out["layers"], [["KC_A", "KC_B"]])

    def test_a_short_keymap_fails(self):
        keymap = numeric([[[0x0005, 0x0004]]])
        keymap["keycodes"].pop()

        with self.assertRaises(render_page.RenderError) as caught:
            render_page.layout_keymap(keymap, SWAPPED, [0], {}, {}, SPEC)
        self.assertIn("1 keycodes", str(caught.exception))

    def test_a_matrix_smaller_than_the_layout_fails(self):
        keymap = numeric([[[0x0005], [0x0004]]], rows=2, cols=1)

        with self.assertRaises(render_page.RenderError) as caught:
            render_page.layout_keymap(keymap, SWAPPED, [0], {}, {}, SPEC)
        self.assertIn("2x1", str(caught.exception))

    def test_a_layer_the_keyboard_lacks_fails(self):
        with self.assertRaises(render_page.RenderError) as caught:
            render_page.layout_keymap(numeric([[[4, 4]]]), SWAPPED, [0, 3], {}, {}, SPEC)
        self.assertIn("layer 3", str(caught.exception))

    def test_an_empty_described_layer_fails(self):
        # skim drops all-KC_NO/KC_TRNS layers and renumbers the rest, so
        # drawing one would put every later layer under the wrong name.
        keymap = numeric([[[0x0004, 0x0004]], [[0x0000, 0x0000]]])

        with self.assertRaises(render_page.RenderError) as caught:
            render_page.layout_keymap(keymap, SWAPPED, [0, 1], {}, {}, SPEC)
        self.assertIn("layer 1", str(caught.exception))


if __name__ == "__main__":
    unittest.main()
