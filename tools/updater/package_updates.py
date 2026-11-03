#!/usr/bin/env python3
"""Package the built firmware as upstream in-firmware-updater .svup files.

    package_updates.py version
    package_updates.py package [--qmk-home DIR] --dir DIR

Stdlib only, Python 3.9 compatible. Run with `python3 -I`.
"""

import argparse
import datetime
import pathlib
import shutil
import subprocess
import sys

KB_BASE = "svalboard/trackball/pmw3389"
KEYMAP = "townk"
HALVES = ("left", "right")
TOOLS = ("sval_update.py", "make_update.py")


def version_string(sha, now):
    """`<sha7>-<HHMMSS>`: 14 chars, within the 16-char updater limit."""
    return "%s-%s" % (sha[:7], now.strftime("%H%M%S"))


def qmk_home_from_config(output):
    """Parse `qmk config user.qmk_home` ("user.qmk_home=/path (config)")."""
    value = output.strip().split("=", 1)[1]
    if value.endswith(" (config)"):
        value = value[: -len(" (config)")]
    return pathlib.Path(value.strip())


def uf2_name(half):
    return "svalboard_trackball_pmw3389_%s_%s.uf2" % (half, KEYMAP)


def uf2_targets(directory):
    directory = pathlib.Path(directory)
    found = {}
    for half in HALVES:
        path = directory / uf2_name(half)
        if not path.is_file():
            raise SystemExit("missing %s firmware: %s" % (half, path))
        found[half] = path
    return found


def _git_sha():
    try:
        out = subprocess.run(
            ["git", "rev-parse", "HEAD"], check=True, capture_output=True, text=True
        )
    except (OSError, subprocess.CalledProcessError) as e:
        raise SystemExit("cannot determine git revision: %s" % e)
    return out.stdout.strip()


def _default_qmk_home():
    try:
        out = subprocess.run(
            ["qmk", "config", "user.qmk_home"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as e:
        raise SystemExit("cannot read qmk_home from qmk config: %s" % e)
    return qmk_home_from_config(out.stdout)


def package(directory, qmk_home):
    directory = pathlib.Path(directory)
    tools_dir = pathlib.Path(qmk_home) / "keyboards" / "svalboard" / "tools"
    targets = uf2_targets(directory)
    for name in TOOLS:
        if not (tools_dir / name).is_file():
            raise SystemExit("missing updater tool: %s" % (tools_dir / name))
    for name in TOOLS:
        shutil.copy2(tools_dir / name, directory / name)
    make_update = directory / "make_update.py"
    for half, uf2 in targets.items():
        svup = uf2.with_suffix(".svup")
        run = [sys.executable, "-I", str(make_update)]
        subprocess.run(
            run
            + [str(uf2), "-o", str(svup), "--kb", "%s/%s" % (KB_BASE, half)]
            + ["--keymap", KEYMAP, "--diagnostic", "--yes"],
            check=True,
        )
        subprocess.run(run + ["verify", str(svup), "--build", "plain"], check=True)
        print("packaged %s" % svup.name)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("version")
    p = sub.add_parser("package")
    p.add_argument("--qmk-home", type=pathlib.Path)
    p.add_argument("--dir", type=pathlib.Path, required=True)
    args = ap.parse_args(argv)
    if args.cmd == "version":
        print(version_string(_git_sha(), datetime.datetime.now()))
        return 0
    try:
        package(args.dir, args.qmk_home or _default_qmk_home())
    except subprocess.CalledProcessError as e:
        raise SystemExit("step failed (exit %s): %s" % (e.returncode, " ".join(map(str, e.cmd))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
