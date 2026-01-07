# Testing Modal Attractors in Xcode

This guide explains how to run the comprehensive test suite using Xcode on macOS.

---

## Quick Start

### 1. Generate Xcode Project

From the project root directory on your Mac:

```bash
./generate_xcode_project.sh
```

This creates `build_xcode/ModalAttractors.xcodeproj` with all test targets.

### 2. Open in Xcode

```bash
open build_xcode/ModalAttractors.xcodeproj
```

Or double-click `ModalAttractors.xcodeproj` in Finder.

---

## Available Test Schemes

The Xcode project includes these schemes (select from scheme dropdown in toolbar):

### 📊 **test_suite** (Recommended)
Comprehensive 7-test suite covering all functionality:
- Test 1: Single voice decay
- Test 2: Multiple excitations
- Test 3: Polyphony (4-voice chord)
- Test 4: Topology comparison (4 variants)
- Test 5: MIDI scale response
- Test 6: Multichannel (8 channels)
- Test 7: Parameter sweep (damping)

**Outputs**: 10 WAV files (16.5 MB total)

### 🎵 **test_modal_voice**
Original single-voice test for basic validation.

**Outputs**: 1 WAV file (output.wav)

---

## Running Tests in Xcode

### Method 1: Direct Run (Simplest)

1. **Select scheme** from dropdown (top-left): `test_suite` or `test_modal_voice`
2. **Set working directory**:
   - Product → Scheme → Edit Scheme... (or Cmd+<)
   - Select "Run" in left sidebar
   - Go to "Options" tab
   - Check "Use custom working directory"
   - Set to: `$PROJECT_DIR/build_xcode`
3. **Run**: Press **Cmd+R** or click ▶ (Play button)
4. **View output**: Check Console (Cmd+Shift+C) for test results
5. **Find WAV files**: In `build_xcode/` directory

### Method 2: Build & Run from Terminal

```bash
cd build_xcode

# Build
xcodebuild -scheme test_suite -configuration Debug

# Run
./Debug/test_suite

# Analyze results
python3 ../check_audio.py test_*.wav
```

---

## Viewing Test Results

### In Xcode Console

After running, you'll see output like:

```
========================================
TEST 1: Single Voice - Excitation & Decay
========================================
✓ Wrote 240000 samples to test_01_single_voice_decay.wav (2 channels)
Expected: Clean decay over 5s with 2Hz beating
```

### Analyzing WAV Files

**Option 1: Automated Analysis** (from `build_xcode/` directory)
```bash
python3 ../check_audio.py test_*.wav
```

Shows:
- Peak/RMS levels
- Discontinuities (clicks/pops)
- Envelope decay
- DC offset
- Clipping

**Option 2: Visual Inspection**

Open WAV files in:
- **Audacity** (free): File → Open → `test_*.wav`
- **Logic Pro X**: Drag into empty project
- **QuickTime Player**: For quick playback

---

## Expected Test Results

| Test | Duration | Channels | File Size | Key Validation |
|------|----------|----------|-----------|----------------|
| Test 1 | 5.0s | 2 | 1.1 MB | Smooth decay, 2 Hz beating |
| Test 2 | 6.0s | 2 | 1.3 MB | 8 pitch changes audible |
| Test 3 | 5.0s | 2 | 1.1 MB | C major chord harmony |
| Test 4a-d | 4.0s each | 2 | 882 KB each | Different decay patterns |
| Test 5 | 8.0s | 2 | 1.8 MB | C major scale melody |
| Test 6 | 5.0s | **8** | 4.4 MB | 8 isolated channels |
| Test 7 | 10.0s | 2 | 2.2 MB | 5 distinct decay rates |

### Success Criteria

✅ **All tests should show**:
- Max sample jump < 0.02 (no clicks/pops)
- No clipping (peak < 1.0)
- Smooth exponential decay
- No DC offset

See `TEST_RESULTS.md` for detailed analysis.

---

## Debugging in Xcode

### Setting Breakpoints

1. Open source file (e.g., `Tests/test_suite.cpp`)
2. Click line number to set breakpoint
3. Run with **Cmd+R**
4. Use debugger controls to step through

### Useful Breakpoints

```cpp
// Tests/test_suite.cpp
Line 187: void test_single_voice_decay()     // Test 1 entry
Line 327: void test_multichannel()           // Test 6 entry

// src/dsp_core/ModalVoice.cpp
Line 74:  void ModalVoice::renderAudio()     // Main audio render
Line 42:  void ModalVoice::updateModal()     // Modal dynamics (500 Hz)

// src/esp32_port/audio_synth.c
Line 117: void audio_synth_render()          // Low-level synthesis
```

### Inspecting Variables

When paused at breakpoint:
- **Hover** over variable to see value
- **Right-click → Print Description** for detailed view
- **View → Debug Area → Variables View** (Cmd+Shift+Y)

Useful variables to inspect:
```cpp
// In ModalVoice::renderAudio()
samples_since_update_     // Should increment by buffer size
synth_.nodes[0].modes[0]  // Modal state (frequency, damping, amplitude)

// In audio_synth_render()
phase_acc                 // Phase accumulator (should be continuous)
amplitude                 // Output level
```

