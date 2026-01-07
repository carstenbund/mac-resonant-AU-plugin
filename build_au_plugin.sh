#!/bin/bash
#
# Build script for Modal Attractors Audio Unit plugin
#
# This script builds the AU plugin bundle that can be installed in:
# ~/Library/Audio/Plug-Ins/Components/
#
# Requirements:
# - macOS 12.0+
# - Xcode Command Line Tools
# - CMake (for building DSP core library)
#
# Usage:
#   ./build_au_plugin.sh [clean|install|test]
#

set -e  # Exit on error

# Configuration
PLUGIN_NAME="ModalAttractors"
BUNDLE_ID="com.carstenbund.audiounit.ModalAttractors"
PLUGIN_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PLUGIN_DIR}/build"
AU_BUILD_DIR="${BUILD_DIR}/au_plugin"
COMPONENT_DIR="${AU_BUILD_DIR}/${PLUGIN_NAME}.component"
INSTALL_DIR="${HOME}/Library/Audio/Plug-Ins/Components"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

function print_header() {
    echo ""
    echo "=========================================="
    echo "$1"
    echo "=========================================="
    echo ""
}

function print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

function print_error() {
    echo -e "${RED}✗${NC} $1"
}

function print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Parse command line arguments
COMMAND="${1:-build}"

case "$COMMAND" in
    clean)
        print_header "Cleaning build directories"
        rm -rf "${BUILD_DIR}"
        print_success "Clean complete"
        exit 0
        ;;
    install)
        INSTALL_AFTER_BUILD=1
        ;;
    test)
        TEST_AFTER_BUILD=1
        ;;
    build)
        ;;
    *)
        echo "Usage: $0 [clean|build|install|test]"
        exit 1
        ;;
esac

# ============================================================================
# Step 1: Build DSP Core Library
# ============================================================================

print_header "Building DSP Core Library"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [ ! -f "Makefile" ]; then
    cmake ..
fi

make -j$(sysctl -n hw.ncpu) modal_dsp_core
print_success "DSP core library built"

# ============================================================================
# Step 2: Build Audio Unit Bundle
# ============================================================================

print_header "Building Audio Unit Plugin"

# Create bundle directory structure
mkdir -p "${COMPONENT_DIR}/Contents/MacOS"
mkdir -p "${COMPONENT_DIR}/Contents/Resources"

# Copy Info.plist
cp "${PLUGIN_DIR}/Resources/Info.plist" "${COMPONENT_DIR}/Contents/"
print_success "Copied Info.plist"

# Compile Objective-C++ AU implementation
print_header "Compiling AU wrapper (Objective-C++)"

OBJCXX_FLAGS=(
    -std=c++17
    -stdlib=libc++
    -O2
    -fPIC
    -fno-common
    -Wall
    -I"${PLUGIN_DIR}/src"
    -F/System/Library/Frameworks
)

FRAMEWORKS=(
    -framework AudioToolbox
    -framework AudioUnit
    -framework CoreAudio
    -framework CoreFoundation
    -framework Foundation
    -framework AVFoundation
    -framework Cocoa
)

SOURCES=(
    "${PLUGIN_DIR}/src/au_plugin/ModalAttractorsAU.mm"
)

# Compile source files to object files
OBJECT_FILES=()
for SOURCE in "${SOURCES[@]}"; do
    OBJ_FILE="${AU_BUILD_DIR}/$(basename ${SOURCE%.mm}.o)"
    echo "  Compiling $(basename $SOURCE)..."
    clang++ "${OBJCXX_FLAGS[@]}" -c "$SOURCE" -o "$OBJ_FILE"
    OBJECT_FILES+=("$OBJ_FILE")
done

print_success "Compilation complete"

# ============================================================================
# Step 3: Link Audio Unit Bundle
# ============================================================================

print_header "Linking Audio Unit Bundle"

LINKER_FLAGS=(
    -bundle
    -Wl,-exported_symbols_list,/System/Library/Frameworks/AudioUnit.framework/Resources/AudioUnit.exp
)

clang++ "${OBJECT_FILES[@]}" \
    "${BUILD_DIR}/libmodal_dsp_core.a" \
    "${FRAMEWORKS[@]}" \
    "${LINKER_FLAGS[@]}" \
    -o "${COMPONENT_DIR}/Contents/MacOS/${PLUGIN_NAME}"

print_success "Linking complete"

# ============================================================================
# Step 4: Code Signing (if requested)
# ============================================================================

if [ -n "$CODESIGN_IDENTITY" ]; then
    print_header "Code Signing"
    codesign --force --sign "$CODESIGN_IDENTITY" "${COMPONENT_DIR}"
    print_success "Code signing complete"
else
    print_warning "Skipping code signing (set CODESIGN_IDENTITY to enable)"
fi

# ============================================================================
# Step 5: Validation
# ============================================================================

print_header "Build Summary"
echo "Plugin bundle: ${COMPONENT_DIR}"
echo "Bundle ID: ${BUNDLE_ID}"
echo "Size: $(du -sh ${COMPONENT_DIR} | cut -f1)"

if [ -f "${COMPONENT_DIR}/Contents/MacOS/${PLUGIN_NAME}" ]; then
    print_success "Build successful!"
else
    print_error "Build failed: Plugin binary not found"
    exit 1
fi

# ============================================================================
# Step 6: Install (optional)
# ============================================================================

if [ -n "$INSTALL_AFTER_BUILD" ]; then
    print_header "Installing Audio Unit"

    # Remove old version if exists
    if [ -d "${INSTALL_DIR}/${PLUGIN_NAME}.component" ]; then
        rm -rf "${INSTALL_DIR}/${PLUGIN_NAME}.component"
        print_success "Removed old version"
    fi

    # Copy to system location
    mkdir -p "${INSTALL_DIR}"
    cp -R "${COMPONENT_DIR}" "${INSTALL_DIR}/"
    print_success "Installed to ${INSTALL_DIR}/${PLUGIN_NAME}.component"

    # Invalidate AU cache
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    print_success "AU cache invalidated"
fi

# ============================================================================
# Step 7: Test (optional)
# ============================================================================

if [ -n "$TEST_AFTER_BUILD" ]; then
    print_header "Validating Audio Unit"

    # Run auval (Audio Unit validation tool)
    # Format: auval -v <type> <subtype> <manufacturer>
    # type: aumi (instrument)
    # subtype: modl (Modal Attractors)
    # manufacturer: CBND (Carsten Bund)

    echo "Running auval validation..."
    auval -v aumi modl CBND || {
        print_error "auval validation failed"
        exit 1
    }
    print_success "auval validation passed"
fi

print_header "Done!"
echo "Next steps:"
echo "  1. Install: ./build_au_plugin.sh install"
echo "  2. Test:    ./build_au_plugin.sh test"
echo "  3. Open Logic Pro X and look for 'Modal Attractors' in the AU Instruments"
echo ""
