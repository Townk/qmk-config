// svaloverlay's page logic. svaloverlay (OverlayWindow.swift) calls the
// overlay.* functions with what svalwatch's session reports; the page
// answers through the "overlay" message handler: "ready" once, then
// "size" whenever the panel's size changes, so the window can fit it.
//
// The manifest (inlined by build_assets.py) holds each layer's name and
// the Svalboard layout's matrix position for every keymap index, which is
// how a pressed [row, col] finds its key: skim tags each key's group with
// data-key="<keymap index>".
"use strict";

const manifest = JSON.parse(document.getElementById("manifest").textContent);
const panel = document.querySelector(".panel");
const nameEl = document.querySelector(".layer-name");
const variantEl = document.querySelector(".layer-variant");
const svgs = new Map(
  [...document.querySelectorAll(".keyboard svg")].map((svg) => [Number(svg.dataset.layer), svg]),
);
const indexAt = new Map(manifest.matrix.map(([row, col], index) => [`${row},${col}`, index]));

const state = { active: 1, defaultLayers: 1, pressed: [], connected: false };
let shown = null;

function post(message) {
  try {
    window.webkit.messageHandlers.overlay.postMessage(message);
  } catch (e) {
    // Opened outside svaloverlay (a browser, while working on the page).
  }
}

// The highest layer that is on and has an image; QMK's layer-0
// fallback when both masks are empty.
function pickLayer() {
  const all = (state.active | state.defaultLayers) >>> 0;
  for (let bit = 31; bit >= 0; bit--) {
    if ((all >>> bit) & 1 && svgs.has(bit)) return bit;
  }
  return svgs.has(0) ? 0 : svgs.keys().next().value;
}

function render() {
  const layer = pickLayer();
  if (layer !== shown) {
    for (const [index, svg] of svgs) svg.classList.toggle("hidden", index !== layer);
    const info = manifest.layers[String(layer)] || { name: `Layer ${layer}` };
    nameEl.textContent = info.name;
    variantEl.textContent = info.variant || "";
    shown = layer;
  }
  const svg = svgs.get(layer);
  const held = new Set(state.pressed.map(([row, col]) => indexAt.get(`${row},${col}`)));
  for (const g of svg.querySelectorAll("g[data-key]")) {
    g.classList.toggle("pressed", held.has(Number(g.dataset.key)));
  }
  panel.classList.toggle("disconnected", !state.connected);
  if (!state.connected) {
    nameEl.textContent = "Svalboard disconnected";
    variantEl.textContent = "";
    shown = null;
  }
}

// Crop every layer image to its own keys. skim's reserve_space gives
// the layers one keyboard geometry, but each image still places it under
// a page header of its own height, so the same frame starts at a
// different y in each; cropping per image lines them up exactly. The
// margin keeps the layer badges, which sit just outside their keys.
// The first configure() measures before render() hides any image.
function keyBounds(svg) {
  let box = null;
  for (const g of svg.querySelectorAll("g[data-key]")) {
    const b = g.getBBox();
    box = box
      ? {
          x0: Math.min(box.x0, b.x),
          y0: Math.min(box.y0, b.y),
          x1: Math.max(box.x1, b.x + b.width),
          y1: Math.max(box.y1, b.y + b.height),
        }
      : { x0: b.x, y0: b.y, x1: b.x + b.width, y1: b.y + b.height };
  }
  return box;
}

// Each image's crop, measured once: hidden images have no bbox, and
// configure() runs again whenever the display changes.
let viewBoxes = null;

function crop(width) {
  if (!viewBoxes) {
    viewBoxes = new Map();
    for (const [layer, svg] of svgs) {
      const box = keyBounds(svg);
      const pad = (box.x1 - box.x0) * 0.02;
      const viewBox = [box.x0 - pad, box.y0 - pad, box.x1 - box.x0 + 2 * pad, box.y1 - box.y0 + 2 * pad];
      svg.setAttribute("viewBox", viewBox.join(" "));
      viewBoxes.set(layer, viewBox);
    }
  }
  for (const [layer, svg] of svgs) {
    const viewBox = viewBoxes.get(layer);
    svg.setAttribute("width", String(width));
    svg.setAttribute("height", String((width * viewBox[3]) / viewBox[2]));
  }
}

// The panel's own horizontal padding and border, so the drawing fills
// exactly the width the panel was given.
function panelChrome() {
  const css = getComputedStyle(panel);
  return ["paddingLeft", "paddingRight", "borderLeftWidth", "borderRightWidth"]
    .map((side) => parseFloat(css[side]))
    .reduce((a, b) => a + b, 0);
}

function reportSize() {
  const rect = panel.getBoundingClientRect();
  post({ type: "size", w: Math.ceil(rect.width), h: Math.ceil(rect.height) });
}

window.overlay = {
  configure({ panelWidth }) {
    crop(panelWidth - panelChrome());
    render();
    document.fonts.ready.then(reportSize);
  },
  connected(on) {
    state.connected = on;
    render();
  },
  layer(active, defaultLayers) {
    state.active = active;
    state.defaultLayers = defaultLayers;
    render();
  },
  keys(pressed) {
    state.pressed = pressed;
    render();
  },
  onTop(on) {
    panel.classList.toggle("on-top", on);
  },
};

document.fonts.ready.then(() => post({ type: "ready" }));
