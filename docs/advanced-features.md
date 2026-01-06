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
- [Runtime Customization](#runtime-customization)

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
  - Without Shift: Delete on tap
  - With Shift: Backspace on tap
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
Buttons) layer**:

- L1 DS: `MB_SFT` (Shift + Mouse Button)
- L2 DS: `MB_GUI` (Command + Mouse Button)
- L3 DS: `MB_ALT` (Option + Mouse Button)
- L4 DS: `MB_CTL` (Control + Mouse Button)

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

| Layer | Color | HSV Constant |
| ----- | ----- | ------------ |
| BASE | Green | HSV_GREEN |
| NAV | Orange | HSV_ORANGE |
| NUM | Azure | HSV_AZURE |
| SYM | Coral | HSV_CORAL |
| FUN | Purple | HSV_PURPLE |
| MED | Yellow | HSV_YELLOW |
| SYS | Red | HSV_RED |
| MBO | Magenta | HSV_MAGENTA |

### Customizing Colors

Edit the layer color definitions in `users/townk/townk_layers.c`:

```c
#define LAYER_COLOR(name, color) rgblight_segment_t const (name)[] = RGBLIGHT_LAYER_SEGMENTS({0, 2, color})
LAYER_COLOR(layer0_colors, HSV_GREEN);  // Base layer
LAYER_COLOR(layer1_colors, HSV_ORANGE); // Navigation
LAYER_COLOR(layer2_colors, HSV_AZURE);  // Numbers
LAYER_COLOR(layer3_colors, HSV_CORAL);  // Symbols
LAYER_COLOR(layer4_colors, HSV_PURPLE); // Fn Keys
LAYER_COLOR(layer5_colors, HSV_YELLOW); // Multimedia
LAYER_COLOR(layer14_colors, HSV_RED);     // System
LAYER_COLOR(layer15_colors, HSV_MAGENTA); // Mouse layer
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
