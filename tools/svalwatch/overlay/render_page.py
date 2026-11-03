"""Draw the overlay page from the keymap read off the keyboard.

    render_page.py --bundle DIR --keymap FILE --out DIR

Runs on the Mac, from the render bundle build_assets.py writes (DIR is
`<assets>/render/`, where this file ships), under skim's Python:

    uv run --no-project --quiet --python 3.13 --with qmk-skim==0.7.5 \\
        python -I -B <bundle>/render_page.py --bundle <bundle> --keymap km.json --out OUT

FILE is the keymap as numbers, in the firmware's `[layer][row][col]`
order: `{"layers": N, "rows": R, "cols": C, "keycodes": [...]}` (rows and
cols default to the layout's matrix size). The page lands in OUT
(index.html, page.css, page.js, icons.ttf: the same set as the page the
overlay ships with), drawn with the same skim config and page code
(page_build.py), so the same keymap gives the same page.

skim's config is keyed on keymap.c's spellings, so each number is spelled
first: see `spell`. Only the layers the bundle's config describes are
drawn, each taken from the keyboard's keymap at its firmware index; the
keyboard's other layers are ignored. An unknown keycode never fails a
render (it is drawn as its hex value). Any real failure exits non-zero
with one line on stderr.

Kept Python 3.9-compatible, and importable without skim (the repo's
tests import its pure functions): page_build, which needs yaml, is only
imported to draw.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional, Sequence, Tuple

# The bundle ships in ~/.local/share/svaloverlay, a chezmoi external with
# `exact = true`: importing page_build from it must not leave __pycache__
# there. svaloverlay also passes -B; this covers any other way in.
sys.dont_write_bytecode = True

HERE = Path(__file__).resolve().parent
# What skim drops as an empty layer (renumbering the rest).
EMPTY_KEYS = frozenset({"KC_NO", "KC_TRNS"})
# QMK's 5-bit mod field: bits 0-3 are ctrl, shift, alt, gui; bit 4 makes
# them all right-hand.
MOD_NAMES = ("CTL", "SFT", "ALT", "GUI")
MOD_RIGHT = 0x10

Position = Tuple[int, int, int]


class RenderError(Exception):
    """A render that cannot go on; its message is the one stderr line."""


# --- Spelling ---------------------------------------------------------------

def _ranges(spec: Dict[str, Any]) -> Dict[str, Tuple[int, int]]:
    """The spec's ranges as `{define: (first, last)}`.

    A spec range is `"0xSTART/0xSIZE"`, SIZE being the offset of its last
    value.
    """
    out = {}
    for span, info in spec.get("ranges", {}).items():
        start, size = span.split("/")
        first = int(start, 16)
        out[info["define"]] = (first, first + int(size, 16))
    return out


def _named(value: int, spec: Dict[str, Any]) -> Optional[str]:
    keycode = spec.get("keycodes", {}).get(f"0x{value:04X}")
    return keycode["key"] if keycode else None


def _mod_bits(mods: int) -> List[str]:
    side = "R" if mods & MOD_RIGHT else "L"
    return [side + name for bit, name in enumerate(MOD_NAMES) if mods & (1 << bit)]


def _mod_mask(mods: int) -> str:
    return "|".join(f"MOD_{name}" for name in _mod_bits(mods)) or "0"


def _wrapped(mods: int, keycode: str) -> str:
    for name in reversed(_mod_bits(mods)):
        keycode = f"{name}({keycode})"
    return keycode


# Layer functions with the layer as their only argument, by spec range.
LAYER_FUNCTIONS = {
    "QK_TO": "TO", "QK_MOMENTARY": "MO", "QK_DEF_LAYER": "DF",
    "QK_TOGGLE_LAYER": "TG", "QK_ONE_SHOT_LAYER": "OSL",
    "QK_LAYER_TAP_TOGGLE": "TT", "QK_PERSISTENT_DEF_LAYER": "PDF",
}


def _decode(define: str, value: int, offset: int, basic: Callable[[int], str],
            layer: Callable[[int], str]) -> Optional[str]:
    """QMK's spelling of `value`, `offset` into the range `define`.

    The bit fields within each range are QMK's (quantum/keycodes.h); the
    ranges themselves come from the spec. `basic` spells an inner basic
    keycode and `layer` a layer argument. Arguments are separated by ", ",
    as keymap.c (and skim-config.yaml's keys) write them; mod masks are
    not spaced. None: a range this does not decode.
    """
    low = offset & 0xFF
    if define == "QK_MODS":
        # Its mod bits are the value's own (QK_LCTL = 0x0100, ...); the
        # range starts at the first of them, not at a field boundary.
        return _wrapped((value >> 8) & 0x1F, basic(low))
    if define == "QK_MOD_TAP":
        return f"MT({_mod_mask((offset >> 8) & 0x1F)}, {basic(low)})"
    if define == "QK_LAYER_TAP":
        return f"LT({layer((offset >> 8) & 0xF)}, {basic(low)})"
    if define == "QK_LAYER_MOD":
        return f"LM({layer((offset >> 5) & 0xF)}, {_mod_mask(offset & 0x1F)})"
    if define == "QK_ONE_SHOT_MOD":
        return f"OSM({_mod_mask(offset & 0x1F)})"
    if define == "QK_SWAP_HANDS" and low < 0xF0:
        return f"SH_T({basic(low)})"
    if define == "QK_TAP_DANCE":
        return f"TD({offset})"
    if define in LAYER_FUNCTIONS:
        return f"{LAYER_FUNCTIONS[define]}({layer(offset)})"
    return None


def spell(value: int, position: Optional[Position], positions: Dict[str, List[Any]],
          table: Dict[int, str], spec: Dict[str, Any],
          layer_names: Optional[Dict[str, str]] = None) -> str:
    """The spelling skim sees for `value`, found at `position` (layer, row, col).

    In order: (1) keymap.c's spelling at that position, when the keyboard
    still has the same value there (`positions`, from positions.json);
    (2) the build-time spelling of the value (`table`, from spelling.json);
    (3) a generic QMK spelling, from the keycode spec's names and ranges,
    in keymap.c's words where they are known: a composite's inner key as
    `table` spells it, and a layer as its identifier (`layer_names`,
    `{"<index>": "<identifier>"}`, from layers.json; numeric otherwise);
    (4) `0x%04X`.
    """
    if position is not None:
        placed = positions.get("{},{},{}".format(*position))
        if placed is not None and placed[0] == value:
            return placed[1]
    if value in table:
        return table[value]
    named = _named(value, spec)
    if named:
        return named
    names = layer_names or {}

    def basic(code: int) -> str:
        return table.get(code) or _named(code, spec) or f"0x{code:04X}"

    def layer(index: int) -> str:
        return names.get(str(index), str(index))

    for define, (first, last) in _ranges(spec).items():
        if first <= value <= last and define != "QK_BASIC":
            decoded = _decode(define, value, value - first, basic, layer)
            if decoded:
                return decoded
    return f"0x{value:04X}"


# --- The keymap skim draws --------------------------------------------------

def layout_keymap(numeric: Dict[str, Any], matrix: Sequence[Sequence[int]],
                  layers: Sequence[int], positions: Dict[str, List[Any]],
                  table: Dict[int, str], spec: Dict[str, Any],
                  layer_names: Optional[Dict[str, str]] = None) -> Dict[str, Any]:
    """A skim-ready keymap.json, as `qmk c2json` shapes it, from `numeric`.

    `matrix[i]` is layout key i's `[row, col]`. `layers` are the firmware
    indices the config describes, in its order (skim pairs the keymap's
    layers with the config's positionally); each comes from the keyboard's
    keymap at that index.
    """
    rows = int(numeric.get("rows") or 1 + max(row for row, _ in matrix))
    cols = int(numeric.get("cols") or 1 + max(col for _, col in matrix))
    count = int(numeric["layers"])
    keycodes = numeric["keycodes"]
    if any(not (0 <= row < rows and 0 <= col < cols) for row, col in matrix):
        raise RenderError(f"the keymap's {rows}x{cols} matrix does not hold the layout's keys")
    if len(keycodes) != count * rows * cols:
        raise RenderError(f"the keymap has {len(keycodes)} keycodes, not "
                          f"{count} layers of {rows}x{cols}")
    out = []
    for layer in layers:
        if not 0 <= layer < count:
            raise RenderError(f"the config describes layer {layer} but the keyboard "
                              f"has {count} layers")
        base = layer * rows * cols
        keys = [spell(int(keycodes[base + row * cols + col]), (layer, row, col),
                      positions, table, spec, layer_names)
                for row, col in matrix]
        if all(key in EMPTY_KEYS for key in keys):
            raise RenderError(f"layer {layer} is empty on the keyboard; skim would "
                              f"drop it and misname the layers after it")
        out.append(keys)
    return {"keyboard": "svalboard", "keymap": "keyboard", "layout": "LAYOUT",
            "layers": out}


# --- Drawing ----------------------------------------------------------------

def _read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text())
    except (OSError, ValueError) as e:
        raise RenderError(f"cannot read {path}: {e}")


def _config_layers(config: Path) -> List[int]:
    import yaml  # pyright: ignore[reportMissingModuleSource] -- skim's environment has it

    try:
        loaded = yaml.safe_load(config.read_text())
        return [int(layer["index"]) for layer in loaded["keyboard"]["layers"]]
    except (OSError, yaml.YAMLError, KeyError, TypeError, ValueError) as e:
        raise RenderError(f"cannot read the layers from {config}: {e!r}")


class _Captured:
    """fds 1 and 2 sent to a file while skim runs, so a failure stays one line."""

    def __init__(self, log: Path) -> None:
        self.log = log

    def __enter__(self) -> "_Captured":
        sys.stdout.flush()
        sys.stderr.flush()
        self.saved = (os.dup(1), os.dup(2))
        self.file = self.log.open("w")
        os.dup2(self.file.fileno(), 1)
        os.dup2(self.file.fileno(), 2)
        return self

    def __exit__(self, *_: Any) -> None:
        sys.stdout.flush()
        sys.stderr.flush()
        os.dup2(self.saved[0], 1)
        os.dup2(self.saved[1], 2)
        for fd in self.saved:
            os.close(fd)
        self.file.close()

    def tail(self, count: int = 3) -> str:
        """The output's last few meaningful lines, joined into one."""
        try:
            text = self.log.read_text(errors="replace")
        except OSError:
            return ""
        lines = [line.strip() for line in re.sub(r"\x1b\[[0-9;]*m", "", text).splitlines()]
        lines = [line for line in lines
                 if line and not line.startswith("For further information")]
        return " ".join(lines[-count:])[-400:]


def render(bundle: Path, keymap_file: Path, out: Path) -> int:
    """Draw the page into `out`; the number of layers drawn."""
    manifest = _read_json(bundle / "manifest.json")
    table = {int(value, 16): spelled
             for value, spelled in _read_json(bundle / "spelling.json").items()}
    positions = _read_json(bundle / "positions.json")
    spec = _read_json(bundle / "keycodes.json")
    config = bundle / "skim-config.yaml"
    layer_names = _read_json(bundle / "layers.json")
    keymap = layout_keymap(_read_json(keymap_file), manifest["matrix"],
                           _config_layers(config), positions, table, spec, layer_names)

    sys.path.insert(0, str(bundle))
    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        (work / "keymap.json").write_text(json.dumps(keymap))
        captured = _Captured(work / "skim.log")
        try:
            with captured:
                import page_build

                images = page_build.layer_images(work / "keymap.json", config, work)
                html = page_build.page(images, manifest)
                page_build.write_page(out, html, bundle / "icons.ttf")
        except SystemExit as e:
            raise RenderError(str(e.code) if isinstance(e.code, str)
                              else captured.tail() or f"exit {e.code}")
        except subprocess.CalledProcessError as e:
            raise RenderError(f"skim generate failed (exit {e.returncode}): {captured.tail()}")
        except Exception as e:  # noqa: BLE001 -- any failure becomes the one line
            detail = captured.tail()
            raise RenderError(f"{type(e).__name__}: {e}" + (f" ({detail})" if detail else ""))
    return len(images)


def main(argv: Optional[Sequence[str]] = None) -> None:
    parser = argparse.ArgumentParser(description="Draw the overlay page from a numeric keymap.")
    parser.add_argument("--bundle", type=Path, default=HERE)
    parser.add_argument("--keymap", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)

    def one_line_error(message: str) -> None:
        print(f"render_page: {message}", file=sys.stderr)
        sys.exit(2)

    parser.error = one_line_error  # type: ignore[method-assign]
    args = parser.parse_args(argv)
    try:
        drawn = render(args.bundle.resolve(), args.keymap, args.out)
    except Exception as e:  # noqa: BLE001 -- RenderError, or a malformed bundle/keymap
        message = " ".join(str(e).split()) if isinstance(e, RenderError) \
            else f"{type(e).__name__}: {' '.join(str(e).split())}"
        print(f"render_page: {message}", file=sys.stderr)
        sys.exit(1)
    print(f"render_page: {drawn} layers -> {args.out}")


if __name__ == "__main__":
    main()
