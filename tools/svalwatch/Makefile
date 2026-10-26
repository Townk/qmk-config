.PHONY: build test clean

build:
	swift build -c release

# The Command Line Tools' swift-testing macros are only found on a clean
# build; pointing the compiler at them keeps incremental test runs working.
TESTING_PLUGINS := $(firstword $(wildcard /Library/Developer/CommandLineTools/usr/lib/swift/host/plugins/testing))
test:
	swift test $(if $(TESTING_PLUGINS),-Xswiftc -plugin-path -Xswiftc $(TESTING_PLUGINS))

clean:
	swift package clean
