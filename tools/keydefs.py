#!/usr/bin/env python3
"""The key-definitions fingerprint: has the custom keycode numbering changed?

    keydefs.py [--repo ROOT] fingerprint
    keydefs.py [--repo ROOT] matches FILE

The overlay's render bundle maps keycode VALUES to the keymap's spellings
(tools/svalwatch/overlay/spelling.py), and the values of every userspace
custom keycode and layer come from where its name sits in a list:

- `enum custom_keycodes` (users/townk/townk_keycodes.h),
- `enum layer_names` (users/townk/townk_layers.h),
- the keymap's sval.json `customKeycodes`, which the Svalboard module
  numbers in order.

Insert or reorder one and every later value moves, so a bundle built
before that spells moved keys as other keys. The fingerprint is a 32-bit
hash over those three ordered name lists, together with every explicit
initializer in the two enums (`= RANGE_START`, `= LAST_LAYER - 1`, ...):
renumbering happens iff it changes. The firmware build compiles it in
(users/townk/rules.mk, `TOWNK_KEYDEFS_FP`) and answers it on request
(townk_announce.c); the bundle build writes it to keydefs.json; the
overlay draws only when the two agree.

`fingerprint` prints it as `0xXXXXXXXX`. `matches FILE` compares it with
a bundle's keydefs.json: exit 0 when equal, 1 when different or FILE is
missing or unreadable (one line on stdout says which).

What it cannot see: a change to what a base expression means (redefining
RANGE_START or LAST_LAYER) leaves the text, and so the fingerprint, as it
was. Those live in the module and the userspace headers and move with a
firmware update, not with keymap edits.

Stdlib only, Python 3.9 compatible; run with `python3 -I`.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

REPO = Path(__file__).resolve().parent.parent
KEYCODES_HEADER = "users/townk/townk_keycodes.h"
KEYCODES_ENUM = "custom_keycodes"
LAYERS_HEADER = "users/townk/townk_layers.h"
LAYERS_ENUM = "layer_names"
SVAL_JSON = "keyboards/svalboard/keymaps/townk/sval.json"


class KeydefsError(ValueError):
    """A source the fingerprint cannot be read from as it stands."""


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def enum_members(header: str, enum: str) -> List[Tuple[str, Optional[str]]]:
    """`enum <enum> { ... }`'s members in order: `(name, initializer)`.

    The initializer is the text after `=`, whitespace collapsed, or None.
    A preprocessor line inside the body (an `#if` branch) is refused: the
    member list would depend on the build's defines, which this cannot
    see, so a fingerprint from it could miss a renumbering.
    """
    text = _strip_comments(header)
    found = re.search(r"\benum\s+" + re.escape(enum) + r"\s*\{(.*?)\}", text, flags=re.S)
    if not found:
        raise KeydefsError(f"no `enum {enum}` found")
    body = found.group(1)
    if re.search(r"(?m)^\s*#", body):
        raise KeydefsError(f"`enum {enum}` has preprocessor lines among its members; "
                           "keep its member list unconditional")
    members: List[Tuple[str, Optional[str]]] = []
    for member in body.split(","):
        name, eq, init = member.partition("=")
        name = name.strip()
        if not name and not eq:
            continue  # the trailing comma
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            raise KeydefsError(f"cannot read `enum {enum}` member {member.strip()!r}")
        if eq and not init.strip():
            raise KeydefsError(f"`enum {enum}` member {name} has an empty initializer")
        members.append((name, " ".join(init.split()) if eq else None))
    if not members:
        raise KeydefsError(f"`enum {enum}` has no members")
    return members


def enum_names(header: str, enum: str) -> List[str]:
    """Just the member names of `enum <enum>`, in order."""
    return [name for name, _ in enum_members(header, enum)]


def sval_keycodes(sval_json: str) -> List[str]:
    """The names in sval.json's `customKeycodes`, in order."""
    try:
        data = json.loads(sval_json)
    except ValueError as e:
        raise KeydefsError(f"sval.json is not JSON: {e}") from None
    keycodes = data.get("customKeycodes", []) if isinstance(data, dict) else None
    if not isinstance(keycodes, list):
        raise KeydefsError("sval.json `customKeycodes` is not a list")
    names = []
    for entry in keycodes:
        name = entry.get("name") if isinstance(entry, dict) else None
        if not isinstance(name, str) or not name:
            raise KeydefsError(f"sval.json `customKeycodes` entry without a name: {entry!r}")
        names.append(name)
    return names


def _line(label: str, members: Sequence[Tuple[str, Optional[str]]]) -> str:
    return label + ":" + ",".join(name if init is None else f"{name}={init}"
                                  for name, init in members)


def canonical(keycodes_header: str, layers_header: str, sval_json: str) -> str:
    """The text the fingerprint hashes: one line per source."""
    return "\n".join([
        _line(KEYCODES_ENUM, enum_members(keycodes_header, KEYCODES_ENUM)),
        _line(LAYERS_ENUM, enum_members(layers_header, LAYERS_ENUM)),
        _line("customKeycodes", [(name, None) for name in sval_keycodes(sval_json)]),
    ]) + "\n"


def fingerprint_of(keycodes_header: str, layers_header: str, sval_json: str) -> int:
    """The first 32 bits (big-endian) of the canonical text's SHA-256."""
    digest = hashlib.sha256(canonical(keycodes_header, layers_header, sval_json).encode())
    return int.from_bytes(digest.digest()[:4], "big")


def fingerprint(repo: Path = REPO) -> int:
    """The fingerprint of the userspace at `repo`."""
    try:
        return fingerprint_of((repo / KEYCODES_HEADER).read_text(),
                              (repo / LAYERS_HEADER).read_text(),
                              (repo / SVAL_JSON).read_text())
    except OSError as e:
        raise KeydefsError(f"cannot read {e.filename}: {e.strerror}") from None


def format_fp(value: int) -> str:
    return f"0x{value:08X}"


def read_bundle_fp(path: Path) -> Optional[int]:
    """A keydefs.json's fingerprint, or None if it is missing or unreadable."""
    try:
        text = json.loads(path.read_text()).get("fingerprint")
        return int(text, 16) if isinstance(text, str) else None
    except (OSError, ValueError, AttributeError):
        return None


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="The userspace's key-definitions fingerprint.")
    parser.add_argument("--repo", type=Path, default=REPO)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("fingerprint", help="print it as 0xXXXXXXXX")
    matches = sub.add_parser("matches", help="compare it with a bundle's keydefs.json")
    matches.add_argument("file", type=Path)
    args = parser.parse_args(argv)
    try:
        ours = fingerprint(args.repo)
    except KeydefsError as e:
        print(f"keydefs: {e}", file=sys.stderr)
        return 2
    if args.command == "fingerprint":
        print(format_fp(ours))
        return 0
    theirs = read_bundle_fp(args.file)
    if theirs is None:
        print(f"keydefs: no fingerprint in {args.file} (source {format_fp(ours)})")
        return 1
    if theirs != ours:
        print(f"keydefs: {args.file} has {format_fp(theirs)}, the source {format_fp(ours)}")
        return 1
    print(f"keydefs: {args.file} matches the source ({format_fp(ours)})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
