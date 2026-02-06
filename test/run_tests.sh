#!/bin/bash

# Test runner for stem extractor
# Usage: ./run_tests.sh [module_file]

TEST_DIR="/mnt/data/SynologyDrive/code/musicdisc/stems-native/test"
BUILD_DIR="/mnt/data/SynologyDrive/code/musicdisc/stems-native/build"

echo "Untracker Test Runner"
echo "=========================="

# Check if build exists
if [ ! -f "$BUILD_DIR/untracker" ]; then
    echo "Error: Build not found. Please run 'make' or 'meson build && ninja -C build' first."
    exit 1
fi

# Check if a module file was provided, otherwise use the default
if [ $# -eq 0 ]; then
    DEFAULT_MODULE="zalza-karate_muffins.xm"
    if [ -f "$TEST_DIR/$DEFAULT_MODULE" ]; then
        MODULE_FILE="$TEST_DIR/$DEFAULT_MODULE"
        echo "Using default module file: $DEFAULT_MODULE"
    else
        echo "Usage: $0 <module_file>"
        echo "Example: $0 zalza-karate_muffins.xm"
        echo ""
        echo "Default module file 'zalza-karate_muffins.xm' not found in test directory."
        exit 1
    fi
else
    MODULE_FILE="$1"
    # Check if the module file exists
    if [ ! -f "$MODULE_FILE" ]; then
        echo "Error: Module file '$MODULE_FILE' not found!"
        exit 1
    fi
    echo "Using module file: $MODULE_FILE"
fi

cd "$TEST_DIR"

echo ""
echo "Running tests..."
"$BUILD_DIR/test/test_main" "$MODULE_FILE"

echo ""
echo "Tests completed!"