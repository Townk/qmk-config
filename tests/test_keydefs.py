"""Tests for the key-definitions fingerprint (tools/keydefs.py).

The fingerprint must change exactly when a custom keycode or layer value
can move: a member inserted, removed or reordered in `enum custom_keycodes`
or `enum layer_names`, an initializer changed, or sval.json's
`customKeycodes` reordered. Comments, formatting and descriptions must not
move it.

    python3 tests/run_tests.py
"""

import io
import json
import os
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "tools"))

import keydefs  # noqa: E402

KEYCODES = """
#define RANGE_START QK_KB_20
enum custom_keycodes {
    /** Doc comment, with a comma, and = sign. */
    CKC_BSPC = RANGE_START,
    CKC_SPC,   ///< trailing doc, with = and ,
    MB_SFT,    // plain comment
};
"""

LAYERS = """
enum layer_names {
    _BASE,
    _NAV,
    _SYS = LAST_LAYER - 1, ///< doc
    _MBO = LAST_LAYER,
};
"""

SVAL = json.dumps({"customKeycodes": [
    {"name": "SV_LEFT_DPI_INC", "title": "Left +", "shortName": "L+"},
    {"name": "SV_LEFT_DPI_DEC", "title": "Left -", "shortName": "L-"},
]})


def fp(keycodes=KEYCODES, layers=LAYERS, sval=SVAL):
    return keydefs.fingerprint_of(keycodes, layers, sval)


class MembersTest(unittest.TestCase):
    def test_names_and_initializers_in_order(self):
        self.assertEqual(keydefs.enum_members(LAYERS, "layer_names"), [
            ("_BASE", None), ("_NAV", None), ("_SYS", "LAST_LAYER - 1"), ("_MBO", "LAST_LAYER")])
        self.assertEqual(keydefs.enum_names(KEYCODES, "custom_keycodes"),
                         ["CKC_BSPC", "CKC_SPC", "MB_SFT"])

    def test_without_a_trailing_comma(self):
        self.assertEqual(keydefs.enum_names("enum custom_keycodes { A = X, B }", "custom_keycodes"),
                         ["A", "B"])

    def test_conditional_members_are_refused(self):
        header = """
        enum custom_keycodes {
            A = RANGE_START,
        #ifdef SVALBOARD
            B,
        #endif
            C,
        };
        """
        with self.assertRaisesRegex(keydefs.KeydefsError, "preprocessor"):
            keydefs.enum_members(header, "custom_keycodes")

    def test_a_missing_or_unreadable_enum_is_refused(self):
        with self.assertRaises(keydefs.KeydefsError):
            keydefs.enum_members("enum other { A };", "custom_keycodes")
        with self.assertRaises(keydefs.KeydefsError):
            keydefs.enum_members("enum custom_keycodes { A B, C };", "custom_keycodes")
        with self.assertRaises(keydefs.KeydefsError):
            keydefs.enum_members("enum custom_keycodes { A =, C };", "custom_keycodes")

    def test_sval_keycodes_need_names(self):
        self.assertEqual(keydefs.sval_keycodes(SVAL), ["SV_LEFT_DPI_INC", "SV_LEFT_DPI_DEC"])
        self.assertEqual(keydefs.sval_keycodes("{}"), [])
        with self.assertRaises(keydefs.KeydefsError):
            keydefs.sval_keycodes('{"customKeycodes": [{"title": "x"}]}')
        with self.assertRaises(keydefs.KeydefsError):
            keydefs.sval_keycodes("not json")


