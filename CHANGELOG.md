# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Host test suite for the `MB_*` dual-role mouse/modifier engine
  (`python3 tests/run_tests.py`). Compiles the real `townk_mouse.c` into a
  shared library via sm_td's own test shim and drives it directly, asserting
  on the mouse buttons and modifiers emitted — no firmware build, no flashing,
  no keyboard attached. Includes a regression test for the phantom middle
  click fixed below, verified to fail when the fix is removed. Because the
  shim `#include`s `sm_td.c` directly, the suite also runs against whatever
  sm_td version is checked out, making it a safety net for future upgrades

- `QWT` layer (1) — QWERTY letter arrangement with the same modifier,
  symbol, and thumb cluster layout as `BASE`, for situations that assume
  the default layout
- `GAM1` layer (2) — gaming-oriented base layer with WASD movement on the
  left hand and mouse buttons / scroll on the right thumb cluster
- `GAM2` layer (3) — sub-layer of `GAM1` providing numbers `1`-`9`, `0`,
  and `F1`-`F9` for hotbar / ability slots without disturbing WASD
- Per-layer custom HSV color constants (`BASE_GREEN`, `QWT_TEAL`,
  `GAME_1_BLUE`, `GAME_2_BLUE`, `NAV_ORANGE`, `NUM_BLUE`, `FUN_PURPLE`,
  `MED_YELLOW`, `SYS_RED`, `MBO_MAGENTA`) replacing the stock `HSV_*`
  presets for finer indicator-color control
- `docs/keymap-symbols.svg` legend documenting the glyphs used across the
  keymap diagrams, embedded in the layer reference
- Comprehensive documentation structure
  - `docs/layer-reference.md` - Complete visual reference for all keyboard layers
  - `docs/advanced-features.md` - Deep dive into special features and configuration
- Semantic versioning system with release-based patch tracking
- VERSION file for major.minor version management (patch auto-increments from
  release count)
- Versioned firmware filenames (e.g., `svalboard-colemak-dh-townk-1.0.1.left.uf2`)

### Changed

- `MB_*` keys are now **button-first**: a press commits to nothing and emits
  nothing, and the modifier role must be earned by a competing signal (another
  keypress, or a scroll) before release. Previously the modifier was claimed on
  press and the mouse button was a release-time fallback, reached only when
  nothing had contradicted it — which required proving a negative from observed
  keypresses. Nothing happening is no longer evidence of a tap; it *is* the
  default. Scrolling with a key held now resolves it to a modifier (keeping
  ⌘+scroll as zoom) rather than leaving it undecided, and pointer motion still
  wins over scroll when a report carries both. The rules are documented as a
  table at the top of `townk_mouse.c` and asserted one test per row

- Renumbered layers to make room for the new typing/gaming layers:
  `NAV` 1→4, `NUM` 2→5, `SYM` 3→6, `FUN` 4→7, `MED` 5→8 (`SYS` stays at
  14 and `MBO` at 15)
- Auto-mouse layer is now disabled whenever `GAM1` *or* `GAM2` is active
  (previously only the single `_GAME` layer)
- Layer reference, README layer overview, and advanced-features RGB table
  updated for the new layer set
- GitHub Actions workflow now uses semantic versions for releases (v1.0.1,
  v1.0.2, etc.)
- Release tags changed from timestamp-based to version-based
- Firmware filenames now include version numbers for better tracking
- README streamlined to reference detailed documentation files

### Removed

- Game-layer key overrides for `Shift + 0/1/2/3/4` → `9/5/6/7/8`. `GAM2`
  now exposes the digits directly, so the workaround is no longer needed.

### Fixed

- Modifiers pressed mid-drag had no effect — starting a drag and then holding
  Option to turn a Finder move into a copy never registered Option, so the copy
  cursor never appeared and the file was moved. A key pressed while a mouse
  button is already held now becomes a modifier immediately, since the next
  event is the drop rather than a keypress that could earn it. The mirror of
  the existing rule that a modifier held at press time makes the key a button
- Phantom mouse click after scrolling with an `MB_*` key held — ⌘+scroll to
  zoom, then release, emitted a stray modifier-less middle click. Same defect
  as the SM_TD case below (an undecided key falling through to its release-time
  tap), found by the new host test suite on its first run
