"""Build svaloverlay's page from this userspace's keymap.

    python build_assets.py --repo ROOT --qmk-home QMK --out DIR

Run it with a Python that has skim installed (it calls `python -m skim`
with its own interpreter); `make overlay-assets` picks one. Steps:

1. `qmk c2json` the keymap, as the pre-commit hook does for docs/.
2. Merge skim-overlay.yaml over the keymap's skim-config.yaml.
3. `skim generate` every layer image with that config.
4. Inline the images into page.html, together with a manifest of layer
   names and the Svalboard layout's matrix position for each keymap
   index (from QMK's keyboards/svalboard/info.json), and copy the page's
   CSS, JS and icon font next to it.

skim must tag keys (`<g data-key="N">`, skim's feat/key-tags); the build
fails if the images have no tags, since the overlay could not highlight.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

import yaml  # pyright: ignore[reportMissingModuleSource] -- skim's environment has it

HERE = Path(__file__).resolve().parent
KEYMAP = "keyboards/svalboard/keymaps/townk"
QMK_KEYBOARD = "svalboard/trackball/pmw3389/right"
ICON_FONT = Path.home() / "Library/Fonts/SymbolsNerdFontMono-Regular.ttf"


def merge(base: Any, over: Any) -> Any:
    """Mappings merge key by key; anything else (including null) replaces."""
    if isinstance(base, dict) and isinstance(over, dict):
        out: dict[Any, Any] = dict(base)
        for key, value in over.items():
            out[key] = merge(base[key], value) if key in base else value
        return out
    return over


def keymap_json(repo: Path, work: Path) -> Path:
    out = work / "keymap.json"
    with out.open("w") as f:
        subprocess.run(
            ["qmk", "c2json", "-kb", QMK_KEYBOARD, "-km", "townk", "--no-cpp",
             str(repo / KEYMAP / "keymap.c")],
            check=True, stdout=f)
    return out


def skim_config(repo: Path, work: Path) -> tuple[Path, dict[str, Any]]:
    base = yaml.safe_load((repo / KEYMAP / "skim-config.yaml").read_text())
    config = merge(base, yaml.safe_load((HERE / "skim-overlay.yaml").read_text()))
    out = work / "skim-config.yaml"
    out.write_text(yaml.safe_dump(config, sort_keys=False, allow_unicode=True))
    return out, config


def layer_images(keymap: Path, config: Path, work: Path) -> dict[int, str]:
    images = work / "images"
    images.mkdir()
    subprocess.run(
        [sys.executable, "-m", "skim", "generate", "--keymap", str(keymap),
         "--config", str(config), "--output-dir", str(images),
         "--layer", "all-layers", "--no-symbols", "--force"],
        check=True)
    found = {}
    for svg in images.glob("keymap-layer-*.svg"):
        found[int(svg.stem.rsplit("-", 1)[1])] = svg.read_text()
    if not found:
        sys.exit("build_assets: skim produced no layer images")
    return dict(sorted(found.items()))


def inline(layer: int, svg: str) -> str:
    """One layer image, ready to sit in the page next to the others.

    Drops the XML prolog and skim's page header (the layer title and the
    logo, everything drawn before the first key; the overlay has its own
    header), prefixes every id (and every reference to one) so the
    images' definitions cannot clash, and marks the root with its layer
    for page.js.
    """
    first_key = svg.find("<g data-key=")
    if first_key < 0:
        sys.exit("build_assets: the layer images have no key tags; this skim "
                 "predates them (see skim's feat/key-tags)")
    header, keys = svg[:first_key], svg[first_key:]
    header = re.sub(r"<text\b.*?</text>\s*", "", header, flags=re.S)
    header = re.sub(r"<image\b[^>]*/>\s*", "", header, flags=re.S)
    svg = re.sub(r"^<\?xml[^>]*\?>\s*", "", header) + keys
    prefix = f"l{layer}-"
    svg = re.sub(r'\bid="([^"]+)"', lambda m: f'id="{prefix}{m.group(1)}"', svg)
    svg = re.sub(r"url\(#([^)]+)\)", lambda m: f"url(#{prefix}{m.group(1)})", svg)
    svg = re.sub(r'href="#([^"]+)"', lambda m: f'href="#{prefix}{m.group(1)}"', svg)
    return re.sub(r"<svg\b", f'<svg data-layer="{layer}"', svg, count=1)


def manifest(config: dict[str, Any], qmk_home: Path) -> dict[str, Any]:
    info = json.loads((qmk_home / "keyboards/svalboard/info.json").read_text())
    layout = next(iter(info["layouts"].values()))["layout"]
    layers = {
        str(layer["index"]): {"name": layer.get("name") or f"Layer {layer['index']}",
                              "variant": layer.get("variant")}
        for layer in config["keyboard"]["layers"]
    }
    return {"layers": layers, "matrix": [key["matrix"] for key in layout]}


def main() -> None:
    parser = argparse.ArgumentParser(description="Build svaloverlay's page from this userspace's keymap.")
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--qmk-home", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        work = Path(tmp)
        keymap = keymap_json(args.repo, work)
        config_path, config = skim_config(args.repo, work)
        images = layer_images(keymap, config_path, work)

    page = (HERE / "page.html").read_text()
    page = page.replace("<!--LAYERS-->", "\n".join(inline(n, svg) for n, svg in images.items()))
    page = page.replace("<!--MANIFEST-->", json.dumps(manifest(config, args.qmk_home)))

    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "index.html").write_text(page)
    for name in ("page.css", "page.js"):
        shutil.copyfile(HERE / name, args.out / name)
    if ICON_FONT.exists():
        shutil.copyfile(ICON_FONT, args.out / "icons.ttf")
    print(f"build_assets: {len(images)} layers -> {args.out}")


if __name__ == "__main__":
    main()
