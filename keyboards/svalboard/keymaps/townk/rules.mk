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

VIA_ENABLE = yes
VIAL_ENABLE = yes
VIAL_INSECURE ?= yes

all: generate_keymap_images

generate_keymap_images:
	@if [ -z "$$CI" ] && [ -z "$$GITHUB_ACTIONS" ]; then \
		echo ""; \
		echo "============================================================"; \
		echo "Generating keymap images..."; \
		echo "============================================================"; \
		python3 "$(KEYMAP_PATH)/../../../../support/keymap-images/keymap-generator.py" || true; \
		echo "============================================================"; \
		echo ""; \
	fi
