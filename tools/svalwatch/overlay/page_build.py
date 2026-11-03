"""The overlay page, from a keymap and a merged skim config.

Shared by build_assets.py (the page that ships with the overlay, drawn at
build time from keymap.c) and render_page.py (the page drawn on the Mac
from the keymap read off the keyboard), so both draw the same page. It
ships in the render bundle next to page.html, page.css and page.js, which
it reads from its own directory.

Runs under a Python that has skim installed (it calls `python -m skim`
with its own interpreter). Kept Python 3.9-compatible.
"""

from __future__ import annotations

import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import yaml  # pyright: ignore[reportMissingModuleSource] -- skim's environment has it

HERE = Path(__file__).resolve().parent
# Copied next to index.html, as they are: the page links them.
PAGE_FILES = ("page.css", "page.js")


def merge(base: Any, over: Any) -> Any:
    """Mappings merge key by key; anything else (including null) replaces."""
    if isinstance(base, dict) and isinstance(over, dict):
        out: Dict[Any, Any] = dict(base)
        for key, value in over.items():
            out[key] = merge(base[key], value) if key in base else value
        return out
    return over


def skim_config(base: Path, over: Path, work: Path) -> Tuple[Path, Dict[str, Any]]:
    """`over` merged over `base` (both skim config YAML), written to `work`."""
    config = merge(yaml.safe_load(base.read_text()), yaml.safe_load(over.read_text()))
    out = work / "skim-config.yaml"
    out.write_text(yaml.safe_dump(config, sort_keys=False, allow_unicode=True))
    return out, config


def layer_images(keymap: Path, config: Path, work: Path) -> Dict[int, str]:
    """skim's image of every layer, by layer index."""
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
        sys.exit("overlay page: skim produced no layer images")
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
        sys.exit("overlay page: the layer images have no key tags; this skim "
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


def manifest(config: Dict[str, Any], layout: List[Dict[str, Any]]) -> Dict[str, Any]:
    """Layer names, and the matrix position of each layout key.

    `layout` is the keyboard's layout from QMK's info.json (a list of
    keys, each with its `matrix` `[row, col]`).
    """
    layers = {
        str(layer["index"]): {"name": layer.get("name") or f"Layer {layer['index']}",
                              "variant": layer.get("variant")}
        for layer in config["keyboard"]["layers"]
    }
    return {"layers": layers, "matrix": [key["matrix"] for key in layout]}


def page(images: Dict[int, str], page_manifest: Dict[str, Any]) -> str:
    """page.html with the layer images and the manifest filled in."""
    html = (HERE / "page.html").read_text()
    html = html.replace("<!--LAYERS-->", "\n".join(inline(n, svg) for n, svg in images.items()))
    return html.replace("<!--MANIFEST-->", json.dumps(page_manifest))


def write_page(out: Path, html: str, icon_font: Optional[Path]) -> bool:
    """index.html plus the files it links into `out`; False without icons."""
    out.mkdir(parents=True, exist_ok=True)
    (out / "index.html").write_text(html)
    for name in PAGE_FILES:
        shutil.copyfile(HERE / name, out / name)
    if icon_font is None or not icon_font.exists():
        return False
    shutil.copyfile(icon_font, out / "icons.ttf")
    return True
