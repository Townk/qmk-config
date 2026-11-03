# Copyright (C) 2025 Thiago Alves (https://github.com/townk)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

VPATH += keyboards/svalboard/keymaps
SRC += keymap_support.c

# sm_td is integrated manually (its documented non-module installation)
# rather than as a community module, so that process_record_user() can guard
# it against Repeat Key's replayed records — the module hook runs before
# anything the userspace controls and offers no such seam. The module
# manifest only auto-enabled deferred_exec, replicated here.
VPATH += $(QMK_USERSPACE)/modules/stasmarkin/sm_td
SRC += sm_td.c
DEFERRED_EXEC_ENABLE = yes

SRC += townk_announce.c
SRC += townk_hostos.c
SRC += townk_idle.c
SRC += townk_layers.c
SRC += townk_mods.c
SRC += townk_mouse.c
SRC += townk_overrides.c
SRC += townk_smtd.c
SRC += townk_streak.c
SRC += townk_switcher.c

CFLAGS += -fcommon

# The key-definitions fingerprint (tools/keydefs.py): a hash over the ordered
# custom keycode, layer and sval.json customKeycodes names, which changes
# exactly when one of their values can. townk_announce.c answers it to the
# overlay, which then knows whether its spelling tables fit this firmware.
# The qmk CLI runs make in qmk_home with QMK_USERSPACE set to this repo's
# absolute path, locally and in CI alike. A changed fingerprint changes the
# flags, so cflags.txt changes and everything rebuilds with the new value.
TOWNK_KEYDEFS_FP := $(shell python3 -I $(QMK_USERSPACE)/tools/keydefs.py --repo $(QMK_USERSPACE) fingerprint)
ifeq ($(TOWNK_KEYDEFS_FP),)
    $(error users/townk/rules.mk: cannot compute the key-definitions fingerprint; run `python3 -I tools/keydefs.py fingerprint` for the reason)
endif
OPT_DEFS += -DTOWNK_KEYDEFS_FP=$(TOWNK_KEYDEFS_FP)

# Route every string-descriptor read the USB stack reports to QMK's OS
# detection through townk_hostos.c first (__wrap_process_wlength), which
# tags it with the configuration state it arrived in. No fork change: the
# linker rewrites the one call site in tmk_core's usb_descriptor.c.
LDFLAGS += -Wl,--wrap=process_wlength

# Same trick for townk_announce.c's subscribe command: the svalboard/core
# module's dispatcher (sval.c) calls client_wrapper_receive() for every
# wrapped host packet, and the wrap lets the subscribe command be answered
# before the module sees it. See townk_announce.h for why it is this
# function and not one of VIA's handlers.
LDFLAGS += -Wl,--wrap=client_wrapper_receive
