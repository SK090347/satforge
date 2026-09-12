# Convenience wrapper around CMake
BUILD_DIR ?= build
CMAKE ?= cmake

.PHONY: all configure build test clean install python-test

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build: configure
	$(CMAKE) --build $(BUILD_DIR) -j

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure
	$(MAKE) python-test

python-test: build
	SATFORGE_BIN="$(CURDIR)/$(BUILD_DIR)/satforge" \
	  python3 -m pytest python/tests -q

clean:
	rm -rf $(BUILD_DIR)

install: build
	$(CMAKE) --install $(BUILD_DIR)
