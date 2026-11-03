"""The overlay's exact-spelling table: keycode value -> keymap.c spelling.

The overlay reads the keymap off the keyboard as numbers, but skim's
config is keyed on the spellings keymap.c uses (`CKC_BSPC`,
`LSFT(KC_TAB)`, ...). This builds, at overlay build time, the table that
turns a number back into that spelling, from two sources:

1. Placed keys, by position. `qmk c2json --no-cpp` gives every key's
   spelling in layout order; the `keymaps` array in the built firmware ELF
   gives the same keys as numbers in `[layer][row][col]` order. The
   layout's matrix positions pair them. This decides the spelling skim
   sees for every key the keymap places.
2. Named keycodes, placed or not: every userspace custom keycode (the
   `custom_keycodes` enum in users/townk/townk_keycodes.h) and every
   `customKeycodes` entry in the keymap's sval.json. Their values come
   from the firmware's own compiler: a probe translation unit, compiled
   with the build's exact flags (the cflags.txt `qmk compile` leaves next
   to the objects), puts each name's value in a section that is read back
   with objcopy. So moving or placing one of these keys never needs an
   overlay rebuild. They only fill in values the keymap does not place.

Needs the ARM binutils the firmware build uses (arm-none-eabi-nm,
-objdump, -objcopy, -gcc); they are looked up the way the qmk CLI does,
in QMK's distribution directory first, then on PATH.

Kept Python 3.9-compatible: the repo's tests import it.
"""

from __future__ import annotations

import os
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, Iterator, List, Optional, Sequence, Tuple

PROBE_SYMBOL = "spelling_probe"


def placed_keys(layout_spellings: Sequence[Optional[Sequence[str]]],
                elf_keymaps: Sequence[Sequence[Sequence[int]]],
                matrix: Sequence[Sequence[int]]) -> Iterator[Tuple[int, int, int, int, str]]:
    """Every placed key as `(layer, row, col, value, spelling)`.

    `layout_spellings[layer]` is that layer's spellings in layout order,
    or None for a layer the keymap does not define (it is skipped).
    `elf_keymaps[layer][row][col]` is the firmware's value there.
    `matrix[i]` is layout key i's `[row, col]`. Layer order, then layout
    order. A shape that does not line up fails, naming both counts.
    """
    if len(layout_spellings) != len(elf_keymaps):
        sys.exit(f"spelling: the keymap has {len(layout_spellings)} layers but "
                 f"the firmware has {len(elf_keymaps)}")
    for layer, (spellings, numbers) in enumerate(zip(layout_spellings, elf_keymaps)):
        if spellings is None:
            continue
        if len(spellings) != len(matrix):
            sys.exit(f"spelling: layer {layer} has {len(spellings)} keys but the "
                     f"layout has {len(matrix)}")
        for spelled, (row, col) in zip(spellings, matrix):
            yield layer, row, col, numbers[row][col], spelled


def pair(layout_spellings: Sequence[Optional[Sequence[str]]],
         elf_keymaps: Sequence[Sequence[Sequence[int]]],
         matrix: Sequence[Sequence[int]]) -> Dict[int, str]:
    """Pair each placed key's spelling with its value, by matrix position.

    See `placed_keys`. A value spelled two ways keeps the first, with a
    warning; `positions` keeps both, by position.
    """
    table: Dict[int, str] = {}
    for layer, _, _, value, spelled in placed_keys(layout_spellings, elf_keymaps, matrix):
        first = table.setdefault(value, spelled)
        if first != spelled:
            print(f"spelling: warning: 0x{value:04X} is spelled {first!r} and "
                  f"{spelled!r} (layer {layer}); keeping {first!r}",
                  file=sys.stderr)
    return table


