.SILENT:

MAKEFLAGS += --no-print-directory

# Everything routes through the qmk CLI, which resolves this overlay's
# keymap (keymap.json, the Vial feature flags, the keymap's config.h)
# BEFORE invoking make inside qmk_home. Forwarding goals straight to
# qmk_home's Makefile -- what this file used to do -- silently drops all of
# that: the build loses VIAL_ENABLE/KEY_OVERRIDE_ENABLE and dies in
# townk_overrides.c under -Werror. (That old path also mis-parsed the
# current `qmk config` output, which appends " (config)" to values.)

.PHONY: all clean
all:
	qmk userspace-compile

clean:
	qmk clean

# Forward keyboard:keymap goals unchanged, e.g.
#     make svalboard/trackball/pmw3389/left:townk
# Anything with a second colon (":flash", ":uf2") needs the qmk CLI's own
# subcommands; refuse rather than quietly compile instead.
%:
	$(if $(word 3,$(subst :, ,$@)),$(error target '$@' has a suffix this Makefile does not forward; use the qmk CLI directly, e.g. `qmk flash -kb <keyboard> -km <keymap>`))
	qmk compile -kb $(firstword $(subst :, ,$@)) -km $(lastword $(subst :, ,$@))
