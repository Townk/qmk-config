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
        self.assertIn("1", str(layers.exception.code))
        self.assertIn("2", str(layers.exception.code))

        with self.assertRaises(SystemExit) as keys:
            spelling.pair([["KC_A", "KC_B", "KC_C"], ["KC_A", "KC_B"]], elf, SWAPPED)
        self.assertIn("3", str(keys.exception.code))
        self.assertIn("2", str(keys.exception.code))


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


if __name__ == "__main__":
    unittest.main()
