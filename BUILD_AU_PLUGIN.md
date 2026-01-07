# Building the Modal Attractors Audio Unit Plugin

This guide explains how to build the AU plugin on macOS and load it in Logic Pro X.

---

## Prerequisites

1. **macOS 12.0+** (Monterey or later recommended)
2. **Xcode 14+** with Command Line Tools installed
3. **Test suite passing** - Run tests first to verify DSP core works:
   ```bash
   ./generate_xcode_project.sh
   open build_xcode/ModalAttractors.xcodeproj
   # Select "test_suite" scheme and press Cmd+R
   ```

---

## Quick Start

### Option 1: Manual Xcode Project (Most Reliable)

Follow the step-by-step instructions in **[Manual Xcode Project Setup](#manual-xcode-project-setup)** below.

### Option 2: CMake-based Build (Experimental)

Coming soon - requires CMake AudioUnit bundle support.

---

## Manual Xcode Project Setup

### Step 1: Create New Xcode Project

1. **Open Xcode**
2. **File → New → Project**
3. **macOS** tab → **App Extension** → **Audio Unit Extension**
4. Click **Next**

5. **Configure the project**:
   - **Product Name**: `ModalAttractors`
   - **Bundle Identifier**: `com.carstenbund.audiounit.ModalAttractors`
   - **Language**: Objective-C
   - **Embed in Application**: Create a new macOS app called `ModalAttractorsHost`

6. **Save** in your project directory (e.g., `~/mac-resonant-AU-plugin/xcode_au/`)

**Note**: Xcode creates two targets:
- `ModalAttractors` - The AU extension (this is the plugin)
- `ModalAttractorsHost` - A simple host app for testing

### Step 2: Add Source Files to Project

1. **Select the ModalAttractors extension target** in Xcode project navigator

2. **Delete auto-generated files** (we'll replace them):
   - Right-click → Delete → "Move to Trash":
     - `AudioUnitViewController.h`
     - `AudioUnitViewController.m`
     - `ModalAttractorsAudioUnit.h`
     - `ModalAttractorsAudioUnit.m`

3. **Add our AU implementation files**:
   - **File → Add Files to "ModalAttractors"...**
   - Navigate to `src/au_wrapper/`
   - Select:
     - `ModalAttractorsAU.mm`
     - `ModalAttractorsDSPKernel.h`
     - `ModalAttractorsDSPKernel.mm`
     - `ModalAttractorsAU.h`
     - `ModalAttractorsEngine.cpp`
     - `ModalParameters.h`
   - **Make sure** "Copy items if needed" is **unchecked** (reference files)
   - **Make sure** target "ModalAttractors" is **checked**
   - Click **Add**

4. **Add DSP core files**:
   - **File → Add Files to "ModalAttractors"...**
   - Navigate to `src/dsp_core/`
   - Select all `.cpp` and `.h` files:
     - `ModalVoice.cpp` / `.h`
     - `VoiceAllocator.cpp` / `.h`
     - `TopologyEngine.cpp` / `.h`
   - **Uncheck** "Copy items if needed"
   - **Check** target "ModalAttractors"
   - Click **Add**

5. **Add ESP32 port files** (C code):
   - **File → Add Files to "ModalAttractors"...**
   - Navigate to `src/esp32_port/`
   - Select:
     - `modal_node.c` / `.h`
     - `audio_synth.c` / `.h`
   - **Uncheck** "Copy items if needed"
   - **Check** target "ModalAttractors"
   - Click **Add**

### Step 3: Replace Info.plist

1. **In Xcode project navigator**, find `Info.plist` under the `ModalAttractors` target

2. **Right-click → Delete → Move to Trash**

3. **Add our Info.plist**:
   - **File → Add Files to "ModalAttractors"...**
   - Navigate to `src/au_wrapper/`
   - Select `Info.plist`
   - **Check** "Copy items if needed" this time
   - **Check** target "ModalAttractors"
   - Click **Add**

4. **Verify Info.plist is set as bundle info**:
   - Select **ModalAttractors target** → **Build Settings**
   - Search for "info.plist"
   - **Info.plist File** should show: `ModalAttractors/Info.plist`

### Step 4: Configure Build Settings

1. **Select ModalAttractors target** → **Build Settings**

2. **Search for each setting** and configure:

   **C++ Language Settings**:
   - **C++ Language Dialect**: `C++17 [-std=c++17]`
   - **C++ Standard Library**: `libc++ (LLVM C++ standard library)`

   **Header Search Paths**:
   - Add: `$(PROJECT_DIR)/../src/dsp_core`
   - Add: `$(PROJECT_DIR)/../src/esp32_port`
   - Add: `$(PROJECT_DIR)/../src/au_wrapper`
   - **Recursive**: No

   **Architectures**:
   - **Architectures**: `arm64, x86_64` (Universal Binary)
   - **Build Active Architecture Only**: No (for Release)

   **Deployment**:
   - **macOS Deployment Target**: `12.0`

   **Linking**:
   - **Other Linker Flags**: `-framework AudioToolbox -framework AVFoundation -framework CoreAudio`

   **Code Signing** (if testing locally):
   - **Code Signing Identity**: `Development`
   - **Enable Hardened Runtime**: Yes
   - **Provisioning Profile**: Automatic

3. **Compiler Flags** (optional, for warnings):
   - Search for "Other C Flags"
   - Add: `-Wno-unused-parameter -Wno-unused-variable`

### Step 5: Build the Plugin

1. **Select scheme**: `ModalAttractors` (not ModalAttractorsHost)

2. **Select destination**: My Mac

3. **Build**: **Product → Build** (Cmd+B)

4. **Check for errors**:
   - Build should succeed with 0 errors
   - Warnings are OK (unused parameters, etc.)

5. **Find the plugin**:
   - **Product → Show Build Folder in Finder**
   - Navigate to `Debug/ModalAttractors.appex`
   - This is your AU plugin bundle!

### Step 6: Install the Plugin

Audio Units must be in specific system locations to be discovered.

**For Development/Testing** (easiest):
```bash
# Copy to user AudioUnits folder
cp -R ~/Library/Developer/Xcode/DerivedData/.../Debug/ModalAttractors.appex \
     ~/Library/Audio/Plug-Ins/Components/

# Or create symlink (auto-updates when you rebuild)
ln -s ~/Library/Developer/Xcode/DerivedData/.../Debug/ModalAttractors.appex \
      ~/Library/Audio/Plug-Ins/Components/ModalAttractors.appex
```

**For Release**:
```bash
# Build Release configuration first
# Then copy to system-wide location (requires admin)
sudo cp -R .../Release/ModalAttractors.appex \
           /Library/Audio/Plug-Ins/Components/
```

**Create the directory** if it doesn't exist:
```bash
mkdir -p ~/Library/Audio/Plug-Ins/Components
```

### Step 7: Validate the AU Plugin

macOS includes a tool to validate Audio Units:

```bash
# Find your AU component
auval -a

# Validate Modal Attractors specifically
# Type: aumi (Music Instrument)
# Subtype: mdla (Modal Attractors)
# Manufacturer: Cbnd (Carsten Bund)
auval -v aumi mdla Cbnd

# Should output:
# VALIDATING AUDIO UNIT: 'aumi' - 'mdla' - 'Cbnd'
# ...
# * * PASS
```

**Common validation errors**:
- `FATAL ERROR: OpenAComponent: result: -2147450880` - Plugin not found, check install path
- `AU Validation FAILED` - Check console logs for details
- Crashes - Run in debugger to find issue

### Step 8: Test in Logic Pro X

1. **Restart Logic Pro X** (or any other DAW) to discover the plugin

2. **Create new project**:
   - **File → New**
   - Select **Empty Project**

3. **Add Software Instrument track**:
   - Click **+** or **Track → New Software Instrument Track**

4. **Load Modal Attractors**:
   - In Channel Strip, click **Instrument** slot (currently shows "No Plug-in")
   - Navigate: **AU Instruments → Carsten Bund → Modal Attractors**

5. **Test MIDI**:
   - Click **R** (Record enable) on the track
   - Play notes on MIDI keyboard or use musical typing (Cmd+K)
   - You should hear modal synthesis!

6. **Adjust parameters**:
   - Click plugin to open controls
   - Adjust: Master Gain, Coupling Strength, Topology
   - Hear the changes in real-time

7. **Check performance**:
   - **Mixer → Fader → CPU** - Should show low CPU usage (<5%)

---

## Debugging the Plugin

### Attach Xcode Debugger to Logic Pro X

1. **Build Debug configuration** in Xcode

2. **Product → Scheme → Edit Scheme**

3. **Run** → **Info** tab:
   - **Executable**: Choose "Other..."
   - Navigate to: `/Applications/Logic Pro X.app`

4. **Run** (Cmd+R):
   - Logic Pro X launches with debugger attached
   - Load Modal Attractors plugin
   - Breakpoints in your AU code will trigger!

### Useful Breakpoints

```cpp
// ModalAttractorsAU.mm
Line 62:  initWithComponentDescription:  // AU initialization
Line 174: internalRenderBlock            // Audio render setup
Line 200: kernel->render()               // Audio rendering

// ModalAttractorsDSPKernel.mm
Line 34:  render()                       // DSP processing
Line 50:  handleMIDI()                   // MIDI input

// ModalAttractorsEngine.cpp
Line 77:  modal_attractors_engine_render // Engine render
Line 62:  modal_attractors_engine_note_on // Note handling
```

### View Console Logs

```bash
# Monitor macOS system logs for AU errors
log stream --predicate 'subsystem == "com.apple.audio"' --level debug
```

### Common Issues

**Issue**: Plugin doesn't appear in Logic

**Solutions**:
1. Check AU is installed in correct location
2. Run `auval -a` to see if it's discovered
3. Restart Logic Pro X
4. Check Console.app for error messages
5. Verify Info.plist bundle identifier matches

**Issue**: Plugin crashes on load

**Solutions**:
1. Attach Xcode debugger (see above)
2. Check crash log in Console.app
3. Verify all source files are included in target
4. Check C++ exceptions aren't escaping to Objective-C

**Issue**: No audio output

**Solutions**:
1. Set breakpoint in `render()` to verify it's being called
2. Check MIDI is triggering notes (breakpoint in `handleMIDI()`)
3. Verify sample rate is valid (48kHz standard)
4. Check master gain parameter isn't 0

**Issue**: Clicking/popping in audio

**Solutions**:
1. This was fixed in the test suite! Verify:
   - Tests pass cleanly
   - `updateModal()` called at 500 Hz
   - Phase accumulator is continuous
2. Check buffer size isn't too small (<64 frames)

---

## Performance Profiling

### Using Xcode Instruments

1. **Build Release configuration** for accurate profiling

2. **Product → Profile** (Cmd+I)

3. **Select template**: Time Profiler

4. **Set target**: Logic Pro X

5. **Record**:
   - Load Modal Attractors in Logic
   - Play MIDI notes
   - Stop recording

6. **Analyze**:
   - Look for `modal_attractors_engine_render` in call tree
   - Should be <1ms per buffer (512 samples @ 48kHz)
   - Hotspots: `audio_synth_render()`, `sinf()`

**Expected Performance** (M1 Mac, 48kHz, 512 frames):
- Single voice: ~0.1% CPU
- 8 voices: ~0.8% CPU
- 16 voices: ~1.5% CPU

---

## Advanced: Multi-Output Routing

Modal Attractors supports up to 8 output channels for per-voice routing.

### Enable in Logic Pro X

1. **Mixer → I/O**
2. Click output slot (currently "Stereo")
3. Select **Surround → 7.1**
4. Each voice can now route to separate channel

**Note**: Multi-output configuration requires updating `outputBusArray` in `ModalAttractorsAU.mm` to create 8 mono busses instead of 1 stereo bus.

---

## Distributing Your Plugin

### Code Signing for Distribution

1. **Get Apple Developer account** ($99/year)

2. **Create certificates**:
   - Developer ID Application (for non-App Store)
   - Or Mac App Store certificate

3. **Configure in Xcode**:
   - Target → Signing & Capabilities
   - Team: Select your team
   - Signing Certificate: Developer ID

4. **Enable Hardened Runtime**:
   - Capabilities → Hardened Runtime
   - Enable: Audio Input, Microphone (if needed)

5. **Notarize** (required for macOS 10.15+):
   ```bash
   # Archive
   xcodebuild archive -scheme ModalAttractors -archivePath ./ModalAttractors.xcarchive

   # Create .pkg installer
   productbuild --component ModalAttractors.appex /Library/Audio/Plug-Ins/Components ModalAttractors.pkg

   # Notarize
   xcrun notarytool submit ModalAttractors.pkg --apple-id "your@email.com" --wait

   # Staple
   xcrun stapler staple ModalAttractors.pkg
   ```

### Creating an Installer

Use **Packages.app** (free) or `productbuild`:

```bash
productbuild --component build/Release/ModalAttractors.appex \
             /Library/Audio/Plug-Ins/Components \
             --sign "Developer ID Installer: Your Name" \
             ModalAttractors-Installer.pkg
```

---

## Troubleshooting Build Errors

### "Module 'AudioToolbox' not found"

**Solution**: Add framework to build settings:
- Target → Build Phases → Link Binary With Libraries
- Click + → Add `AudioToolbox.framework`, `AVFoundation.framework`, `CoreAudio.framework`

### "Use of undeclared identifier 'ModalAttractorsEngine'"

**Solution**: Check header search paths include `src/dsp_core` and `src/au_wrapper`

### "Undefined symbols for architecture arm64"

**Solution**: Make sure all `.cpp` files are in **Compile Sources**:
- Target → Build Phases → Compile Sources
- Should include all .cpp and .mm files

### "Property list error: / Info.plist does not exist"

**Solution**: Set Info.plist path in Build Settings:
- Search "info.plist file"
- Set to: `ModalAttractors/Info.plist`

---

## Next Steps

1. ✅ Build succeeds
2. ✅ `auval` passes validation
3. ✅ Plugin loads in Logic Pro X
4. ✅ MIDI notes trigger synthesis
5. ✅ Parameters respond correctly
6. 🎵 **Make music!**

Then:
- Add UI (macOS view controller)
- Implement preset system
- Add more parameters (mode frequencies, damping, etc.)
- Create demo presets
- Record demos for distribution

---

## Resources

- **Apple Audio Unit Programming Guide**: https://developer.apple.com/documentation/audiounit
- **AUv3 Sample Code**: https://developer.apple.com/documentation/audiotoolbox/audio_unit_v3_plug-ins
- **auval documentation**: `man auval`
- **Test results**: See `TEST_RESULTS.md` for DSP validation

---

**Congratulations!** You're building a professional-grade Audio Unit plugin. 🎉
