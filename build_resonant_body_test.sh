#!/bin/bash
#
# Build script for ResonantBodyProcessor standalone test
# Compiles all necessary DSP components and test executable
#

set -e  # Exit on error

echo "========================================="
echo "  ModalAttractor Resonant Body Test"
echo "  Build Script"
echo "========================================="
echo ""

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="${SCRIPT_DIR}"
ESP32_PORT="${SRC_DIR}/ModalAttractors/ModalAttractorsExtension/DSP"
DSP_DIR="${SRC_DIR}/ModalAttractors/DSP"
TEST_DIR="${SRC_DIR}/Tests"
BUILD_DIR="${SRC_DIR}/build_resonant_body"

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "Build directory: ${BUILD_DIR}"
echo ""

# Compilers
CC="clang"
CXX="clang++"
CFLAGS="-std=c11 -Wall -Wextra -O2 -ffast-math"
CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -ffast-math"
INCLUDES="-I${ESP32_PORT} -I${DSP_DIR}"
LIBS="-lm -lstdc++"

echo "C Compiler: ${CC}"
echo "C++ Compiler: ${CXX}"
echo "C Flags: ${CFLAGS}"
echo "C++ Flags: ${CXXFLAGS}"
echo ""

# Source files
ESP32_SOURCES=(
    "${ESP32_PORT}/modal_node.c"
    "${ESP32_PORT}/audio_synth.c"
)

DSP_SOURCES=(
    "${DSP_DIR}/EnergyExtractor.cpp"
    "${DSP_DIR}/SpectralAnalyzer.cpp"
    "${DSP_DIR}/PitchDetector.cpp"
    "${DSP_DIR}/ResonantBodyProcessor.cpp"
)

TEST_SOURCE="${TEST_DIR}/test_resonant_body.cpp"

# Compile ESP32 C sources (use C compiler for .c files)
echo "Compiling ESP32 C sources..."
for src in "${ESP32_SOURCES[@]}"; do
    obj=$(basename "$src" .c).o
    echo "  - $src -> $obj"
    $CC $CFLAGS $INCLUDES -c "$src" -o "$obj"
done
echo ""

# Compile DSP C++ sources
echo "Compiling DSP C++ sources..."
for src in "${DSP_SOURCES[@]}"; do
    obj=$(basename "$src" .cpp).o
    echo "  - $src -> $obj"
    $CXX $CXXFLAGS $INCLUDES -c "$src" -o "$obj"
done
echo ""

# Compile test
echo "Compiling test application..."
echo "  - $TEST_SOURCE -> test_resonant_body.o"
$CXX $CXXFLAGS $INCLUDES -c "$TEST_SOURCE" -o test_resonant_body.o
echo ""

# Link executable (use C++ linker since we have C++ code)
echo "Linking executable..."
$CXX -o test_resonant_body \
    modal_node.o \
    audio_synth.o \
    EnergyExtractor.o \
    SpectralAnalyzer.o \
    PitchDetector.o \
    ResonantBodyProcessor.o \
    test_resonant_body.o \
    $LIBS

echo "  - Executable: test_resonant_body"
echo ""

echo "========================================="
echo "  Build successful!"
echo "========================================="
echo ""
echo "To run the test:"
echo "  cd ${BUILD_DIR}"
echo "  ./test_resonant_body"
echo ""
echo "This will generate 4 WAV files:"
echo "  1. test_resonant_body_impulse.wav"
echo "  2. test_resonant_body_sweep.wav"
echo "  3. test_resonant_body_noise.wav"
echo "  4. test_resonant_body_morph.wav"
echo ""
