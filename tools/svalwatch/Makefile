.PHONY: build test clean overlay-assets install

PREFIX ?= $(HOME)/.local
ASSETS ?= $(PREFIX)/share/svaloverlay
# A Python that has skim installed, with key tags (skim's feat/key-tags);
# the overlay page is built with `$(SKIM_PYTHON) -m skim`.
SKIM_PYTHON ?= $(HOME)/.local/share/uv/tools/qmk-skim/bin/python
QMK_HOME ?= $(shell qmk config user.qmk_home | sed -E 's/^user\.qmk_home=//; s/ \(config\)$$//')
REPO := $(abspath ../..)
# The built firmware, which the overlay's spelling table is read from: `make`
# in $(REPO) leaves it in qmk_home.
ELF ?= $(QMK_HOME)/.build/svalboard_trackball_pmw3389_right_townk.elf

build:
	swift build -c release

# The Command Line Tools' swift-testing macros are only found on a clean
# build; pointing the compiler at them keeps incremental test runs working.
TESTING_PLUGINS := $(firstword $(wildcard /Library/Developer/CommandLineTools/usr/lib/swift/host/plugins/testing))
test:
	swift test $(if $(TESTING_PLUGINS),-Xswiftc -plugin-path -Xswiftc $(TESTING_PLUGINS))

# svaloverlay's default page: skim's layer images in the overlay's palette,
# plus the render bundle (render/) the overlay draws the keyboard's own keymap
# with. The overlay reads the keymap off the keyboard when it connects and
# renders its page itself, so nothing needs running after a flash; this target
# is for working on the overlay itself (its look, skim config or bundle) and
# for CI, which packs the result into the release archive.
overlay-assets:
	@test -f "$(ELF)" || { echo "overlay-assets: no firmware ELF at $(ELF); build the firmware first (\`make\` in $(REPO))" >&2; exit 1; }
	$(SKIM_PYTHON) overlay/build_assets.py --repo $(REPO) --qmk-home $(QMK_HOME) --elf $(ELF) --out $(ASSETS)

# A running svaloverlay keeps the code and page it started with, so stop
# it: Hammerspoon's svalboard module restarts it within seconds, on the new
# install, in the mode it was in.
install: build overlay-assets
	install -d $(PREFIX)/bin
	install .build/release/svalwatch .build/release/svaloverlay $(PREFIX)/bin/
	-pkill -x svaloverlay

clean:
	swift package clean
