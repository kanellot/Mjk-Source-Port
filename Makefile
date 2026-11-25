# Wrapper de CMake para Linux y macOS
.PHONY: all build test run clean

BUILD_DIR ?= build
BUILD_TYPE ?= RelWithDebInfo

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run: build
	./$(BUILD_DIR)/bin/mjk

clean:
	rm -rf $(BUILD_DIR)
