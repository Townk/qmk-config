# Advanced Features

This document covers the special features and advanced capabilities of my
personal Svalboard COLEMAK firmware configuration.

## Table of Contents

- [Home Row Modifiers](#home-row-modifiers)
- [Dual-Function Layer Keys](#dual-function-layer-keys)
- [Special Mouse Button Keys](#special-mouse-button-keys)
- [Trackball Configuration](#trackball-configuration)
- [Caps Word](#caps-word)
- [RGB Layer Indicators](#rgb-layer-indicators)
- [Repeat Key](#repeat-key)
- [Additional Resources](#additional-resources)

---

## Home Row Modifiers

This configuration uses **standard QMK modifiers** on the Double-South keys for
reliable modifier access without tap-dance complexity.

### Placement

All four primary modifiers are placed on the **Double-South keys** (accessible
via downward flick) of each finger cluster:

**Left Hand:**

- L4 DS: Control (⌃)
- L3 DS: Option (⌥)
- L2 DS: Command (⌘)
- L1 DS: Shift (⇧)

**Right Hand:**

- R1 DS: Shift (⇧)
- R2 DS: Command (⌘)
- R3 DS: Option (⌥)
- R4 DS: Control (⌃)

### How It Works

These are **regular modifier keys** that activate immediately when pressed:

- Press and hold the Double-South key to activate the modifier
- Press another key while holding to apply the modifier
- Release to deactivate

### Why No Tap-Dance?

Traditional home row modifiers with tap-dance behavior can interfere with fast
typing, causing missed keys or accidental modifier activations. By placing
modifiers on dedicated Double-South keys (separate from typing keys), this
layout achieves:

1. **Fast typing without interference** - no timing delays or accidental
   activations
2. **Instant modifier response** - modifiers activate immediately when pressed
3. **Ergonomic access** - downward flick motion is comfortable and distinct from
   regular typing

The Double-South position provides a dedicated modifier key that doesn't
conflict with letter keys, eliminating the need for tap-dance timing logic.

---

## Dual-Function Layer Keys

The **thumb cluster keys** use the **SM_TD (Stasmarkin Tap Dance)** library to
provide intelligent dual-function behavior: tap for a key, hold for a layer.

### SM_TD Layer Keys

These thumb keys use SM_TD for reliable tap/hold distinction:

**Left Thumb:**

- **Pad**: Backspace/Delete (tap) / Navigation layer (hold)
  - Without Shift: Backspace on tap
  - With Shift: Delete on tap (shift is suppressed for the Delete output)
- **Nail**: Tab (tap) / Symbol layer (hold)
- **Down**: Smart Shift (tap) / Shift modifier (hold)

**Right Thumb:**

- **Pad**: Space (tap) / Number layer (hold)
- **Nail**: Back-tab (tap) / Function layer (hold)

### How SM_TD Works

The SM_TD library provides intelligent timing analysis:

- **Quick tap**: Sends the tap action (key press)
- **Hold briefly then press other key**: Activates the layer
- **Hold longer**: Registers the key as held
- **Adaptive timing**: Learns your typing patterns to avoid false triggers

### Why SM_TD for Layer Keys?

Layer activation requires tap/hold distinction, which needs intelligent timing.
SM_TD solves this by:

1. **Analyzing typing patterns** to distinguish intentional holds from fast
   typing
2. **Adjusting timing dynamically** based on your typing speed
3. **Providing consistent layer activation** without missed taps or accidental
   holds

> [!NOTE]
> While SM_TD has a reputation in the QMK community for reimplementing existing
> features, the library's unique adaptive timing algorithm requires functionality
> not available in standard QMK. After extensively testing all available QMK
> tap-hold solutions, SM_TD provides significantly superior results for fast
> typing (>50 wpm) with dual-function keys.
>
> The performance difference is substantial enough to justify the additional
> complexity for this configuration's thumb cluster layer activation.

### Smart Shift

The left thumb Down key (`CKC_SMSFT`) has special behavior:

- **Single tap**: One-shot Shift (next key only is capitalized)
- **Double-tap**: Activates Caps Word
- **Tap while Shift held**: Activates Caps Word
- **Hold**: Standard Shift modifier

This reduces the need to hold Shift for single capital letters.

For detailed customization options, see the [SM_TD
documentation](../modules/stasmarkin/sm_td/README.md).

---

## Special Mouse Button Keys

The `MB_SFT`, `MB_ALT`, `MB_GUI`, and `MB_CTL` keys provide intelligent
dual-function behavior for mouse operations with modifiers.

### Location

These keys are located on the **left Double-South keys** in the **MBO (Mouse
Buttons) layer**, with each key paired to a specific mouse button and
modifier:

| Key | Position | Mouse Button | Modifier (on hold) |
| --- | -------- | ------------ | ------------------ |
| `MB_SFT` | L1 DS | Button 1 (left click) | Shift (⇧) |
| `MB_GUI` | L2 DS | Button 3 (middle click) | Command (⌘) |
| `MB_ALT` | L3 DS | Button 2 (right click) | Option (⌥) |
| `MB_CTL` | L4 DS | Button 4 | Control (⌃) |

### Behavior Modes

These special keys have **three different behaviors** depending on context:

#### 1. Tap Alone → Mouse Click

When you tap the key without any other interaction, it sends the mouse
button paired with that key (left/right/middle click for `MB_SFT`/`MB_ALT`/
`MB_GUI`, and Button 4 for `MB_CTL` — see the table above).

**Use case**: Quick clicking with the most ergonomic key for the click type
you need, without holding any modifier.

#### 2. Hold + Press Other Key → Keyboard Modifier

When you hold the special key and press another keyboard key, it acts as a
**keyboard modifier**.

**Examples:**

- Hold `MB_GUI` + press `C` → ⌘C (copy)
- Hold `MB_SFT` + press arrow keys → Select text

**Use case**: Standard keyboard shortcuts while on the mouse layer.

#### 3. Hold + Move Trackball → Held Mouse Button

When you hold the special key and move the trackball, it converts to a **held
mouse button** for dragging.

**Use case**: Drag and drop operations with modifier context (e.g., Option-drag
to duplicate in macOS).

#### 4. With External Modifiers → Mouse Button

When you hold an external modifier (from the right Double-South keys or another
device) and tap the special key, it acts as a **mouse button click**.

**Examples:**

- Hold right ⌘ (R2 DS) + tap `MB_SFT` → ⌘+Click
- Hold right ⌃ (R4 DS) + tap `MB_SFT` → ⌃+Click

**Use case**: Modifier+Click operations like ⌘+Click to open links in new tabs.

### Implementation Details

These keys are implemented in `users/townk/townk_mouse.c` using custom key code
processing. The firmware tracks:

- Whether the key was tapped or held
- Whether the trackball moved during the hold
- Whether other modifiers are currently active
- Whether other keys were pressed during the hold

This state management enables the intelligent switching between the three
behavior modes.

### Example Workflows

**Drag and Drop:**

1. Move trackball to position cursor over item
2. Hold `MB_SFT` (auto-activates mouse layer if not already active)
3. Move trackball to drag item
4. Release `MB_SFT` to drop

**Modifier+Click for Link:**

1. Move trackball to position cursor over link
2. Hold right ⌘ (R2 DS)
3. Tap `MB_SFT` to ⌘+Click
4. Link opens in new tab

**Quick Click:**

1. Move trackball to position cursor
2. Tap `MB_SFT` for single click

---

## Trackball Configuration

The Svalboard features **dual trackballs** with independent configuration for
each side.

### Default Settings

**Left Trackball:**

- **Mode**: Scroll mode enabled
- **DPI**: 400
- **Purpose**: Scrolling through documents and web pages

**Right Trackball:**

- **Mode**: Cursor mode (scroll disabled)
- **DPI**: 1200
- **Purpose**: Precise cursor control

**Auto Mouse Layer**: Enabled (automatically activates MBO layer on trackball movement)

### Available DPI Settings

Both trackballs support the following DPI options:

- 200 DPI (ultra-precise)
- 400 DPI (precise scrolling)
- 800 DPI (balanced)
- 1200 DPI (fast cursor movement) **← Default for right**
- 1600 DPI (very fast)
- 2400 DPI (maximum speed)

### Changing Settings

#### Via Code (Permanent)

Edit `keyboard_post_init_user()` in `keyboards/svalboard/keymaps/townk/keymap.c`:

```c
void keyboard_post_init_user(void) {
    // Left trackball scroll mode (true = scroll, false = cursor)
    global_saved_values.left_scroll = true;

    // Right trackball scroll mode
    global_saved_values.right_scroll = false;

    // Auto mouse layer activation
    global_saved_values.auto_mouse = true;

    // DPI index (0=200, 1=400, 2=800, 3=1200, 4=1600, 5=2400)
    global_saved_values.left_dpi_index = 1;   // 400 DPI
    global_saved_values.right_dpi_index = 3;  // 1200 DPI

    // ... rest of initialization
}
```

After editing, rebuild and re-flash the firmware.

#### Via SYS Layer Keys (Runtime)

You can adjust trackball DPI settings on-the-fly using the **SYS layer** keys:

**Activating SYS Layer:**

Hold the **Right Knuckle** thumb key to activate the `SYS` layer.

**DPI Adjustment Keys:**

- **Left Trackball DPI**:
  - **L3 North** (upward flick): Increase left trackball DPI
  - **L3 South** (downward flick): Decrease left trackball DPI

- **Right Trackball DPI**:
  - **L2 North** (upward flick): Increase right trackball DPI
  - **L2 South** (downward flick): Decrease right trackball DPI

**How to Use:**

1. Hold Right Knuckle to activate `SYS` layer
2. Flick the appropriate key up or down to adjust DPI
3. Release Right Knuckle to return to normal operation
4. DPI changes are saved to EEPROM automatically

**Available DPI levels cycle through**: 200 → 400 → 800 → 1200 → 1600 → 2400 → (back to 200)

### Sniper Mode

Both thumb **Double-Down (DD) keys** on the `MBO` layer activate **Sniper Mode**:

- Left DD: Sniper mode with 3x sensitivity reduction
- Right DD: Sniper mode with 5x sensitivity reduction

Use Sniper Mode for:

- Pixel-perfect cursor positioning
- Detailed image editing
- Precise CAD work
- Small button clicking

---

## Caps Word

**Caps Word** allows you to type in CAPITAL LETTERS without holding Shift,
automatically deactivating when you're done with a word.

### Activation

There are two ways to activate Caps Word:

- **Press** either **Double-Down (DD) thumb key** — both are mapped to
  `QK_CAPS_WORD_TOGGLE`, which toggles Caps Word on/off in a single press.
- **Double-tap** the left thumb **Down** key (Smart Shift / `CKC_SMSFT`).
  Tapping it once gives a one-shot Shift; tapping it twice (or tapping it
  while Shift is already held) calls `caps_word_on()` instead.

### Behavior

When Caps Word is active:

- All letters are automatically capitalized
- Numbers and symbols work normally
- **Auto-deactivation** occurs when you press:
  - Space
  - Enter
  - Escape
  - Any punctuation
  - Any layer change

### Use Cases

- Typing constants: `MAX_BUFFER_SIZE`
- Environment variables: `PATH`, `HOME`
- Acronyms: `HTTP`, `API`, `JSON`
- Section headings: `INTRODUCTION`

### Implementation

Caps Word is provided by QMK's built-in `QK_CAPS_WORD_TOGGLE` feature, mapped
to both DD thumb keys for convenience.

---

## RGB Layer Indicators

The keyboard's RGB lighting changes color automatically based on the active
layer, providing instant visual feedback.

### Layer Colors

| Layer | Name | Color | HSV Constant | Hex |
| ----- | ---- | :---: | ------------ | --- |
| 0 | BASE | ![Green](https://img.shields.io/badge/Green-347156?style=flat) | `BASE_GREEN` | `#347156` |
| 1 | QWT | ![Teal](https://img.shields.io/badge/Teal-2F898B?style=flat) | `QWT_TEAL` | `#2F898B` |
| 2 | GAM1 | ![Blue](https://img.shields.io/badge/Blue-355A93?style=flat) | `GAME_1_BLUE` | `#355A93` |
| 3 | GAM2 | ![Deep blue](https://img.shields.io/badge/Deep%20blue-0C7595?style=flat) | `GAME_2_BLUE` | `#0C7595` |
| 4 | NAV | ![Orange](https://img.shields.io/badge/Orange-D96702?style=flat) | `NAV_ORANGE` | `#D96702` |
| 5 | NUM | ![Blue](https://img.shields.io/badge/Blue-41687E?style=flat) | `NUM_BLUE` | `#41687E` |
| 6 | SYM | ![Coral](https://img.shields.io/badge/Coral-FF7C4D?style=flat) | `HSV_CORAL` | `#FF7C4D` |
| 7 | FUN | ![Purple](https://img.shields.io/badge/Purple-605181?style=flat) | `FUN_PURPLE` | `#605181` |
| 8 | MED | ![Yellow](https://img.shields.io/badge/Yellow-767709?style=flat) | `MED_YELLOW` | `#767709` |
| 14 | SYS | ![Red](https://img.shields.io/badge/Red-9C2927?style=flat) | `SYS_RED` | `#9C2927` |
| 15 | MBO | ![Magenta](https://img.shields.io/badge/Magenta-8C3396?style=flat) | `MBO_MAGENTA` | `#8C3396` |

### Customizing Colors

Layer colors are defined as custom HSV triples at the top of
`users/townk/townk_layers.c` and assigned to RGB lighting segments via
`LAYER_COLOR`:

```c
// HSV triples for each layer color
#define BASE_GREEN   70, 220, 180
#define QWT_TEAL    105, 240, 150
#define GAME_1_BLUE 153, 255, 180
// ... etc.

#define LAYER_COLOR(name, color) rgblight_segment_t const (name)[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, color})
LAYER_COLOR(layer0_colors, BASE_GREEN);   // Base layer
LAYER_COLOR(layer1_colors, QWT_TEAL);     // Base QWERTY
LAYER_COLOR(layer2_colors, GAME_1_BLUE);  // Game layer 1
// ... etc.
```

To change a color, either edit the existing HSV triple or substitute one of
QMK's built-in constants (`HSV_RED`, `HSV_ORANGE`, `HSV_YELLOW`, `HSV_GREEN`,
`HSV_CYAN`, `HSV_BLUE`, `HSV_PURPLE`, `HSV_MAGENTA`, `HSV_PINK`, `HSV_CORAL`,
`HSV_WHITE`, etc.).

> [!NOTE]
> The `rgb_layers` array uses **all 16 RGB lighting slots** (the QMK maximum)
> even though several layer indices in between are unused. Slots 9-13 are
> assigned placeholder colors (`HSV_PINK`, `HSV_CHARTREUSE`, `HSV_GOLD`,
> `HSV_TEAL`, `HSV_SPRINGGREEN`) so that `SYS` (14) and `MBO` (15) can keep
> their fixed slot positions.

### How It Works

`townk_layers.c` registers the `rgb_layers` array via
`setup_rgb_light_layer()` during keyboard initialization, so QMK's built-in
RGB lighting layer system handles the indicator on its own.

The user-level `layer_state_set_user()` callback iterates every RGB layer
slot and toggles each one to match the current layer state, so the highest
active layer's color always wins. The same callback also disables the
auto-mouse layer while `GAM1` or `GAM2` is active.

Layer colors are firmware-defined and reset to the values above on every
boot — they are not user-tunable at runtime.

---

## Repeat Key

The **Repeat Key** (`QK_REP`) repeats the last key you pressed, useful for
character repetition and pattern entry.

### Location

**Left Up** thumb key on the BASE layer.

### Use Cases

The Repeat key is designed to maintain typing flow when encountering double
letters in words:

**Primary Use - Double Letters in Words:**

- English words: _arrow_, _pressed_, _pattern_, _coffee_, _balloon_
- Maintains finger roll rhythm without breaking flow
- Type the letter once, tap Repeat, continue typing

**Example Workflow:**

Typing "coffee": `c` `o` `f` `f` → becomes → `c` `o` `f` `[Repeat]` `e` `e` → `[Repeat]`

**Secondary Use - Symbol Pairs:**

- Markdown bold: `*` then `[Repeat]` → `**`
- Comparison operators: `=` then `[Repeat]` → `==`
- Quick symbol duplication for common patterns

The Repeat key is optimized for **one or two repetitions** to maintain typing
rhythm. For longer repetitions, holding the key or using copy-paste is more
efficient.

### Behavior

The Repeat key works with:

- Letters, numbers, and symbols
- Arrow keys and navigation
- Function keys

It does **not** repeat:

- Modifiers (Shift, Cmd, etc.)
- Layer keys
- Special function keys

---

## Additional Resources

- [QMK Firmware Documentation](https://docs.qmk.fm)
- [SM_TD Library Documentation](../modules/stasmarkin/sm_td/README.md)
- [Vial Documentation](https://get.vial.today)
- [Keybard GitHub Repository](https://github.com/svalboard/keybard)
- [Svalboard Official Site](https://svalboard.com)

For implementation details, see the source files:

- `users/townk/townk_mouse.c` - Special mouse key implementation
- `users/townk/townk_layers.c` - RGB layer indicators
- `users/townk/townk_smtd.c` - SM_TD dual-function layer key configuration
- `keyboards/svalboard/keymaps/townk/keymap.c` - Main keymap and trackball settings
- `keyboards/svalboard/keymaps/townk/config.h` - Hardware and feature configuration