def positions(layout_spellings: Sequence[Optional[Sequence[str]]],
              elf_keymaps: Sequence[Sequence[Sequence[int]]],
              matrix: Sequence[Sequence[int]]) -> Dict[str, List[Any]]:
    """`{"L,R,C": [value, spelling]}` for every placed key (matrix coordinates).

    The renderer uses a position's spelling when the keyboard still has
    that value there, so a value spelled two ways keeps both spellings.
    """
    return {f"{layer},{row},{col}": [value, spelled]
            for layer, row, col, value, spelled
            in placed_keys(layout_spellings, elf_keymaps, matrix)}


def check_spec(layout_spellings: Sequence[Optional[Sequence[str]]],
               elf_keymaps: Sequence[Sequence[Sequence[int]]],
               matrix: Sequence[Sequence[int]], spec: Dict[str, Any]) -> None:
    """Fail unless every placed spec name (or alias) has its spec value.

    Cross-checks the pairing itself: a layer or key paired with the wrong
    firmware position shows up as a basic keycode with another value.
    """
    by_name: Dict[str, int] = {}
    for value, keycode in spec["keycodes"].items():
        for name in [keycode["key"], *keycode.get("aliases", [])]:
            by_name[name] = int(value, 16)
    wrong = [f"spelling: layer {layer} row {row} col {col}: {spelled} is 0x{value:04X} "
             f"in the firmware but 0x{by_name[spelled]:04X} in the keycode spec"
             for layer, row, col, value, spelled
             in placed_keys(layout_spellings, elf_keymaps, matrix)
             if spelled in by_name and by_name[spelled] != value]
    if wrong:
        sys.exit("\n".join(wrong + ["spelling: the keymap and the firmware do not pair up; "
                                    "is the firmware stale, or skim-config.yaml's layers "
                                    "out of keymap.c's order?"]))


def with_named(placed: Dict[int, str], named: Dict[str, int]) -> Dict[int, str]:
    """The placed table, plus each named keycode whose value it lacks."""
    table = dict(placed)
    for name, value in named.items():
        table.setdefault(value, name)
    return table


def to_json(table: Dict[int, str]) -> Dict[str, str]:
    """`{"0xNNNN": spelling}`, sorted, in the keycode spec's key format."""
    return {f"0x{value:04X}": table[value] for value in sorted(table)}