---

## Troubleshooting

### Issue: "Command-line tool not found"

**Solution**: Install Xcode Command Line Tools
```bash
xcode-select --install
```

### Issue: "Working directory not set, WAV files not found"

**Solution**: Set custom working directory in scheme settings (see Method 1 above)

### Issue: "Python script not found"

**Solution**: Run from correct directory
```bash
cd build_xcode
python3 ../check_audio.py test_*.wav
```

### Issue: Architecture mismatch errors

**Solution**: Rebuild project for your Mac's architecture

For Apple Silicon (M1/M2/M3):
```bash
cmake -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64" ..
```

For Intel:
```bash
cmake -G Xcode -DCMAKE_OSX_ARCHITECTURES="x86_64" ..
```

For Universal Binary (both):
```bash
cmake -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ..
```

### Issue: Test fails with "Segmentation fault"

**Common causes**:
1. Check `ModalVoice::initialize()` was called
2. Verify `VoiceAllocator::initialize()` was called
3. Ensure buffer sizes are valid (> 0, < 1000000)

**Debug**:
- Run with Address Sanitizer: Edit Scheme → Diagnostics → Enable Address Sanitizer
- Check Console for detailed error location

---

## Performance Profiling

### Using Xcode Instruments

1. **Product → Profile** (Cmd+I)
2. Select **Time Profiler** template
3. Click **Record** (red button)
4. Tests run automatically
5. View call tree to identify hotspots

**Expected results**:
- `audio_synth_render()`: 60-70% of CPU time
- `sinf()`: 20-30% of CPU time
- `modal_node_update()`: 5-10% of CPU time

### CPU Usage

**Single voice** (test_modal_voice):
- ~1-2% CPU on M1 Mac (48 kHz, 512 samples/buffer)

**8 voices** (test_suite, Test 6):
- ~8-12% CPU on M1 Mac

---

## Comparing with Python Reference

To validate C++ implementation matches Python behavior:

### 1. Run Python version
```bash
cd src
python3 network.py
# Generates audio with carrier-based AM synthesis
```

### 2. Run C++ test suite
```bash
cd build_xcode
./Debug/test_suite
```

### 3. Compare spectrograms

In Audacity:
- Analyze → Plot Spectrum (Shift+Cmd+P)
- Compare fundamental frequency peaks
- Verify modal frequencies present in C++ version

**Note**: Direct waveform comparison won't match because Python uses carrier + AM synthesis while C++ uses direct modal synthesis. See `AUDIO_SYNTHESIS_COMPARISON.md` for details.

---

## Continuous Testing Workflow

### Recommended workflow during development:

1. **Make code changes** in Xcode editor
2. **Build**: Cmd+B
3. **Run tests**: Cmd+R (scheme: test_suite)
4. **Check console** for "✓ Wrote..." messages
5. **Analyze audio** if needed:
   ```bash
   cd build_xcode
   python3 ../check_audio.py test_01_*.wav
   ```
6. **Listen** to critical tests in QuickTime
7. **Repeat**

### Quick validation (just Test 1):

Change `main()` in `test_suite.cpp`:
```cpp
int main() {
    test_single_voice_decay();  // Just this one
    // test_multiple_excitations();
    // ... (comment out others)
    return 0;
}
```

Rebuild and run for faster iteration.

---

## Next Steps After Testing

Once all tests pass on macOS:

1. **Build AU plugin**: Enable in CMakeLists.txt
   ```cmake
   option(BUILD_AU_PLUGIN "Build Audio Unit plugin" ON)
   ```

2. **Regenerate Xcode project**:
   ```bash
   ./generate_xcode_project.sh
   ```

3. **Load in Logic Pro X**:
   - Open Logic
   - Create Software Instrument track
   - Select "Modal Attractors" from plugin list
   - Play MIDI notes

4. **Validate in DAW**:
   - MIDI input response
   - Parameter automation
   - Multi-output routing (8 channels)
   - CPU usage in real session

See `BUILD_MACOS.md` for AU plugin build instructions.

---

## Additional Resources

- **TEST_PLAN.md**: Detailed test methodology
- **TEST_RESULTS.md**: Expected results and validation criteria
- **BUILD_MACOS.md**: macOS build instructions
- **PYTHON_COMPARISON.md**: Python vs C++ architecture differences

---

## Quick Reference Commands

```bash
# Generate Xcode project
./generate_xcode_project.sh

# Open in Xcode
open build_xcode/ModalAttractors.xcodeproj

# Build from terminal
cd build_xcode
xcodebuild -scheme test_suite -configuration Debug

# Run tests
./Debug/test_suite

# Analyze results
python3 ../check_audio.py test_*.wav

# Automated test run
cd build_xcode
cmake --build . --target test_suite
./Debug/test_suite
python3 ../check_audio.py test_*.wav
```

---

**Happy Testing!** 🎵

For issues or questions, see `TEST_PLAN.md` debugging section.
