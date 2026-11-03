#!/usr/bin/env python3
"""Package the built firmware as upstream in-firmware-updater .svup files.

    package_updates.py version
    package_updates.py build-env KEYBOARD KEYMAP [--qmk-json FILE]
    package_updates.py package [--qmk-home DIR] --dir DIR [--half left|right]

Stdlib only, Python 3.9 compatible. Run with `python3 -I`.
"""

import argparse
import datetime
import json
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


def build_env(qmk_json, keyboard, keymap):
    """The `-e KEY=VALUE` arguments qmk.json gives one build target.

    qmk.json is the one place the updater flags live: `qmk userspace-compile`
    reads them itself, and a single-target `qmk compile` is handed them from
    here, so the two builds cannot drift. A target qmk.json does not list is
    refused rather than built without them -- it would not be an updater build.
    """
    targets = json.loads(pathlib.Path(qmk_json).read_text()).get("build_targets", [])
    for target in targets:
        if isinstance(target, list) and target[:2] == [keyboard, keymap]:
            env = target[2] if len(target) > 2 else {}
            return [x for k, v in env.items() for x in ("-e", "%s=%s" % (k, v))]
    raise SystemExit(
        "%s:%s is not a build target in %s; add it there so it builds with the "
        "updater flags" % (keyboard, keymap, qmk_json)
    )


def uf2_name(half):
    return "svalboard_trackball_pmw3389_%s_%s.uf2" % (half, KEYMAP)


def uf2_targets(directory, halves=HALVES):
    directory = pathlib.Path(directory)
    found = {}
    for half in halves:
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


def package(directory, qmk_home, halves=HALVES):
    directory = pathlib.Path(directory)
    tools_dir = pathlib.Path(qmk_home) / "keyboards" / "svalboard" / "tools"
    targets = uf2_targets(directory, halves)
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
    e = sub.add_parser("build-env")
    e.add_argument("keyboard")
    e.add_argument("keymap")
    e.add_argument("--qmk-json", type=pathlib.Path, default=pathlib.Path("qmk.json"))
    p = sub.add_parser("package")
    p.add_argument("--qmk-home", type=pathlib.Path)
    p.add_argument("--dir", type=pathlib.Path, required=True)
    p.add_argument("--half", choices=HALVES, help="package this half only")
    args = ap.parse_args(argv)
    if args.cmd == "version":
        print(version_string(_git_sha(), datetime.datetime.now()))
        return 0
    if args.cmd == "build-env":
        print(" ".join(build_env(args.qmk_json, args.keyboard, args.keymap)))
        return 0
    halves = (args.half,) if args.half else HALVES
    try:
        package(args.dir, args.qmk_home or _default_qmk_home(), halves)
    except subprocess.CalledProcessError as e:
        raise SystemExit("step failed (exit %s): %s" % (e.returncode, " ".join(map(str, e.cmd))))
    if args.half:
        # The other half's .svup, if any, is from another build, and qmk-flash
        # refuses halves whose version strings differ -- so say it up front.
        print(
            "note: only the %s half was rebuilt; qmk-flash installs both halves "
            "from ONE build, so run `make` before flashing over the updater"
            % args.half
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
