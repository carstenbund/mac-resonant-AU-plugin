# Building on macOS - Modal Attractors AU Plugin

This guide is for building the AU plugin on macOS with Xcode.

## Prerequisites

1. **macOS 12.0+** (Monterey or later)
2. **Xcode 14+** installed from the Mac App Store
3. **Xcode Command Line Tools:**
   ```bash
   xcode-select --install
   ```
4. **CMake** (optional, for Xcode project generation):
   ```bash
   brew install cmake
   ```

## Quick Start

### Option 1: Using CMake + Xcode (Recommended for DSP Core)

```bash
# Generate Xcode project
./build_xcode.sh xcode

# Open in Xcode
open build-xcode/ModalAttractors.xcodeproj

# Or build from command line
./build_xcode.sh build
```

This builds the **DSP core library** only. For the full AU plugin, continue to Option 2.

### Option 2: Manual Xcode Project (Required for AU Plugin)

Since the AU plugin requires Objective-C++ and AudioUnit frameworks, you need to create a proper Xcode project:

#### Step 1: Create New Xcode Project

1. Open Xcode
2. File → New → Project
3. Choose **macOS** → **Bundle**
4. Product Name: `ModalAttractors`
5. Bundle Identifier: `com.carstenbund.audiounit.ModalAttractors`
6. Save in project directory

#### Step 2: Configure Bundle as Audio Unit

1. Select project in navigator
2. Under **General**:
   - Set Deployment Target: `macOS 12.0`
   - Set Architectures: `arm64, x86_64` (Universal)

3. Under **Build Settings**:
   - **Wrapper Extension**: `component`
   - **Product Name**: `ModalAttractors`
   - **C++ Language Dialect**: `C++17`
   - **C++ Standard Library**: `libc++`

4. Under **Info** tab:
   - Copy contents from `Resources/Info.plist`
   - Or add Info.plist to project

#### Step 3: Add Source Files

Add to Xcode project:

**DSP Core (C/C++):**
- `src/esp32_port/modal_node.c`
- `src/esp32_port/modal_node.h`
- `src/esp32_port/audio_synth.c`
- `src/esp32_port/audio_synth.h`
- `src/dsp_core/ModalVoice.cpp`
- `src/dsp_core/ModalVoice.h`
- `src/dsp_core/VoiceAllocator.cpp`
- `src/dsp_core/VoiceAllocator.h`
- `src/dsp_core/TopologyEngine.cpp`
- `src/dsp_core/TopologyEngine.h`
- `src/au_wrapper/ModalAttractorsEngine.cpp`
- `src/au_wrapper/ModalParameters.h`

**AU Plugin (Objective-C++):**
- `src/au_plugin/ModalAttractorsAU.mm` ⭐ Main AU implementation

#### Step 4: Add Frameworks

In **Build Phases** → **Link Binary With Libraries**, add:
- `AudioToolbox.framework`
- `AudioUnit.framework`
- `CoreAudio.framework`
- `AVFoundation.framework`
- `Foundation.framework`
- `Cocoa.framework`

#### Step 5: Build

1. Select scheme: **ModalAttractors**
2. Product → Build (⌘B)
3. Find built plugin in DerivedData

#### Step 6: Install

```bash
# Copy to system AU folder
cp -R ~/Library/Developer/Xcode/DerivedData/.../ModalAttractors.component \
     ~/Library/Audio/Plug-Ins/Components/

# Invalidate AU cache
killall -9 AudioComponentRegistrar
```

#### Step 7: Validate

```bash
# Validate with auval
auval -v aumi modl CBND
```

Should output: `PASSED` with no errors.

#### Step 8: Test in Logic Pro X

1. Open Logic Pro X
2. Create Software Instrument track
3. Click instrument slot
4. Navigate to: **AU Instruments** → **Carsten Bund** → **Modal Attractors**
5. Load plugin
6. Play MIDI notes - should hear audio output
7. Test parameters in plugin window

---

## Troubleshooting

### "make: command not found"

Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### "No such file or directory: AudioToolbox/AudioToolbox.h"

You need to build using Xcode (not Linux tools). The AudioUnit SDK is macOS-only.

### "Code signature invalid"

For development:
1. Xcode → Preferences → Accounts
2. Add your Apple ID
3. In project settings, select your Development Team
4. Product → Clean Build Folder
5. Rebuild

For distribution, you need an Apple Developer account.

### Plugin doesn't appear in Logic Pro X

```bash
# Check if installed
ls ~/Library/Audio/Plug-Ins/Components/ModalAttractors.component

# Invalidate AU cache
killall -9 AudioComponentRegistrar

# Restart Logic Pro X
```

### auval fails

Common issues:
- **Info.plist incorrect**: Check bundle identifier, AU type/subtype
- **Missing frameworks**: Ensure all frameworks linked
- **Architecture mismatch**: Build for both arm64 and x86_64
- **Code signing**: Sign with ad-hoc signature: `codesign -s - ModalAttractors.component`

Check detailed errors:
```bash
auval -v aumi modl CBND 2>&1 | less
```

---

## Alternative: Build DSP Core Only (for testing)

If you just want to test the DSP core without the full AU:

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build standalone test
make test_modal_voice

# Run test (generates test_output.wav)
./test_modal_voice

# Listen to output
open test_output.wav
```

This validates the DSP engine works correctly before integrating into AU.

---

## Project Structure

```
mac-resonant-AU-plugin/
├── src/
│   ├── esp32_port/          # C DSP core (from ESP32)
│   ├── dsp_core/            # C++ DSP wrappers
│   ├── au_wrapper/          # C++ AU engine wrapper
│   └── au_plugin/           # Objective-C++ AU implementation ⭐
├── Resources/
│   └── Info.plist           # AU bundle configuration ⭐
├── Tests/
│   └── test_modal_voice.cpp # Standalone DSP test
├── CMakeLists.txt           # CMake build (for DSP core)
├── build_xcode.sh           # Xcode project generator
└── BUILD_MACOS.md           # This file

⭐ = Essential for AU plugin
```

---

## Next Steps After Building

1. **Test basic functionality:**
   - MIDI input (notes trigger sound)
   - Audio output (generates audio)
   - Parameters (gain, coupling, topology)

2. **Expand parameters:**
   - Add per-mode frequency/damping/weight controls
   - See `src/au_wrapper/ModalParameters.h` for parameter IDs

3. **Add preset system:**
   - Implement factory presets
   - Add preset save/load

4. **Create custom GUI:**
   - Add Cocoa view controller
   - Visualize topology network
   - Add custom parameter controls

5. **Optimize performance:**
   - Profile with Instruments
   - Add SIMD optimizations (Accelerate framework)
   - Test at different sample rates

---

## Resources

- [Apple Audio Unit Programming Guide](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/)
- [AUv3 Sample Code](https://developer.apple.com/documentation/audiotoolbox/audio_unit_v3_plug-ins)
- Phase 1 Status: `PHASE1_STATUS.md`
- Phase 2 Status: `PHASE2_STATUS.md`

---

## Questions?

See the main README.md or create an issue on GitHub.
