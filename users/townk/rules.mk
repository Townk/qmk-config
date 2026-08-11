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

SRC += townk_layers.c
SRC += townk_mods.c
SRC += townk_mouse.c
SRC += townk_overrides.c
SRC += townk_smtd.c

CFLAGS += -fcommon