class FingerprintTest(unittest.TestCase):
    def test_is_32_bits_and_stable(self):
        self.assertEqual(fp(), fp())
        self.assertTrue(0 <= fp() < 1 << 32)

    def test_ignores_comments_formatting_and_descriptions(self):
        reflowed = KEYCODES.replace("CKC_BSPC = RANGE_START", "CKC_BSPC=RANGE_START /* x */") \
                           .replace("///< trailing doc, with = and ,", "")
        self.assertEqual(fp(keycodes=reflowed), fp())
        retitled = json.dumps({"customKeycodes": [
            {"name": "SV_LEFT_DPI_INC", "title": "other"}, {"name": "SV_LEFT_DPI_DEC"}]})
        self.assertEqual(fp(sval=retitled), fp())
        self.assertEqual(fp(layers=LAYERS.replace("LAST_LAYER - 1", "LAST_LAYER  -  1")), fp())

    def test_changes_when_a_value_can_move(self):
        base = fp()
        changed = {
            "keycode inserted": fp(keycodes=KEYCODES.replace("CKC_SPC,", "NEW, CKC_SPC,")),
            "keycode reordered": fp(keycodes=KEYCODES.replace("CKC_SPC,", "TMP,")
                                    .replace("MB_SFT,", "CKC_SPC,").replace("TMP,", "MB_SFT,")),
            "keycode base": fp(keycodes=KEYCODES.replace("= RANGE_START", "= SAFE_RANGE")),
            "layer inserted": fp(layers=LAYERS.replace("_NAV,", "_NEW, _NAV,")),
            "layer initializer": fp(layers=LAYERS.replace("LAST_LAYER - 1", "LAST_LAYER - 2")),
            "sval reordered": fp(sval=json.dumps({"customKeycodes": [
                {"name": "SV_LEFT_DPI_DEC"}, {"name": "SV_LEFT_DPI_INC"}]})),
            "sval appended": fp(sval=json.dumps({"customKeycodes": [
                {"name": "SV_LEFT_DPI_INC"}, {"name": "SV_LEFT_DPI_DEC"}, {"name": "SV_X"}]})),
        }
        for what, value in changed.items():
            self.assertNotEqual(value, base, what)

    def test_a_name_cannot_slide_between_sources(self):
        # The same names, split differently between the enums, still differ.
        moved = KEYCODES.replace("    MB_SFT,    // plain comment\n", "")
        layers = LAYERS.replace("_BASE,", "MB_SFT, _BASE,")
        self.assertNotEqual(fp(keycodes=moved, layers=layers), fp())

    def test_the_repo_sources_parse(self):
        self.assertEqual(keydefs.fingerprint(REPO), keydefs.fingerprint(REPO))


class CommandLineTest(unittest.TestCase):
    def run_cli(self, *args):
        out = io.StringIO()
        with redirect_stdout(out):
            code = keydefs.main(["--repo", str(REPO), *args])
        return code, out.getvalue()

    def test_fingerprint_prints_hex(self):
        code, out = self.run_cli("fingerprint")
        self.assertEqual(code, 0)
        self.assertEqual(out.strip(), keydefs.format_fp(keydefs.fingerprint(REPO)))
        self.assertRegex(out.strip(), r"^0x[0-9A-F]{8}$")

    def test_matches_compares_a_bundle(self):
        ours = keydefs.format_fp(keydefs.fingerprint(REPO))
        with tempfile.TemporaryDirectory() as tmp:
            bundle = Path(tmp) / "keydefs.json"
            self.assertEqual(self.run_cli("matches", str(bundle))[0], 1)  # missing
            bundle.write_text(json.dumps({"fingerprint": ours}))
            self.assertEqual(self.run_cli("matches", str(bundle))[0], 0)
            bundle.write_text(json.dumps({"fingerprint": "0x00000000" if ours != "0x00000000"
                                          else "0x00000001"}))
            code, out = self.run_cli("matches", str(bundle))
            self.assertEqual(code, 1)
            self.assertIn(ours, out)
            bundle.write_text("garbage")
            self.assertEqual(self.run_cli("matches", str(bundle))[0], 1)

    def test_runs_isolated_from_any_directory(self):
        # As users/townk/rules.mk runs it: python3 -I, cwd qmk_home.
        with tempfile.TemporaryDirectory() as tmp:
            done = subprocess.run([sys.executable, "-I", str(REPO / "tools" / "keydefs.py"),
                                   "fingerprint"], cwd=tmp, stdout=subprocess.PIPE, text=True,
                                  check=True)
        self.assertEqual(done.stdout.strip(), keydefs.format_fp(keydefs.fingerprint(REPO)))


if __name__ == "__main__":
    unittest.main()
