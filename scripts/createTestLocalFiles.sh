#!/bin/bash

# Define the target directory
TARGET_DIR="./files/remote/local"

# Create the target directory if it doesn't exist
mkdir -p "$TARGET_DIR"

# Create files of specified sizes
# dd if=/dev/zero of="$TARGET_DIR/test5mb" bs=1M count=5
dd if=/dev/zero of="$TARGET_DIR/test10mb" bs=1M count=10
dd if=/dev/zero of="$TARGET_DIR/test20mb" bs=1M count=20
dd if=/dev/zero of="$TARGET_DIR/test50mb" bs=1M count=50
dd if=/dev/zero of="$TARGET_DIR/test75mb" bs=1M count=75
dd if=/dev/zero of="$TARGET_DIR/test100mb" bs=1M count=100

echo "Files created successfully in $TARGET_DIR"
