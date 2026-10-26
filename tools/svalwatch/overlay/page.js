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

const state = { active: 1, defaultLayers: 1, pressed: [], connected: false, pointer: null };

// Svalboard's DPI steps (dpi_choices in keyboards/svalboard/svalboard.c):
// one meter pip each.
const DPI_CHOICES = [200, 400, 600, 800, 1200, 1600, 2400, 3200, 4800, 6400, 12000];
const GLYPH_POINTER = "\u{F01C0}"; // nf-md-cursor_default
const GLYPH_SCROLL = "\u{F1552}"; // nf-md-mouse_move_vertical
const GLYPH_SNIPER = "\u{F01A4}"; // nf-md-crosshairs_gps
const GLYPH_BOOST = "\u{F14DE}"; // nf-md-rocket_launch

const pointersEl = document.querySelector(".pointers");
const footerEl = document.querySelector(".footer");
const factorEl = document.querySelector(".factor");
const factorGlyph = document.querySelector(".factor-glyph");
const factorText = document.querySelector(".factor-text");
const trackballs = {};
for (const el of document.querySelectorAll(".trackball")) {
  const meter = el.querySelector(".meter");
  DPI_CHOICES.forEach((_, i) => {
    const pip = document.createElement("i");
    pip.style.height = `${4 + i * 0.8}px`; // rising, like signal bars
    meter.appendChild(pip);
  });
  trackballs[el.dataset.side] = {
    el,
    mode: el.querySelector(".mode"),
    pips: [...meter.children],
    dpi: el.querySelector(".dpi b"),
  };
}
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

// The highest DPI step at or below `dpi` (a value off the list still
// lands on a sensible pip).
function dpiStep(dpi) {
  let step = 0;
  DPI_CHOICES.forEach((choice, i) => {
    if (dpi >= choice) step = i;
  });
  return step;
}

function gcd(a, b) {
  return b === 0 ? a : gcd(b, a % b);
}

function renderPointer() {
  const p = state.pointer;
  const shown = Boolean(p && state.connected);
  if (pointersEl.hidden === shown) {
    pointersEl.hidden = !shown;
    reportSize(); // the footer changes the panel's height
  }
  if (!shown) return;

  // Sniper keys divide both trackballs' motion, boost keys multiply it
  // (absent from version 2 firmware: 1). Shown as one reduced factor, and
  // applied to both DPIs, which then read as the speed the trackballs move
  // at rather than the sensors' setting.
  const sniper = p.sniper || 1;
  const boost = p.boost || 1;
  const common = gcd(boost, sniper);
  const num = boost / common;
  const den = sniper / common;
  const scaled = num !== den;
  factorEl.hidden = !scaled;
  footerEl.classList.toggle("scaled", scaled && num < den);
  footerEl.classList.toggle("boosted", scaled && num > den);
  factorGlyph.textContent = num > den ? GLYPH_BOOST : GLYPH_SNIPER;
  factorText.textContent = den === 1 ? `×${num}` : `×${num}/${den}`;

  for (const side of ["left", "right"]) {
    const t = trackballs[side];
    const { scroll } = p[side];
    const dpi = Math.round((p[side].dpi * num) / den);
    const step = dpiStep(dpi);
    t.el.classList.toggle("scrolling", scroll);
    t.mode.textContent = scroll ? GLYPH_SCROLL : GLYPH_POINTER;
    t.pips.forEach((pip, i) => pip.classList.toggle("on", i <= step));
    t.dpi.textContent = String(dpi);
  }
  footerEl.classList.toggle("locked", p.scroll_key);
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
    if (!on) state.pointer = null; // the next board reports its own
    render();
    renderPointer();
  },
  layer(active, defaultLayers) {
    state.active = active;
    state.defaultLayers = defaultLayers;
    render();
  },
  // svalwatch's "pointer" event, as svalwatch prints it.
  pointer(event) {
    state.pointer = event;
    renderPointer();
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
