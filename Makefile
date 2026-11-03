.SILENT:

MAKEFLAGS += --no-print-directory

# `make` is the build: it compiles through the qmk CLI and then packages
# the .svup updater files (a bare `qmk userspace-compile` skips that).
# Everything routes through the qmk CLI, which resolves this overlay's
# keymap (keymap.json, the Vial feature flags, the keymap's config.h)
# BEFORE invoking make inside qmk_home. Forwarding goals straight to
# qmk_home's Makefile -- what this file used to do -- silently drops all of
# that: the build loses VIAL_ENABLE/KEY_OVERRIDE_ENABLE and dies in
# townk_overrides.c under -Werror. (That old path also mis-parsed the
# current `qmk config` output, which appends " (config)" to values.)

# Overridden by CI (FW_VERSION_STRING=v1.2.3); at most 16 chars of A-Za-z0-9._+-
# Computed only when a build uses it, and an empty answer (no git, say) stops
# the build: an updater build with no version string cannot be told apart
# from any other, which is what qmk-flash's read-back relies on.
FW_VERSION_STRING ?= $(or $(shell python3 -I tools/updater/package_updates.py version),$(error cannot determine the firmware version string (package_updates.py version failed); pass FW_VERSION_STRING=... to set one))

PACKAGE := python3 -I tools/updater/package_updates.py

# The installed overlay's render bundle, and a Python with skim to rebuild it
# with: the same defaults as tools/svalwatch/Makefile.
OVERLAY_ASSETS ?= $(HOME)/.local/share/svaloverlay
SKIM_PYTHON ?= $(HOME)/.local/share/uv/tools/qmk-skim/bin/python

.PHONY: all clean overlay-keydefs
all:
	qmk userspace-compile -e SVAL_FW_VERSION_STRING=$(FW_VERSION_STRING)
	$(PACKAGE) package --dir .
	$(MAKE) overlay-keydefs

# The installed overlay draws the keyboard's keymap with spelling tables
# keyed on keycode values, which move when a custom keycode or layer enum
# (or sval.json's customKeycodes) is inserted or reordered. The overlay
# refuses to draw with tables built for other key definitions (it compares
# fingerprints, tools/keydefs.py), so after a build that renumbered them,
# rebuild the installed bundle and restart a running overlay; otherwise it
# would show "out of date" after the flash. Nothing to do when the
# fingerprints match, which is every build that did not touch the enums.
# Skipped in CI (nothing installed) and without skim; a failed rebuild only
# warns, since the firmware itself is fine.
overlay-keydefs:
ifdef CI
	echo "overlay: CI build, not checking the installed overlay bundle"
else
	if [ ! -x "$(SKIM_PYTHON)" ]; then \
	  echo "overlay: no skim at $(SKIM_PYTHON), not checking the installed overlay bundle"; \
	elif python3 -I tools/keydefs.py matches "$(OVERLAY_ASSETS)/render/keydefs.json"; then \
	  :; \
	else \
	  echo "overlay: rebuilding the installed overlay bundle for these key definitions"; \
	  if $(MAKE) -C tools/svalwatch overlay-assets ASSETS="$(OVERLAY_ASSETS)" SKIM_PYTHON="$(SKIM_PYTHON)"; then \
	    if command -v hs >/dev/null 2>&1; then \
	      hs -t 3 -c 'local s = require("svalboard"); s.pause(10); s.resume()' </dev/null >/dev/null \
	        && echo "overlay: restarted the running overlay" \
	        || echo "overlay: warning: could not restart the overlay through Hammerspoon" >&2; \
	    fi; \
	  else \
	    echo "overlay: warning: rebuilding the overlay bundle failed; the overlay keeps its old one" >&2; \
	  fi; \
	fi
endif

clean:
	qmk clean

# One half on its own, e.g.
#     make svalboard/trackball/pmw3389/left:townk
# built exactly as `make` builds it: a bare `qmk compile` does not read
# qmk.json, so it would drop the updater flags (and the version string) and
# leave a non-updater .uf2 where qmk-flash looks for firmware. The flags are
# read from qmk.json rather than repeated here, so it stays the ONE place they
# are set; a target it does not list is refused. Only this half's .svup is
# packaged, and it says so: qmk-flash installs both halves from one build.
# Anything with a second colon (":flash", ":uf2") needs the qmk CLI's own
# subcommands; refuse rather than quietly compile instead.
%:
	$(if $(word 3,$(subst :, ,$@)),$(error target '$@' has a suffix this Makefile does not forward; use the qmk CLI directly, e.g. `qmk flash -kb <keyboard> -km <keymap>`))
	flags=$$($(PACKAGE) build-env $(firstword $(subst :, ,$@)) $(lastword $(subst :, ,$@))) && \
	  qmk compile -kb $(firstword $(subst :, ,$@)) -km $(lastword $(subst :, ,$@)) \
	    $$flags -e SVAL_FW_VERSION_STRING=$(FW_VERSION_STRING)
	$(PACKAGE) package --dir . --half $(notdir $(firstword $(subst :, ,$@)))
