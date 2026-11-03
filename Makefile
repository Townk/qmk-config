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

# The overlay's layer pictures are drawn from this keymap and installed in
# ~/.local/share/svaloverlay, and nothing on the keyboard tells the overlay
# the keymap changed: without this step a flash puts the new keymap on the
# board while the overlay keeps showing the old one. So `make` redraws them
# too, then restarts a running overlay through Hammerspoon, because a running
# svaloverlay keeps the page it started with. pause+resume instead of a kill:
# a kill reads as a crash to the svalboard module, and a few of those in a
# row make it stop restarting the overlay at all. resume() leaves an overlay
# that is switched off, off.
#
# Skipped where it cannot or should not run: in CI (the overlay page is its
# own job there, packed into the release archive) and on a machine without
# skim. A failure here is a warning, not a failed build, because the firmware
# is already built and packaged by then, and that is what `make` is for.
SKIM_PYTHON ?= $(HOME)/.local/share/uv/tools/qmk-skim/bin/python

.PHONY: all clean overlay
all:
	qmk userspace-compile -e SVAL_FW_VERSION_STRING=$(FW_VERSION_STRING)
	$(PACKAGE) package --dir .
	$(MAKE) overlay

overlay:
	if [ -n "$$CI" ]; then \
	  echo "overlay: skipped in CI (the release builds its own overlay page)"; \
	elif [ ! -x "$(SKIM_PYTHON)" ]; then \
	  echo "overlay: skipped, no skim at $(SKIM_PYTHON) (set SKIM_PYTHON=...)"; \
	elif $(MAKE) -C tools/svalwatch overlay-assets SKIM_PYTHON="$(SKIM_PYTHON)"; then \
	  if command -v hs >/dev/null 2>&1; then \
	    hs -t 3 -c 'local s = require("svalboard"); s.pause(10); s.resume()' </dev/null >/dev/null 2>&1 \
	      && echo "overlay: redrawn and restarted" \
	      || echo "overlay: redrawn; Hammerspoon did not answer, so a running overlay shows the new keymap after its next restart"; \
	  else \
	    echo "overlay: redrawn"; \
	  fi; \
	else \
	  echo "warning: overlay: could not redraw the layer pictures; the firmware is built, but the overlay still shows the previous keymap" >&2; \
	fi

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