- Phantom mouse click when an `MB_*` key was combined with an SM_TD key
  (e.g. Cmd+Space, Cmd+Tab, Cmd+Backspace). SM_TD hooks
  `pre_process_record_sm_td()`, which sits ahead of `process_record_user()`
  on the `&&`-chain in `pre_process_record_quantum()`, so the held `MB_*`
  key never learned it had been used as a modifier and fired its "tapped
  alone" mouse-button tap on release — a real, modifier-less click as far
  as the OS was concerned (it pasted in any app bound to middle-click
  paste, and opened context menus over the Dock). `on_smtd_action()` now
  commits pending `MB_*` keys to their modifier role as well
- `ML_CMD` now resolves to `KC_LCMD` instead of `KC_LGUI`, so generated
  keymap images label the Command key correctly
- Documentation: corrected the Backspace/Delete shift behavior on
  `CKC_BSPC` (plain tap = Backspace, Shift+tap = Delete — was reversed)
- Documentation: per-`MB_*` mouse button mapping now reflects the actual
  buttons sent by `townk_mouse.c` (BTN1 / BTN2 / BTN3 / BTN4 instead of
  the generic "Button 1")
- Documentation: Caps Word activation — Double-Down keys are
  `QK_CAPS_WORD_TOGGLE` (single press toggles); the double-tap path is on
  the Smart Shift key, not on DD
- Documentation: layout name standardized to `COLEMAK-DH` throughout the
  README and Technical Details
- Version tracking no longer requires git commits (uses GitHub API to count releases).

## [1.0.0] - 2025-01-03

Initial release of the Svalboard COLEMAK-DH firmware configuration.

### Added

- COLEMAK-DH base layer with ergonomic letter arrangement
- Miryoku-inspired layer system with 8 layers:
  - BASE: COLEMAK-DH letters and common symbols
  - NAV: Vim-style navigation and macOS window management
  - NUM: Numeric keypad with arithmetic operators
  - SYM: Programming symbols and brackets
  - FUN: Function keys F1-F20
  - MED: Multimedia keys
  - SYS: System keys (keyboard firmware and OS)
  - MBO: Auto-activated mouse layer with special mouse keys
- Home row modifiers using standard QMK modifiers on Double-South keys
- Dual-function layer keys using SM_TD tap dance library for intelligent
  tap/hold behavior
- Special mouse button keys (MB_SFT, MB_ALT, MB_GUI, MB_CTL) with intelligent
  dual behavior
- Dual trackball support (PMW3389) with independent DPI settings
- RGB layer indicators with custom colors per layer
- Caps Word support (double-tap to toggle)
- Repeat key for efficient character repetition
- Vial/Keybard support for runtime customization
- GitHub Actions CI/CD for automated firmware builds
- Dev container configuration for consistent development environment
- Comprehensive README with setup, building, and customization instructions

### Configuration Defaults

- Left trackball: Scroll mode enabled, 400 DPI
- Right trackball: Cursor mode, 1200 DPI
- Auto mouse layer: Enabled
- RGB layer indicators: Enabled
- Home row modifiers: Standard QMK implementation on Double-South keys
- Dual-function layer keys: SM_TD implementation on thumb cluster keys

### Development

- QMK userspace configuration
- C code formatted with clang-format (Google style)
- Submodule integration for SM_TD library
- Build targets for left and right keyboard halves
- Automated release creation on successful builds

---

## Version Format

This project uses semantic versioning: `MAJOR.MINOR.PATCH`

- **MAJOR**: Incompatible changes (manual bump via VERSION file)
- **MINOR**: New features, backward compatible (manual bump via VERSION file)
- **PATCH**: Bug fixes, small improvements (auto-incremented per build)

The PATCH number automatically increments based on the number of GitHub
releases for the current MAJOR.MINOR version. No version bump commits are
created.

To bump MAJOR or MINOR version, edit the `VERSION` file and commit the change.

---

## Unreleased Changes

Changes in the `main` branch that haven't been released yet appear in the
[Unreleased] section above.

[Unreleased]: https://github.com/Townk/qmk-config/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Townk/qmk-config/releases/tag/v1.0.0
