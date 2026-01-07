#!/bin/bash
#
# Comprehensive test runner for Modal Attractors DSP
# Runs all tests and analyzes output
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}MODAL ATTRACTORS - TEST RUNNER${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Build if needed
if [ ! -f "build/test_suite" ]; then
    echo -e "${YELLOW}Building test suite...${NC}"
    mkdir -p build
    cd build
    cmake .. > /dev/null 2>&1
    make -j4 test_suite
    cd ..
    echo -e "${GREEN}✓ Build complete${NC}"
    echo ""
fi

# Run tests
cd build
echo -e "${BLUE}Running test suite...${NC}"
echo ""
./test_suite

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}ANALYZING TEST OUTPUTS${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Analyze each test file
for wavfile in test_*.wav; do
    if [ -f "$wavfile" ]; then
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${GREEN}$wavfile${NC}"
        echo -e "${YELLOW}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        python3 ../check_audio.py "$wavfile"
        echo ""
    fi
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}TEST SUMMARY${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Generated test files:"
ls -lh test_*.wav | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "To view waveforms:"
echo "  open test_*.wav"
echo "  Or import into DAW/audio editor"
echo ""
echo -e "${GREEN}✓ All tests complete${NC}"
