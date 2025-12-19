#!/bin/bash
# Quick build script for Linux/macOS

echo "myMCpp Build Script"

# Check for Qt6
if ! command -v qmake6 &> /dev/null && ! command -v qmake &> /dev/null; then
    echo "ERROR: Qt6 not found in PATH"
    echo "Please install Qt6 development libraries:"
    echo "  Ubuntu/Debian: sudo apt install qt6-base-dev"
    echo "  Fedora: sudo dnf install qt6-qtbase-devel"
    echo "  macOS: brew install qt@6"
    exit 1
fi

# Detect Qt path
if command -v qmake6 &> /dev/null; then
    QT_DIR=$(qmake6 -query QT_INSTALL_PREFIX)
elif command -v qmake &> /dev/null; then
    QT_DIR=$(qmake -query QT_INSTALL_PREFIX)
fi

echo "Qt Directory: $QT_DIR"
echo ""

# Create build directory
mkdir -p build

# Configure
echo "Configuring CMake..."
cmake -B build -S . -DCMAKE_PREFIX_PATH="$QT_DIR" -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

echo ""
echo "Building..."
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Build failed!"
    exit 1
fi

echo ""
echo "========================================"
echo "Build completed successfully!"
echo "Executable location: build/bin/myMCpp"
echo "========================================"
echo ""
