"""Tests for the overlay's exact-spelling table (tools/svalwatch/overlay/spelling.py).

The table turns a numeric keycode read off the keyboard back into the
spelling keymap.c uses, because that is what skim's config is keyed on.
It is built by pairing the keymap's spellings (layout order) with the
firmware's numeric keymap (matrix order) through the layout's matrix
positions.

    python3 tests/run_tests.py
"""

import contextlib
import io
import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "tools", "svalwatch", "overlay"))

import spelling  # noqa: E402

# Two keys, on a 1-row by 2-column matrix, with the layout listing them in
# the opposite order: layout key 0 sits at matrix column 1 and key 1 at 0.
SWAPPED = [[0, 1], [0, 0]]


class PairTest(unittest.TestCase):
    def test_pair_matches_positions(self):
        spellings = [["KC_A", "KC_B"], ["MO(1)", "LT(2, KC_C)"]]
        elf = [[[0x0005, 0x0004]], [[0x4206, 0x5221]]]

        self.assertEqual(spelling.pair(spellings, elf, SWAPPED), {
            0x0004: "KC_A",
            0x0005: "KC_B",
            0x5221: "MO(1)",
            0x4206: "LT(2, KC_C)",
        })

    def test_pair_keeps_first_duplicate(self):
        spellings = [["KC_LCMD", "KC_B"], ["ML_CMD", "KC_B"]]
        elf = [[[0x0005, 0x00E3]], [[0x0005, 0x00E3]]]

        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            table = spelling.pair(spellings, elf, SWAPPED)

        self.assertEqual(table[0x00E3], "KC_LCMD")
        self.assertEqual(table[0x0005], "KC_B")
        self.assertIn("ML_CMD", err.getvalue())
        self.assertIn("KC_LCMD", err.getvalue())

    def test_pair_skips_layers_the_keymap_does_not_define(self):
        spellings = [["KC_A", "KC_B"], None]
        elf = [[[0x0005, 0x0004]], [[0x0000, 0x0000]]]

        self.assertEqual(spelling.pair(spellings, elf, SWAPPED),
                         {0x0004: "KC_A", 0x0005: "KC_B"})

    def test_pair_rejects_shape_mismatch(self):
        elf = [[[0x0005, 0x0004]], [[0x0005, 0x0004]]]

        with self.assertRaises(SystemExit) as layers:
            spelling.pair([["KC_A", "KC_B"]], elf, SWAPPED)
        self.assertRegex(str(layers.exception.code),
                         r"^spelling: the keymap has 1 layers but the firmware has 2$")

        with self.assertRaises(SystemExit) as keys:
            spelling.pair([["KC_A", "KC_B", "KC_C"], ["KC_A", "KC_B"]], elf, SWAPPED)
        self.assertRegex(str(keys.exception.code),
                         r"^spelling: layer 0 has 3 keys but the layout has 2$")


class PositionsTest(unittest.TestCase):
    def test_one_value_spelled_two_ways_keeps_both_positions(self):
        spellings = [["KC_LCMD", "KC_B"], ["ML_CMD", "KC_B"]]
        elf = [[[0x0005, 0x00E3]], [[0x0005, 0x00E3]]]

        self.assertEqual(spelling.positions(spellings, elf, SWAPPED), {
            "0,0,1": [0x00E3, "KC_LCMD"],
            "0,0,0": [0x0005, "KC_B"],
            "1,0,1": [0x00E3, "ML_CMD"],
            "1,0,0": [0x0005, "KC_B"],
        })

    def test_skips_undefined_layers(self):
        self.assertEqual(spelling.positions([None, ["KC_A", "KC_B"]],
                                            [[[0, 0]], [[0x0005, 0x0004]]], SWAPPED),
                         {"1,0,1": [0x0004, "KC_A"], "1,0,0": [0x0005, "KC_B"]})


SPEC = {"keycodes": {
    "0x0004": {"key": "KC_A"},
    "0x0005": {"key": "KC_B"},
    "0x00E3": {"key": "KC_LEFT_GUI", "aliases": ["KC_LGUI", "KC_LCMD"]},
}}


class CheckSpecTest(unittest.TestCase):
    def test_accepts_names_and_aliases_with_their_values(self):
        spellings = [["KC_A", "KC_LCMD"], ["ML_CMD", "LT(1, KC_B)"]]
        elf = [[[0x00E3, 0x0004]], [[0x4105, 0x00E3]]]

        spelling.check_spec(spellings, elf, SWAPPED, SPEC)  # no SystemExit

    def test_fails_naming_position_spelling_and_both_values(self):
        # Layer 1 paired with the wrong firmware layer: KC_B reads as 0x0004.
        spellings = [["KC_A", "KC_B"], ["KC_A", "KC_B"]]
        elf = [[[0x0005, 0x0004]], [[0x0004, 0x0004]]]

        with self.assertRaises(SystemExit) as failed:
            spelling.check_spec(spellings, elf, SWAPPED, SPEC)
        self.assertRegex(str(failed.exception.code),
                         r"(?m)^spelling: layer 1 row 0 col 0: KC_B is 0x0004 in the "
                         r"firmware but 0x0005 in the keycode spec$")


class EnumNamesTest(unittest.TestCase):
    def test_reads_the_enum_names_in_order(self):
        header = """
        enum other { NOT_ME };
        enum custom_keycodes {
            /** Doc comment, with a comma, and = sign. */
            CKC_A = RANGE_START,
            CKC_B,   ///< trailing doc, with = and ,
            CKC_C = CKC_A + 4, // plain comment
            CKC_D
        };
        """
        self.assertEqual(spelling.enum_names(header, "custom_keycodes"),
                         ["CKC_A", "CKC_B", "CKC_C", "CKC_D"])

    def test_missing_enum_fails_loudly(self):
        with self.assertRaises(SystemExit):
            spelling.enum_names("enum other { A };", "custom_keycodes")


class MergeTest(unittest.TestCase):
    def test_named_keycodes_fill_in_but_never_override(self):
        placed = {0x7E14: "CKC_BSPC", 0x7E44: "SV_SOUT"}
        named = {"CKC_BSPC": 0x7E14, "CKC_SPC": 0x7E15, "SV_OUTPUT_STATUS": 0x7E44}

        self.assertEqual(spelling.with_named(placed, named), {
            0x7E14: "CKC_BSPC",
            0x7E15: "CKC_SPC",
            0x7E44: "SV_SOUT",
        })

    def test_table_json_keys_are_hex(self):
        self.assertEqual(spelling.to_json({0x7E14: "CKC_BSPC", 4: "KC_A"}),
                         {"0x0004": "KC_A", "0x7E14": "CKC_BSPC"})


class LayerNamesTest(unittest.TestCase):
    def test_layer_names_by_index_keep_the_first_name(self):
        named = {"_BASE": 0, "_NAV": 4, "_SYS": 14, "_ALSO_NAV": 4}

        self.assertEqual(spelling.layer_names(named), {"0": "_BASE", "4": "_NAV", "14": "_SYS"})


if __name__ == "__main__":
    unittest.main()
