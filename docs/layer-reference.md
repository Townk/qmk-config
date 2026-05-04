# Layer Reference

This document provides a complete visual reference for all keyboard layers in
the Svalboard COLEMAK-DH configuration.

## Layer Overview

| Layer | Name | Activation | Purpose |
| ----- | :---: | ---------- | ------- |
| 0 | ![BASE](https://img.shields.io/badge/BASE-347156) | Default | COLEMAK-DH letters with common symbols |
| 1 | ![QWT](https://img.shields.io/badge/QWT-2F898B) | Right ring finger on the System layer | QWERTY letters with common symbols |
| 2 | ![GAM1](https://img.shields.io/badge/GAM1-355A93) | Right middle finger on the System layer | WASD-driven layout on the left hand |
| 3 | ![GAM2](https://img.shields.io/badge/GAM2-0C7595) | Hold left thumb down while in the GAM1 layer | Number keys for the WASD-driven layout on the left hand |
| 4 | ![NAV](https://img.shields.io/badge/NAV-D96702) | Hold left thumb inward (Backspace) | Vim-style navigation and window management |
| 5 | ![NUM](https://img.shields.io/badge/NUM-41687E) | Hold right thumb inward (Space) | Numeric keypad with operators |
| 6 | ![SYM](https://img.shields.io/badge/SYM-FF7C4D) | Hold left thumb outward (Tab) | Programming symbols and brackets |
| 7 | ![FUN](https://img.shields.io/badge/FUN-605181) | Hold right thumb outward (Back-tab) | Function keys F1-F20 |
| 8 | ![MED](https://img.shields.io/badge/MED-767709) | Hold left thumb knuckle outward | Multimedia keys |
| 14 | ![SYS](https://img.shields.io/badge/SYS-9C2927) | Hold right thumb knuckle outward | System keys (keyboard firmware and OS) |
| 15 | ![MBO](https://img.shields.io/badge/MBO-8C3396) | Auto on trackball movement | Mouse buttons with modifiers |

## Svalboard Key Terminology

Understanding the Svalboard's unique key naming system:

### Finger Clusters

Each half has 4 finger clusters labeled `L1`-`L4` (left) and `R1`-`R4` (right),
numbered from inside to outside.

Keys are named by physical position:

- **C** (Center): Downward press
- **N** (North): Upward flick
- **E** (East): Right lateral movement
- **S** (South): Downward flick (scratching motion)
- **W** (West): Left lateral movement
- **DS** (Double-South): Behind South key, same activation

### Thumb Clusters

Thumb keys are named by the thumb part used:

- **Pad**: Inward thumb motion
- **Nail**: Outward thumb motion
- **Knuckle**: Outward thumb knuckle motion
- **Up**: Upward thumb motion
- **Down**: Downward thumb motion
- **DD** (Double-Down): Press Down key further after initial activation

### Symbol Legend

The layer diagrams below use a mix of [Nerd Font](https://www.nerdfonts.com)
glyphs and Unicode characters to label keys. The full legend is available in
[keymap-symbols.svg](keymap-symbols.svg).

![](keymap-symbols.svg)

---

## Layer 0: BASE

The primary typing layer with COLEMAK-DH letter arrangement and frequently used
symbols.

![](keymap-layer-0.svg)

### Key Features

- **COLEMAK-DH letter layout** optimized for finger rolls
- **Home row modifiers** on Double-South keys (⌃ ⌥ ⌘ ⇧) accessible via downward
  flick
- **Dual-function thumb keys** for layer access (hold) and common keys (tap)

### Thumb Key Functions

- **Left Pad**: Backspace (tap) / `NAV` layer (hold)
- **Left Nail**: Tab (tap) / `SYM` layer (hold)
- **Left Down**: Shift (one-shot tap, hold, or Caps Word on double-tap)
- **Left Knuckle**: `MED` layer
- **Left Up**: Repeat last key
- **Left Double-Down**: Caps Word toggle
- **Right Pad**: Space (tap) / `NUM` layer (hold)
- **Right Nail**: Back-tab (⇧⇥) (tap) / `FUN` layer (hold)
- **Right Down**: Enter
- **Right Knuckle**: `SYS` layer
- **Right Up**: Escape
- **Right Double-Down**: Caps Word toggle

---

## Layer 1: QWT

Standard QWERTY letter arrangement, kept around for situations that assume
the default layout (remote sessions, shared machines, applications with
hard-coded letter-position shortcuts).

![](keymap-layer-1.svg)

### Key Features

- **QWERTY letters** in their familiar positions across both hands
- **Identical thumb cluster** to BASE, so `NAV`, `NUM`, `SYM`, `FUN`, `MED`,
  and `SYS` remain reachable without re-learning thumb access
- **Same home row modifiers** (⌃ ⌥ ⌘ ⇧) on the Double-South keys
- **Same outer-column symbols** as BASE: parentheses, brackets, quote, grave,
  and backslash on the West and East keys
- **Switches the default layer**, so it persists until you switch back from
  the `SYS` layer

### Activation

Tap **R3 Center** (Right Ring Finger) on the **SYS** layer to make QWT the
default layer. Tap **R4 Center** on `SYS` to return to BASE.

---

## Layer 2: GAM1

Gaming-oriented base layer that consolidates movement and action keys onto
the left hand and turns the right thumb into a mouse.

![](keymap-layer-2.svg)

### Key Features

- **WASD movement** on the strong left-hand fingers:
  - `W` on `L2 Center`, `S` on `L2 South` (same middle finger, press vs.
    downward flick)
  - `A` on `L3 Center`, `D` on `L1 Center`
- **Action keys within reach**: `Q`/`E`/`R`/`F`/`T`/`V`/`B`/`G`/`X`/`Z`/`M`
  surrounding WASD on the same hand
- **Modifiers for play**: Shift on `L4 Center` (sprint), Ctrl on `L4 South`
  (crouch), Option on `L4 North`
- **Right thumb mouse cluster**: Pad = left click, Down = right click,
  Nail = scroll up, Knuckle = scroll down
- **Left thumb essentials**: Pad = Space (jump), Nail = Tab, Up = Escape,
  Knuckle = ⌘
- **Auto-mouse layer is disabled** while GAM1 (or GAM2) is active, so
  trackball motion never hijacks the keymap mid-game
- **Hold Left Down** to access **GAM2** for numbers and function keys
- **Double-Down on either thumb** returns to BASE

### Activation

Tap **R2 Center** (Right Middle Finger) on the **SYS** layer to make GAM1
the default layer. Tap the Double-Down on either thumb (or `R4 Center` on
`SYS`) to return to BASE.

---

## Layer 3: GAM2

Numbers and function keys layered on top of GAM1, providing hotbar and
ability slots without disturbing the WASD positions.

![](keymap-layer-3.svg)

### Key Features

- **Numbers `1`-`9`** on the left finger cluster, arranged systematically:
  - South row → `1`, `2`, `3` (outside-to-inside)
  - Center row → `4`, `5`, `6`
  - North row → `7`, `8`, `9`
- **Number `0`** on `L1 East`, within easy reach of the index finger
- **F1-F4** on the Double-South of every left finger (`L4 DS` → F1,
  `L3 DS` → F2, `L2 DS` → F3, `L1 DS` → F4) for quick weapon or ability
  switching
- **F5-F9** on the L4 (pinky) cluster: South = F5, Center = F6,
  North = F7, West = F8, East = F9
- **Right hand keeps its GAM1 bindings**, so mouse aim and click stay
  available while you reach for a number or F-key

### Activation

Hold **Left Thumb Down** while in the **GAM1** layer. Release to fall back
to GAM1.

---

## Layer 4: NAV (Navigation)

Vim-style navigation and macOS window management on the right hand.

![](keymap-layer-4.svg)

### Key Features

- **Vim-style arrows**: H(←) J(↓) K(↑) L(→) on right hand
- **Page navigation**: Page Up/Down, Home/End
- **Word/line navigation**: ⌥← / ⌥→ (word), ⌃← / ⌃→ (line)
- **macOS window management**: Mission Control, App Windows, Desktop switching

### Activation

Hold **Left Pad** (Backspace key) to activate.

---

## Layer 5: NUM (Numbers)

Numeric keypad layout with mathematical operators on the left hand.

![](keymap-layer-5.svg)

### Key Features

- **Numbers 1-8** arranged bottom-to-top, outside-to-inside
- **Number 0** positioned on the _left side_ of `1`
- **Number 9** positioned on the _right side_ of `8`
- **Arithmetic operators** (+, -, *, /) on North keys
- **Parentheses** and **equals sign** for formula entry

### Activation

Hold **Right Pad** (Space key) to activate.

---

## Layer 6: SYM (Symbols)

Programming symbols and brackets on the right hand, organized for efficient chording.

![](keymap-layer-6.svg)

### Key Features

- **Paired brackets**: {}, [], () positioned for easy chording
- **Comparison operators**: <, >, =
- **Logic symbols**: &, |, !
- **Special characters**: @, #, $, %, ^, *, /, \, _, :

### Activation

Hold **Left Nail** (Tab key) to activate.

---

## Layer 7: FUN (Function Keys)

All function keys `F1`-`F20` on the left hand.

![](keymap-layer-7.svg)

### Key Features

- **F1-F9** follow the same convention as the numbers layer
- **F10** positioned where the number `0` is on the numbers layer
- **F11-F14** on North keys following the number increase pattern
- Arranged bottom-to-top, outside-to-inside for systematic access

### Activation

Hold **Right Nail** (Back-tab key) to activate.

---

## Layer 8: MED (Multimedia Keys)

Media playback and volume control on the right hand.

![](keymap-layer-8.svg)

### Key Features

- **Volume control**: Volume Up/Down, Mute/Unmute
- **Playback control**: Play/Pause, Next/Previous track
- **Seek control**: Fast Forward, Rewind
- **Media keys** positioned on strong fingers for easy one-handed access

### Activation

Hold **Left Knuckle** to activate.

---

## Layer 14: SYS (System Keys)

System-level controls for display brightness, trackball DPI adjustment, and
power management on the left hand.

![](keymap-layer-14.svg)

### Key Features

- **Display brightness**: Brightness Up/Down on `L1`
- **Trackball DPI control**: Independent DPI adjustment for both trackballs
  - Left trackball DPI: Up/Down on `L3`
  - Right trackball DPI: Up/Down on `L2`
- **Power control**: Power button on `L4` Center
- **Firmware mode**: Svalboard Output key for firmware state logging

### Activation

Hold **Right Knuckle** to activate.

---

## Layer 15: MBO (Mouse Buttons)

Auto-activated mouse layer with special mouse button keys and modifiers.

![](keymap-layer-15.svg)

### Key Features

- **Special mouse button keys** on left Double-South positions (`MB_SFT`,
  `MB_ALT`, `MB_GUI`, `MB_CTL`)
- **Regular modifiers** on right Double-South for Cmd+Click, etc.
- **Sniper mode** on thumb Double-Down keys for precision pointing
- **Return to BASE** when pressing any key that is not a mouse button

### Activation

Automatically activated when you move either trackball.

### Special Mouse Button Keys (Left Double-South)

These keys have intelligent dual behavior:

- **MB_SFT**: Shift or mouse left click
- **MB_GUI**: Cmd or mouse middle click
- **MB_ALT**: Option or mouse right click
- **MB_CTL**: Control or mouse button 4 click

See [Advanced Features](advanced-features.md#special-mouse-button-keys) for
detailed behavior.

---

## Layout Philosophy

### Miryoku-Inspired Activation

Layers use a single-hand activation pattern: hold a layer key on one hand, and
all functional keys appear on the opposite hand. This eliminates same-hand
stretching and complex finger gymnastics.

Example: Hold left thumb Backspace → `NAV` layer keys appear on right hand only.

The modifiers keys are still available on the same key that activated the layer
so you can combine them with the active layer's keys.

### Key Organization Principles

- **Numbers/Functions**: Bottom-to-top (`1`→`9`, `F1`→`F9`), outside-to-inside
- **Symbols**: Paired logically for chording (`{}` and `[]` adjacent)
- **Navigation**: Vim-style HJKL with contextual modifiers
- **Modifiers**: Symmetrically placed on both hands for flexibility
- **Frequent keys**: Placed on strong fingers (index, middle)

---

## References

- [Miryoku Layout Philosophy](https://github.com/manna-harbour/miryoku)
- [COLEMAK-DH Layout](https://colemakmods.github.io/mod-dh/)
- [Svalboard Official Site](https://svalboard.com)
