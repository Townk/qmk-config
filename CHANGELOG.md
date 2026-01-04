# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Comprehensive documentation structure
  - `docs/layer-reference.md` - Complete visual reference for all 6 keyboard layers
  - `docs/advanced-features.md` - Deep dive into special features and configuration
- Semantic versioning system with release-based patch tracking
- VERSION file for major.minor version management (patch auto-increments from
  release count)
- Versioned firmware filenames (e.g., `svalboard-colemak-dh-townk-1.0.1.left.uf2`)

### Changed

- GitHub Actions workflow now uses semantic versions for releases (v1.0.1,
  v1.0.2, etc.)
- Release tags changed from timestamp-based to version-based
- Firmware filenames now include version numbers for better tracking
- README streamlined to reference detailed documentation files

### Fixed

- Version tracking no longer requires git commits (uses GitHub API to count releases).

## [1.0.0] - 2025-01-03

Initial release of the Svalboard COLEMAK-DH firmware configuration.

### Added

- COLEMAK-DH base layer with ergonomic letter arrangement
- Miryoku-inspired layer system with 6 layers:
  - BASE: COLEMAK-DH letters and common symbols
  - NAV: Vim-style navigation and macOS window management
  - NUM: Numeric keypad with arithmetic operators
  - SYM: Programming symbols and brackets
  - FUN: Function keys F1-F20
  - MBO: Auto-activated mouse layer with special mouse keys
- Advanced home row modifiers using SM_TD tap dance library
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
- Home row modifiers: SM_TD implementation on Double-South keys

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
