#!/bin/bash
#
# Generate Xcode project for Modal Attractors AU Plugin
# Run this script on macOS to create/update the Xcode project with all test targets
#

set -e

echo "========================================="
echo "Generating Xcode Project for Modal Attractors"
echo "========================================="

# Clean previous build directory
if [ -d "build_xcode" ]; then
    echo "Removing old build_xcode directory..."
    rm -rf build_xcode
fi

# Create fresh build directory
mkdir build_xcode
cd build_xcode

echo ""
echo "Running CMake with Xcode generator..."
echo ""

# Generate Xcode project
# - Builds for both Apple Silicon (arm64) and Intel (x86_64)
# - Creates separate schemes for each target
cmake -G Xcode \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.15" \
    ..

echo ""
echo "========================================="
echo "✅ Xcode Project Generated Successfully!"
echo "========================================="
echo ""
echo "Open the project with:"
echo "  open build_xcode/ModalAttractors.xcodeproj"
echo ""
echo "Available schemes in Xcode:"
echo "  • test_modal_voice    - Original single voice test"
echo "  • test_suite          - Comprehensive 7-test suite"
echo "  • modal_dsp_core      - DSP library only"
echo "  • ALL_BUILD           - Build everything"
echo ""
echo "To run tests from Xcode:"
echo "  1. Select scheme (e.g., 'test_suite')"
echo "  2. Product → Scheme → Edit Scheme"
echo "  3. Run → Options → Working Directory"
echo "  4. Set to: \$PROJECT_DIR/build_xcode"
echo "  5. Press Cmd+R to run"
echo ""
echo "Output WAV files will be in: build_xcode/"
echo ""
