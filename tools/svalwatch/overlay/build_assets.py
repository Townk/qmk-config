"""Build svaloverlay's page, and its render bundle, from this userspace's keymap.

    python build_assets.py --repo ROOT --qmk-home QMK --out DIR [--elf ELF]

Run it with a Python that has skim installed (it calls `python -m skim`
with its own interpreter); `make overlay-assets` picks one. It needs the
built firmware (`make` in ROOT): the ELF, and the compile flags `qmk
compile` leaves next to its objects. Steps:

1. `qmk c2json` the keymap, as the pre-commit hook does for docs/.
2. Merge skim-overlay.yaml over the keymap's skim-config.yaml.
3. `skim generate` every layer image with that config.
4. Inline the images into page.html, together with a manifest of layer
   names and the Svalboard layout's matrix position for each keymap
   index (from QMK's keyboards/svalboard/info.json), and copy the page's
   CSS, JS and icon font next to it. That is the page the overlay ships
   with, shown until it has drawn the keyboard's own keymap.
5. Write the render bundle to DIR/render/: what render_page.py needs to
   draw the same page on the Mac from the keymap read off the keyboard
   (the page code and files, the merged config, the manifest, the
   exact-spelling tables, by value and by position, and QMK's keycode
   spec), plus a VERSION hash of all of it.

skim must tag keys (`<g data-key="N">`, skim's feat/key-tags); the build
fails if the images have no tags, since the overlay could not highlight.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import page_build
import spelling

HERE = Path(__file__).resolve().parent
KEYMAP = "keyboards/svalboard/keymaps/townk"
QMK_KEYBOARD = "svalboard/trackball/pmw3389/right"
# What `qmk userspace-compile` leaves in qmk_home for that keyboard.
BUILD_NAME = "svalboard_trackball_pmw3389_right_townk"
# The userspace's custom keycodes: every member is in the spelling table.
KEYCODES_HEADER = "users/townk/townk_keycodes.h"
KEYCODES_ENUM = "custom_keycodes"
# The header's icons (keyboard, pin). Optional: without it they fall back
# to whatever the system has, usually a missing-glyph box.
ICON_FONT = Path.home() / "Library/Fonts/SymbolsNerdFontMono-Regular.ttf"
# Copied into the bundle from this directory: the renderer and the page code.
BUNDLE_CODE = ("render_page.py", "page_build.py", "page.html") + page_build.PAGE_FILES


def keymap_json(repo: Path, work: Path) -> Path:
    out = work / "keymap.json"
    with out.open("w") as f:
        subprocess.run(
            ["qmk", "c2json", "-kb", QMK_KEYBOARD, "-km", "townk", "--no-cpp",
             str(repo / KEYMAP / "keymap.c")],
            check=True, stdout=f)
    return out


def layout_spellings(keymap: Dict[str, Any], config: Dict[str, Any],
                     layers: int) -> List[Optional[List[str]]]:
    """The keymap's spellings, by firmware layer index (None: not defined).

    `qmk c2json --no-cpp` drops the `[_NAV] =` designators, so its layers
    are numbered as skim numbers them: the config's layers, in order.
    """
    indices = [layer["index"] for layer in config["keyboard"]["layers"]]
    if len(indices) != len(keymap["layers"]):
        sys.exit(f"build_assets: keymap.c has {len(keymap['layers'])} layers but "
                 f"skim-config.yaml lists {len(indices)}")
    out: List[Optional[List[str]]] = [None] * layers
    for index, keys in zip(indices, keymap["layers"]):
        if not 0 <= index < layers:
            sys.exit(f"build_assets: skim-config.yaml layer {index} is not one of "
                     f"the firmware's {layers}")
        out[index] = keys
    return out


def spelling_tables(repo: Path, qmk_home: Path, elf: Path, keymap: Dict[str, Any],
                    config: Dict[str, Any], matrix: List[List[int]],
                    spec: Dict[str, Any]) -> Tuple[Dict[int, str], Dict[str, List[Any]]]:
    """The value table (placed keys, then every named custom keycode) and
    the per-position table, after checking the pairing against `spec`."""
    stale = [path for path in (repo / KEYMAP / "keymap.c", repo / KEYCODES_HEADER)
             if elf.is_file() and path.stat().st_mtime > elf.stat().st_mtime]
    if stale:
        sys.exit(f"build_assets: {stale[0]} is newer than {elf}; rebuild the firmware (`make`)")
    rows = 1 + max(row for row, _ in matrix)
    cols = 1 + max(col for _, col in matrix)
    numbers = spelling.elf_keymaps(elf, rows, cols)
    spellings = layout_spellings(keymap, config, len(numbers))
    spelling.check_spec(spellings, numbers, matrix, spec)
    placed = spelling.pair(spellings, numbers, matrix)
    by_position = spelling.positions(spellings, numbers, matrix)

    names = spelling.enum_names((repo / KEYCODES_HEADER).read_text(), KEYCODES_ENUM)
    sval = json.loads((repo / KEYMAP / "sval.json").read_text())
    names += [key["name"] for key in sval.get("customKeycodes", [])]
    cflags = elf.parent / f"obj_{elf.stem}" / "cflags.txt"
    named = spelling.compiled_values(
        qmk_home, cflags, ["QMK_KEYBOARD_H", f'"{Path(KEYCODES_HEADER).name}"'], names)
    return spelling.with_named(placed, named), by_position


def keycode_spec(qmk_home: Path) -> Dict[str, Any]:
    """QMK's keycode spec, as `qmk.keycodes.load_spec('latest')` builds it.

    Loaded with the qmk CLI's own interpreter, which has the libraries
    qmk_home's Python code needs (skim's does not).
    """
    qmk = shutil.which("qmk")
    if qmk is None:
        sys.exit("build_assets: no `qmk` on PATH")
    shebang = Path(qmk).read_text(errors="replace").splitlines()[0]
    if not shebang.startswith("#!"):
        sys.exit(f"build_assets: cannot tell which Python runs {qmk}")
    python = shebang[2:].split()
    code = ("import json, sys; sys.path.insert(0, 'lib/python'); "
            "from qmk.keycodes import load_spec; json.dump(load_spec('latest'), sys.stdout)")
    done = subprocess.run([*python, "-c", code], cwd=qmk_home, check=True,
                          stdout=subprocess.PIPE, text=True)
    return json.loads(done.stdout)


def write_bundle(final: Path, config_path: Path, page_manifest: Dict[str, Any],
                 table: Dict[int, str], by_position: Dict[str, List[Any]],
                 spec: Dict[str, Any], icon_font: Path) -> None:
    """The render bundle in `final`, replacing whatever was there.

    Written in full next to it, then swapped in with renames, so a reader
    sees the old bundle or the new one, never a half-written one.
    """
    out = final.with_name(final.name + ".tmp")
    old = final.with_name(final.name + ".old")
    for leftover in (out, old):
        if leftover.exists():
            shutil.rmtree(leftover)
    out.mkdir(parents=True)
    for name in BUNDLE_CODE:
        shutil.copyfile(HERE / name, out / name)
    if icon_font.exists():
        shutil.copyfile(icon_font, out / "icons.ttf")
    shutil.copyfile(config_path, out / "skim-config.yaml")
    (out / "manifest.json").write_text(json.dumps(page_manifest))
    (out / "spelling.json").write_text(json.dumps(spelling.to_json(table), indent=1) + "\n")
    (out / "positions.json").write_text(json.dumps(by_position, indent=1) + "\n")
    (out / "keycodes.json").write_text(json.dumps(spec))
    (out / "VERSION").write_text(bundle_version(out))
    if final.exists():
        final.rename(old)
    out.rename(final)
    if old.exists():
        shutil.rmtree(old)


def bundle_version(bundle: Path) -> str:
    """SHA-256 over every bundle file's name and bytes, in name order."""
    digest = hashlib.sha256()
    for path in sorted(p for p in bundle.iterdir() if p.is_file() and p.name != "VERSION"):
        data = path.read_bytes()
        digest.update(f"{path.name}\0{len(data)}\0".encode())
        digest.update(data)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description="Build svaloverlay's page from this userspace's keymap.")
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--qmk-home", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--elf", type=Path,
                        help=f"the built firmware (default: QMK/.build/{BUILD_NAME}.elf)")
    parser.add_argument("--icon-font", type=Path, default=ICON_FONT,
                        help="Symbols Nerd Font Mono .ttf (default: the installed one)")
    args = parser.parse_args()
    elf = args.elf or args.qmk_home / ".build" / f"{BUILD_NAME}.elf"
    if not elf.is_file():
        sys.exit(f"build_assets: no firmware ELF at {elf}; build the firmware first (`make`)")

    info = json.loads((args.qmk_home / "keyboards/svalboard/info.json").read_text())
    layout = next(iter(info["layouts"].values()))["layout"]

    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        keymap = keymap_json(args.repo, work)
        config_path, config = page_build.skim_config(
            args.repo / KEYMAP / "skim-config.yaml", HERE / "skim-overlay.yaml", work)
        images = page_build.layer_images(keymap, config_path, work)
        page_manifest = page_build.manifest(config, layout)
        spec = keycode_spec(args.qmk_home)
        table, by_position = spelling_tables(
            args.repo, args.qmk_home, elf, json.loads(keymap.read_text()), config,
            page_manifest["matrix"], spec)

        if not page_build.write_page(args.out, page_build.page(images, page_manifest),
                                     args.icon_font):
            print(f"build_assets: no icon font at {args.icon_font}; header icons will not render")
        write_bundle(args.out / "render", config_path, page_manifest, table, by_position,
                     spec, args.icon_font)
    print(f"build_assets: {len(images)} layers, {len(table)} spellings -> {args.out}")


if __name__ == "__main__":
    main()
