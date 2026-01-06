# Svalboard QMK Configuration - COLEMAK-DH Layout

[![Build Vial-QMK firmware](https://github.com/Townk/qmk-config/actions/workflows/build_binaries.yaml/badge.svg)](https://github.com/Townk/qmk-config/actions/workflows/build_binaries.yaml)

A highly optimized [QMK](https://qmk.fm) firmware configuration for the
[Svalboard](https://svalboard.com) split ergonomic keyboard featuring dual
trackballs, COLEMAK-DH layout, and Miryoku-inspired layer design.

## Features

- **COLEMAK-DH Layout**: Modern, ergonomic letter arrangement optimized for
  finger rolls
- **Miryoku-Inspired Layers**: Single-hand layer activation with all functional
  keys on the opposite hand
- **Dual Trackball Support**: Independent sensors with configurable DPI
  (200-2400)
- **Home Row Modifiers**: Standard QMK modifiers on Double-South keys for
  instant, reliable modifier access without tap-dance interference
- **Special Mouse Keys**: Dual-function modifier/mouse button keys with
  intelligent context switching
- **RGB Layer Indicators**: Visual feedback for active layers
- **Vial/Keybard Support**: Runtime customization without reflashing
- **8 Thoughtfully Designed Layers**: `BASE`, `NAV`, `NUM`, `SYM`, `FUN`,
  `MED`, `SYS`, and auto-activated mouse layer

## Documentation

- **[Layer Reference](docs/layer-reference.md)** - Complete visual guide to all
  8 keyboard layers
- **[Advanced Features](docs/advanced-features.md)** - Home row modifiers,
  special mouse keys, trackball configuration, and more
- **[Changelog](CHANGELOG.md)** - Version history and release notes

## Layer Overview

| Layer | Name | Activation | Purpose |
| ----- | ---- | ---------- | ------- |
| 0 | BASE | Default | COLEMAK-DH letters with common symbols |
| 1 | NAV | Hold left thumb inward (Backspace) | Vim-style navigation and window management |
| 2 | NUM | Hold right thumb inward (Space) | Numeric keypad with operators |
| 3 | SYM | Hold left thumb outward (Tab) | Programming symbols and brackets |
| 4 | FUN | Hold right thumb outward (Back-tab) | Function keys F1-F20 |
| 5 | MED | Hold left thumb knuckle outward | Multimedia keys |
| 6 | SYS | Hold right thumb knuckle outward | System keys (keyboard firmware and OS) |
| 7 | MBO | Auto on trackball movement | Mouse buttons with modifiers |

For detailed layer diagrams with all key positions, see the [Layer Reference](docs/layer-reference.md).

## Quick Start

### Prerequisites

- Python 3.9 or later
- [QMK CLI](https://docs.qmk.fm/#/newbs_getting_started)
- Git with submodules support

### Initial Setup

1. **Clone this repository**:

   ```bash
   git clone --recurse-submodules https://github.com/Townk/qmk-config.git
   cd qmk-config
   ```

2. **Set up QMK** (first time only):

   ```bash
   qmk setup
   ```

3. **Configure QMK userspace** to point to this directory:

   ```bash
   qmk config user.overlay_dir="$(realpath .)"
   ```

### Building Locally

Build firmware for both keyboard halves:

```bash
qmk userspace-compile
```

This generates versioned firmware files:

- `svalboard-colemak-dh-townk-{VERSION}.left.uf2` - Left half firmware
- `svalboard-colemak-dh-townk-{VERSION}.right.uf2` - Right half firmware

Or build a specific half:

```bash
qmk compile -kb svalboard/trackball/pmw3389/left -km townk
qmk compile -kb svalboard/trackball/pmw3389/right -km townk
```

### Flashing Firmware

1. Put the keyboard half into bootloader mode (consult your Svalboard manual)
2. Copy the corresponding `.uf2` file to the mounted drive
3. The keyboard will automatically reboot with the new firmware
4. Repeat for the other half

### Using GitHub Actions (Recommended)

This repository is configured to automatically build firmware on every push:

1. Fork this repository
2. Enable GitHub Actions in your fork's settings
3. Push your changes
4. Download built firmware from the Actions artifacts tab
5. Firmware is automatically published to the Releases page with semantic
   versioning (v1.0.1, v1.0.2, etc.)

## Customization

### Runtime Customization (No Reflashing Required)

This firmware supports [Vial](https://vial.rocks/) and
[Keybard](https://captdeaf.github.io/keybard/) (recommended) for runtime
configuration. Just access one of these web applications using a Chrome-based
browser to connect your keyboard and modify its layers and keys.

### Code Customization

Key files to modify:

- **`keyboards/svalboard/keymaps/townk/keymap.c`**: Layer definitions and key
  mappings
- **`users/townk/townk_mouse.c`**: Mouse button behavior
- **`users/townk/townk_layers.c`**: RGB layer colors
- **`users/townk/townk_keycodes.h`**: Custom key code definitions
- **`keyboards/svalboard/keymaps/townk/config.h`**: Hardware settings (DPI,
  timeouts, etc.)

After making changes, rebuild and flash:

```bash
qmk userspace-compile
```

### Trackball Configuration

Default settings:

- **Left Trackball**: Scroll mode enabled, 400 DPI
- **Right Trackball**: Cursor mode, 1200 DPI
- **Auto Mouse Layer**: Enabled
- **DPI Options**: 200, 400, 800, 1200, 1600, 2400

For detailed configuration instructions and Sniper mode documentation, see
[Advanced Features - Trackball
Configuration](docs/advanced-features.md#trackball-configuration).

## Project Structure

```txt
.
├── docs/                               # Documentation
│   ├── layer-reference.md              # Complete layer visual reference
│   └── advanced-features.md            # Special features and configuration
│
├── keyboards/svalboard/keymaps/townk/  # Keyboard-specific keymap
│   ├── keymap.c                        # Main layer definitions
│   ├── config.h                        # Hardware configuration
│   ├── vial.json                       # Vial definitions
│   └── rules.mk                        # Build flags
│
├── users/townk/                        # Shared user code
│   ├── townk_keycodes.h                # Custom keycodes
│   ├── townk_layers.h/c                # RGB layer indicators
│   ├── townk_mouse.h/c                 # Special mouse keys
│   ├── townk_overrides.h/c             # Key overrides
│   └── townk_smtd.c                    # SM_TD integration
│
├── modules/stasmarkin/sm_td/           # SM_TD library (submodule)
├── .github/workflows/                  # CI/CD configuration
├── .devcontainer/                      # Docker dev environment
├── VERSION                             # Semantic version (major.minor)
├── CHANGELOG.md                        # Version history
└── qmk.json                            # Build targets
```

## Special Features

### Home Row Modifiers

Standard QMK modifiers placed on Double-South keys (accessible via downward
flick). Provides instant modifier activation without tap-dance timing delays
that can interfere with fast typing.

[Learn more →](docs/advanced-features.md#home-row-modifiers)

### Dual-Function Layer Keys

Thumb cluster keys use the [SM_TD](https://github.com/stasmarkin/sm_td)
(Stasmarkin Tap Dance) library for intelligent tap/hold behavior: tap for a key
(Backspace, Tab, Space, etc.), hold to activate a layer (`NAV`, `SYM`, `NUM`,
`FUN`). SM_TD adapts to your typing speed to prevent false triggers.

[Learn more →](docs/advanced-features.md#dual-function-layer-keys)

### Special Mouse Button Keys

The `MB_SFT`, `MB_ALT`, `MB_GUI`, and `MB_CTL` keys provide intelligent dual
behavior for mouse operations with modifiers. They act as mouse buttons,
keyboard modifiers, or held mouse buttons depending on context.

[Learn more →](docs/advanced-features.md#special-mouse-button-keys)

### Caps Word

Double-tap the Double-Down thumb key to enable Caps Word - type in CAPS without
holding shift. Automatically deactivates on space, punctuation, or layer
change.

[Learn more →](docs/advanced-features.md#caps-word)

### RGB Layer Indicators

RGB lighting automatically changes color based on the active layer, providing
instant visual feedback.

[Learn more →](docs/advanced-features.md#rgb-layer-indicators)

## Development

### Using the Dev Container

This project includes a `.devcontainer` configuration for development:

```bash
# With VS Code
1. Install the "Remote - Containers" extension
2. Open this folder in VS Code
3. Click "Reopen in Container" when prompted

# With Docker directly
docker run -v $(pwd):/workspace ghcr.io/qmk/qmk_cli
```

### Code Style

- C code follows Google style (see `.clang-format`)
- Format code before committing:

  ```bash
  clang-format -i **/*.c **/*.h
  ```

### Testing Changes

1. Build locally to check for compilation errors
2. Flash to keyboard and test functionality
3. Check RGB indicators for each layer
4. Verify trackball behavior and DPI settings
5. Test special mouse keys in various scenarios

## Technical Details

- **QMK Userspace Version**: 1.0
- **Base Firmware**: [vial-qmk](https://github.com/svalboard/vial-qmk)
  (Svalboard fork)
- **Keyboard**: Svalboard with PMW3389 dual-trackballs
- **Layout**: COLEMAK-DH (46-key split)
- **Features**: Vial, RGB Lighting, Pointing Device, Mouse Keys

## Resources

- [Svalboard Official Site](https://svalboard.com)
- [QMK Documentation](https://docs.qmk.fm)
- [Vial Documentation](https://get.vial.today)
- [COLEMAK-DH Layout](https://colemakmods.github.io/mod-dh/)
- [Miryoku Layout Philosophy](https://github.com/manna-harbour/miryoku)
- [SM_TD Library](https://github.com/stasmarkin/sm_td)

## Contributing

Contributions are welcome! This is a personal keyboard configuration, but if
you have improvements or find bugs:

1. Fork this repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on hardware
5. Submit a pull request with a clear description

## License

This project is licensed under the GNU General Public License v3.0 or later.
See individual file headers for copyright information.

Copyright (C) 2025 [Thiago Alves](https://github.com/townk)

## Acknowledgments

- [QMK Firmware](https://qmk.fm) - The foundation of this configuration
- [Svalboard](https://svalboard.com) - For creating an incredible ergonomic keyboard
- [stasmarkin](https://github.com/stasmarkin) - For the SM_TD tap dance library
- [Miryoku](https://github.com/manna-harbour/miryoku) - For layer design inspiration
