# Advanced Features

This document covers the special features and advanced capabilities of the
Svalboard COLEMAK-DH firmware configuration.

## Table of Contents

- [Home Row Modifiers](#home-row-modifiers)
- [Special Mouse Button Keys](#special-mouse-button-keys)
- [Trackball Configuration](#trackball-configuration)
- [Caps Word](#caps-word)
- [RGB Layer Indicators](#rgb-layer-indicators)
- [Repeat Key](#repeat-key)
- [Runtime Customization](#runtime-customization)

---

## Home Row Modifiers

This configuration uses the **SM_TD (Stasmarkin Tap Dance)** library for
reliable home row modifiers that work correctly even during fast typing.

### Placement

All four primary modifiers are placed on the **Double-South keys** (accessible
via downward flick) of each finger cluster:

**Left Hand:**

- L4 D-S: Control (⌃)
- L3 D-S: Option (⌥)
- L2 D-S: Command (⌘)
- L1 D-S: Shift (⇧)

**Right Hand:**

- R1 D-S: Shift (⇧)
- R2 D-S: Command (⌘)
- R3 D-S: Option (⌥)
- R4 D-S: Control (⌃)

### How It Works

The SM_TD library provides intelligent tap-dance behavior:

- **Tap alone**: Acts as the modifier for the next key
- **Hold + press other key**: Acts as held modifier (traditional behavior)
- **Works during fast typing**: The library handles timing intelligently to
  avoid false triggers

### Why SM_TD?

Traditional home row modifiers can interfere with fast typing, causing
accidental modifier activations. SM_TD solves this by:

1. **Analyzing typing patterns** to distinguish intentional holds from fast typing
2. **Adjusting timing dynamically** based on your typing speed
3. **Providing consistent behavior** across different typing speeds

For detailed customization options, see the [SM_TD documentation](../modules/stasmarkin/sm_td/README.md).

---

## Special Mouse Button Keys

The `MB_SFT`, `MB_ALT`, `MB_GUI`, and `MB_CTL` keys provide intelligent
dual-function behavior for mouse operations with modifiers.

### Location

These keys are located on the **left Double-South keys** in the **MBO (Mouse
Buttons) layer**:

- L1 D-S: `MB_SFT` (Shift + Mouse Button)
- L2 D-S: `MB_GUI` (Command + Mouse Button)
- L3 D-S: `MB_ALT` (Option + Mouse Button)
- L4 D-S: `MB_CTL` (Control + Mouse Button)

### Behavior Modes

These special keys have **three different behaviors** depending on context:

#### 1. Tap Alone → Mouse Click

When you tap the key without any other interaction, it acts as a **mouse button
click** (Button 1).

**Use case**: Quick clicking without holding modifiers.

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

- Hold right ⌘ (R2 D-S) + tap `MB_SFT` → ⌘+Click
- Hold right ⌃ (R4 D-S) + tap `MB_SFT` → ⌃+Click

**Use case**: Modifier+Click operations like ⌘+Click to open links in new tabs.

### Implementation Details

These keys are implemented in `users/townk/townk_mouse.c` using custom keycode
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
2. Hold right ⌘ (R2 D-S)
3. Tap `MB_SFT` to ⌘+Click
4. Link opens in new tab

**Quick Click:**

1. Move trackball to position cursor
2. Tap `MB_SFT` for single click

---

## Trackball Configuration

The Svalboard features **dual PMW3389 trackballs** with independent
configuration for each side.

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

After editing, rebuild and reflash the firmware.

#### Via Vial/Keybard (Runtime)

Use the [Keybard](https://github.com/svalboard/keybard) configurator to adjust
trackball settings in real-time:

1. Connect your keyboard
2. Open Keybard
3. Navigate to trackball settings
4. Adjust DPI, scroll mode, and auto-mouse settings
5. Changes are saved to EEPROM automatically

### Sniper Mode

Both thumb **Knuckle keys** on the MBO layer activate **Sniper Mode**:

- Left Knuckle: Sniper mode with 3x sensitivity reduction
- Right Knuckle: Sniper mode with 5x sensitivity reduction

Use Sniper Mode for:

- Pixel-perfect cursor positioning
- Detailed image editing
- Precise CAD work
- Small button clicking

---

## Caps Word

**Caps Word** allows you to type in CAPITAL LETTERS without holding Shift,
automatically deactivating when you're done.

### Activation

**Double-tap** either **Double-Down (DD) thumb key** to toggle Caps Word on/off.

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

| Layer | Color | RGB Value |
| ----- | ----- | --------- |
| BASE | Purple | (170, 0, 255) |
| NAV | Blue | (0, 0, 255) |
| NUM | Green | (0, 255, 0) |
| SYM | Yellow | (255, 255, 0) |
| FUN | Orange | (255, 128, 0) |
| MBO | Red | (255, 0, 0) |

### Customizing Colors

Edit `townk_layer_colors[]` array in `users/townk/townk_layers.c`:

```c
const HSV townk_layer_colors[DYNAMIC_KEYMAP_LAYER_COUNT] = {
    [_BASE] = {HSV_PURPLE},     // Purple
    [_NAV]  = {HSV_BLUE},       // Blue
    [_NUM]  = {HSV_GREEN},      // Green
    [_SYM]  = {HSV_YELLOW},     // Yellow
    [_FUN]  = {HSV_ORANGE},     // Orange
    [_MBO]  = {HSV_RED},        // Red
};
```

Available QMK color constants:

- `HSV_RED`, `HSV_ORANGE`, `HSV_YELLOW`
- `HSV_GREEN`, `HSV_CYAN`, `HSV_BLUE`
- `HSV_PURPLE`, `HSV_MAGENTA`, `HSV_PINK`
- `HSV_WHITE`

Or define custom RGB values: `{hue, saturation, value}`

### How It Works

The `layer_state_set_user()` callback in `townk_layers.c` detects layer changes
and updates the RGB lighting via `rgblight_sethsv_noeep()`.

RGB changes are **not saved to EEPROM**, so the lighting resets to your default
on restart.

---

## Repeat Key

The **Repeat Key** (`QK_REP`) repeats the last key you pressed, useful for
character repetition and pattern entry.

### Location

**Left Up** thumb key on the BASE layer.

### Use Cases

- Repeating characters: Type `=` then tap Repeat 5 times → `======`
- Markdown: Type `*` then Repeat once → `**` (bold)
- Drawing ASCII boxes: Type `-` then Repeat many times
- Quick duplication of symbols

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

## Runtime Customization

This firmware supports **Vial** and **Keybard** for runtime customization
without reflashing.

### Using Keybard

[Keybard](https://github.com/svalboard/keybard) is a Vial-based configurator specifically for the Svalboard.

**Installation:**

1. Download from [Keybard releases](https://github.com/svalboard/keybard/releases)
2. Install on your system
3. Connect your Svalboard
4. Keybard automatically detects your keyboard

**What You Can Customize:**

- **Keymap**: Remap any key to any function
- **Layers**: Modify layer behavior and activation
- **Trackball settings**: DPI, scroll mode, auto-mouse
- **Lighting**: RGB colors and effects
- **Macros**: Record and assign custom macros

**Changes are saved immediately** to the keyboard's EEPROM, persisting across reboots.

### Vial JSON

The Vial layout definition is in `keyboards/svalboard/keymaps/townk/vial.json`.
This file defines:

- Physical key positions
- Available keycodes
- Layer structure
- Lighting zones

When you modify the layout in code, update `vial.json` to keep the runtime
configurator in sync.

### Dynamic Keymap Support

The firmware includes `DYNAMIC_KEYMAP_LAYER_COUNT` set to 6, allowing runtime
modification of all layers through Vial/Keybard.

Layer count is defined in `keyboards/svalboard/keymaps/townk/config.h`:

```c
#define DYNAMIC_KEYMAP_LAYER_COUNT 6
```

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
- `users/townk/townk_smtd.c` - SM_TD home row modifier configuration
- `keyboards/svalboard/keymaps/townk/keymap.c` - Main keymap and trackball settings
- `keyboards/svalboard/keymaps/townk/config.h` - Hardware and feature configuration
