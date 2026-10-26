.PHONY: build test clean overlay-assets install

PREFIX ?= $(HOME)/.local
ASSETS ?= $(PREFIX)/share/svaloverlay
# A Python that has skim installed, with key tags (skim's feat/key-tags);
# the overlay page is built with `$(SKIM_PYTHON) -m skim`.
SKIM_PYTHON ?= $(HOME)/.local/share/uv/tools/qmk-skim/bin/python
QMK_HOME ?= $(shell qmk config user.qmk_home | sed -E 's/^user\.qmk_home=//; s/ \(config\)$$//')
REPO := $(abspath ../..)

build:
	swift build -c release

# The Command Line Tools' swift-testing macros are only found on a clean
# build; pointing the compiler at them keeps incremental test runs working.
TESTING_PLUGINS := $(firstword $(wildcard /Library/Developer/CommandLineTools/usr/lib/swift/host/plugins/testing))
test:
	swift test $(if $(TESTING_PLUGINS),-Xswiftc -plugin-path -Xswiftc $(TESTING_PLUGINS))

# svaloverlay's page: skim's layer images in the overlay's palette.
# Rebuild after changing the keymap or its skim config.
overlay-assets:
	$(SKIM_PYTHON) overlay/build_assets.py --repo $(REPO) --qmk-home $(QMK_HOME) --out $(ASSETS)

install: build overlay-assets
	install -d $(PREFIX)/bin
	install .build/release/svalwatch .build/release/svaloverlay $(PREFIX)/bin/

clean:
	swift package clean
