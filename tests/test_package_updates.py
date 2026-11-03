"""Tests for tools/updater/package_updates.py (pure helpers only)."""

import datetime
import os
import pathlib
import re
import sys
import tempfile
import unittest

sys.path.insert(
    0,
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "updater"),
)

import package_updates as pu  # noqa: E402


class VersionString(unittest.TestCase):
    def test_version_string_format(self):
        s = pu.version_string("abcdef1234", datetime.datetime(2026, 11, 3, 15, 4, 5))
        self.assertEqual(s, "abcdef1-150405")
        self.assertLessEqual(len(s), 16)

    def test_version_string_charset(self):
        s = pu.version_string("abcdef1234", datetime.datetime(2026, 11, 3, 1, 2, 3))
        self.assertRegex(s, r"^[A-Za-z0-9._+-]{1,16}$")


class QmkHome(unittest.TestCase):
    def test_qmk_home_strips_config_suffix(self):
        self.assertEqual(
            pu.qmk_home_from_config("user.qmk_home=/x/qmk (config)\n"),
            pathlib.Path("/x/qmk"),
        )


class Uf2Targets(unittest.TestCase):
    def test_uf2_targets_finds_both_halves(self):
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            for h in ("left", "right"):
                (d / f"svalboard_trackball_pmw3389_{h}_townk.uf2").write_bytes(b"x")
            t = pu.uf2_targets(d)
            self.assertEqual(set(t), {"left", "right"})
            self.assertEqual(t["left"].name, "svalboard_trackball_pmw3389_left_townk.uf2")

    def test_uf2_targets_refuses_missing_half(self):
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            (d / "svalboard_trackball_pmw3389_left_townk.uf2").write_bytes(b"x")
            with self.assertRaises(SystemExit) as cm:
                pu.uf2_targets(d)
            self.assertIn("right", str(cm.exception))


    def test_uf2_targets_one_half_needs_only_that_half(self):
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            (d / "svalboard_trackball_pmw3389_left_townk.uf2").write_bytes(b"x")
            self.assertEqual(set(pu.uf2_targets(d, ("left",))), {"left"})


class BuildEnv(unittest.TestCase):
    REPO_QMK_JSON = pathlib.Path(__file__).resolve().parent.parent / "qmk.json"

    def test_repo_targets_are_updater_builds(self):
        # The single-target build takes its flags from here, so every target
        # the repo lists must carry the updater flags `make` builds with.
        for half in ("left", "right"):
            env = pu.build_env(
                self.REPO_QMK_JSON, f"svalboard/trackball/pmw3389/{half}", "townk"
            )
            self.assertEqual(
                env, ["-e", "SVAL_UPDATER=yes", "-e", "SVAL_UPDATE_NO_CHORD=yes"]
            )

    def test_unlisted_target_is_refused(self):
        with self.assertRaises(SystemExit) as cm:
            pu.build_env(self.REPO_QMK_JSON, "svalboard/trackball/pmw3389/left", "other")
        self.assertIn("not a build target", str(cm.exception))

    def test_target_without_env(self):
        with tempfile.TemporaryDirectory() as d:
            j = pathlib.Path(d) / "qmk.json"
            j.write_text('{"build_targets": [["kb", "km"]]}')
            self.assertEqual(pu.build_env(j, "kb", "km"), [])


if __name__ == "__main__":
    unittest.main()