def enum_names(header: str, enum: str) -> List[str]:
    """The member names of `enum <enum> { ... }`, in order.

    Only the names: their values come from the compiler (see
    `compiled_values`), never from re-implementing C's enum rules here.
    """
    text = re.sub(r"/\*.*?\*/", " ", header, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    found = re.search(r"\benum\s+" + re.escape(enum) + r"\s*\{(.*?)\}", text, flags=re.S)
    if not found:
        sys.exit(f"spelling: no `enum {enum}` found")
    names = []
    for member in found.group(1).split(","):
        name = member.split("=", 1)[0].strip()
        if not name:
            continue
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            sys.exit(f"spelling: cannot read `enum {enum}` member {member.strip()!r}")
        names.append(name)
    return names


def toolchain_path() -> str:
    """PATH with QMK's distribution bin first, as the qmk CLI sets it."""
    distrib = os.environ.get("QMK_DISTRIB_DIR")
    if not distrib:
        if sys.platform == "darwin":
            distrib = str(Path.home() / "Library/Application Support/qmk")
        else:
            data = os.environ.get("XDG_DATA_HOME") or str(Path.home() / ".local/share")
            distrib = str(Path(data) / "qmk")
    path = os.environ.get("PATH", "")
    if Path(distrib, "bin").is_dir():
        path = str(Path(distrib, "bin")) + os.pathsep + path
    return path


def _run(argv: List[str], cwd: Optional[Path] = None) -> str:
    env = dict(os.environ, PATH=toolchain_path())
    try:
        done = subprocess.run(argv, cwd=cwd, env=env, check=False,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    except FileNotFoundError:
        sys.exit(f"spelling: {argv[0]} not found (the ARM toolchain the firmware "
                 "build uses; install it with `qmk setup`)")
    if done.returncode != 0:
        sys.exit(f"spelling: {' '.join(argv[:2])} failed:\n{done.stderr.strip()}")
    return done.stdout


def _section_bytes(obj: Path, symbol: str, section: Optional[str] = None) -> bytes:
    """The bytes of `symbol` in `obj` (an ELF), read through its section.

    `section` names it when it is known (an unlinked object, where every
    section starts at 0); otherwise it is the section whose addresses
    hold the symbol.
    """
    found = None
    for line in _run(["arm-none-eabi-nm", "-S", str(obj)]).splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[3] == symbol:
            found = (int(parts[0], 16), int(parts[1], 16))
    if found is None:
        sys.exit(f"spelling: no `{symbol}` symbol in {obj}")
    address, size = found
    for line in _run(["arm-none-eabi-objdump", "-h", str(obj)]).splitlines():
        parts = line.split()
        if len(parts) >= 4 and parts[0].isdigit():
            name, length, vma = parts[1], int(parts[2], 16), int(parts[3], 16)
            holds = vma <= address and address + size <= vma + length
            if (name == section) if section else holds:
                with tempfile.TemporaryDirectory() as tmp:
                    dump = Path(tmp) / "section.bin"
                    _run(["arm-none-eabi-objcopy", "-O", "binary", "--only-section",
                          name, str(obj), str(dump)])
                    data = dump.read_bytes()
                return data[address - vma:address - vma + size]
    sys.exit(f"spelling: no section holds `{symbol}` in {obj}")


def _uint16s(data: bytes) -> List[int]:
    return [int.from_bytes(data[i:i + 2], "little") for i in range(0, len(data), 2)]


def elf_keymaps(elf: Path, rows: int, cols: int) -> List[List[List[int]]]:
    """The firmware's `keymaps[layer][row][col]` (little-endian uint16)."""
    if not elf.is_file():
        sys.exit(f"spelling: no firmware ELF at {elf}; build the firmware first (`make`)")
    values = _uint16s(_section_bytes(elf, "keymaps"))
    per_layer = rows * cols
    if not values or len(values) % per_layer:
        sys.exit(f"spelling: `keymaps` in {elf} holds {len(values)} keycodes, not a "
                 f"whole number of {rows}x{cols} layers")
    return [[values[start + row * cols:start + (row + 1) * cols] for row in range(rows)]
            for start in range(0, len(values), per_layer)]


def compiled_values(qmk_home: Path, cflags: Path, headers: Sequence[str],
                    names: Sequence[str]) -> Dict[str, int]:
    """Each name's value, as the firmware build's compiler sees it.

    Compiles `const uint16_t spelling_probe[] = {NAME, ...};` after
    `headers`, with the build's own flags (from `cflags`, relative to
    `qmk_home`), and reads the array back out of the object file. A name
    the firmware does not define fails the build, loudly.
    """
    if not cflags.is_file():
        sys.exit(f"spelling: no {cflags}; build the firmware first (`make`)")
    if not names:
        return {}
    flags = shlex.split(cflags.read_text())
    with tempfile.TemporaryDirectory() as tmp:
        probe = Path(tmp) / "spelling_probe.c"
        obj = Path(tmp) / "spelling_probe.o"
        probe.write_text(
            "".join(f"#include {header}\n" for header in headers)
            + f"const uint16_t {PROBE_SYMBOL}[] = {{\n"
            + "".join(f"    {name},\n" for name in names)
            + "};\n")
        _run(["arm-none-eabi-gcc", *flags, "-c", str(probe), "-o", str(obj)], cwd=qmk_home)
        # -fdata-sections (in the build's flags) gives the array its own section.
        values = _uint16s(_section_bytes(obj, PROBE_SYMBOL, f".rodata.{PROBE_SYMBOL}"))
    if len(values) != len(names):
        sys.exit(f"spelling: the probe holds {len(values)} values for {len(names)} names")
    return dict(zip(names, values))
