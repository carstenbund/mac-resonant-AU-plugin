#!/bin/bash
#
# Build script for Modal Attractors Audio Unit plugin (macOS Xcode)
#
# This script generates an Xcode project and builds the AU plugin
#
# Usage:
#   ./build_xcode.sh [xcode|build|install|test|clean]
#

set -e

PLUGIN_NAME="ModalAttractors"
PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build-xcode"
INSTALL_DIR="${HOME}/Library/Audio/Plug-Ins/Components"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

function print_header() {
    echo ""
    echo -e "${BLUE}=========================================="
    echo "$1"
    echo -e "==========================================${NC}"
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

COMMAND="${1:-xcode}"

case "$COMMAND" in
    clean)
        print_header "Cleaning build directories"
        rm -rf "${BUILD_DIR}"
        rm -rf ModalAttractors.xcodeproj 2>/dev/null || true
        print_success "Clean complete"
        exit 0
        ;;
    xcode)
        # Generate Xcode project
        ;;
    build)
        BUILD_ONLY=1
        ;;
    install)
        INSTALL_AFTER_BUILD=1
        ;;
    test)
        TEST_AFTER_BUILD=1
        ;;
    *)
        echo "Usage: $0 [xcode|build|install|test|clean]"
        echo ""
        echo "  xcode   - Generate Xcode project (default)"
        echo "  build   - Build using xcodebuild"
        echo "  install - Build and install to ~/Library/Audio/Plug-Ins/Components/"
        echo "  test    - Build and validate with auval"
        echo "  clean   - Clean all build artifacts"
        exit 1
        ;;
esac

# ============================================================================
# Generate Xcode Project
# ============================================================================

if [ "$COMMAND" = "xcode" ]; then
    print_header "Generating Xcode Project"

    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"

    # Generate Xcode project with CMake
    cmake -G Xcode \
        -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0" \
        ..

    print_success "Xcode project generated: ${BUILD_DIR}/ModalAttractors.xcodeproj"
    echo ""
    echo "To open in Xcode:"
    echo "  open ${BUILD_DIR}/ModalAttractors.xcodeproj"
    echo ""
    echo "Or build from command line:"
    echo "  ./build_xcode.sh build"

    exit 0
fi

# ============================================================================
# Build with xcodebuild
# ============================================================================

if [ ! -d "${BUILD_DIR}" ]; then
    print_error "Xcode project not generated. Run: ./build_xcode.sh xcode"
    exit 1
fi

print_header "Building with xcodebuild"

cd "${BUILD_DIR}"

xcodebuild -project ModalAttractors.xcodeproj \
    -scheme ALL_BUILD \
    -configuration Release \
    -arch arm64 -arch x86_64 \
    build

print_success "Build complete"

# ============================================================================
# Create AU Bundle
# ============================================================================

print_header "Creating Audio Unit Bundle"

COMPONENT_DIR="${BUILD_DIR}/${PLUGIN_NAME}.component"
mkdir -p "${COMPONENT_DIR}/Contents/MacOS"
mkdir -p "${COMPONENT_DIR}/Contents/Resources"

# Copy Info.plist
cp "${PROJECT_DIR}/Resources/Info.plist" "${COMPONENT_DIR}/Contents/"
print_success "Copied Info.plist"

# For now, the AU wrapper needs to be built separately since we need
# the Objective-C++ implementation. This is a placeholder.
print_warning "AU bundle created but needs manual Objective-C++ compilation"
print_warning "See PHASE2_STATUS.md for complete build instructions"

# ============================================================================
# Install (optional)
# ============================================================================

if [ -n "$INSTALL_AFTER_BUILD" ]; then
    print_header "Installing Audio Unit"

    if [ -d "${INSTALL_DIR}/${PLUGIN_NAME}.component" ]; then
        rm -rf "${INSTALL_DIR}/${PLUGIN_NAME}.component"
        print_success "Removed old version"
    fi

    mkdir -p "${INSTALL_DIR}"
    cp -R "${COMPONENT_DIR}" "${INSTALL_DIR}/"
    print_success "Installed to ${INSTALL_DIR}/${PLUGIN_NAME}.component"

    killall -9 AudioComponentRegistrar 2>/dev/null || true
    print_success "AU cache invalidated"
fi

# ============================================================================
# Test (optional)
# ============================================================================

if [ -n "$TEST_AFTER_BUILD" ]; then
    print_header "Validating Audio Unit"

    echo "Running auval validation..."
    auval -v aumi modl CBND || {
        print_error "auval validation failed"
        exit 1
    }
    print_success "auval validation passed"
fi

print_header "Done!"
