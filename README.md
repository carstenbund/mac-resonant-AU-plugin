# Modal Attractors AU Plugin

**A macOS Audio Unit plugin for modal synthesis with network coupling**

Ported from ESP32 distributed resonator system to professional AU plugin for Logic Pro X and compatible DAWs.

---

## Project Status

**Current Phase:** Phase 2 - Audio Unit Plugin ✅ (Structure Complete, Testing Required)

This repository contains the Modal Attractors AU plugin implementation, from DSP core to Audio Unit wrapper, based on the ESP32 distributed resonator system.

### Completed Components

#### Phase 1: Core DSP ✅
- ✅ ESP32 DSP core ported to macOS (C)
  - `modal_node.c/.h` - Core modal oscillator
  - `audio_synth.c/.h` - Audio synthesis engine
- ✅ C++ DSP wrapper classes
  - `ModalVoice` - Voice management
  - `VoiceAllocator` - Polyphonic allocation
  - `TopologyEngine` - Network coupling
- ✅ CMake build system
- ✅ Standalone test application (validated Jan 7, 2026)

#### Phase 2: Audio Unit Plugin ✅ (Structure)
- ✅ AU plugin bundle structure
- ✅ Info.plist configuration
- ✅ Objective-C++ AU wrapper (`ModalAttractorsAU.mm`)
- ✅ MIDI event handling (note on/off)
- ✅ AU parameter system (3 core parameters)
- ✅ Build script for macOS
- ⏳ macOS testing and validation required

### In Progress

- ⏳ Phase 2: macOS testing and Logic Pro X validation
- ⏳ Phase 3: Voice coupling and topologies expansion
- ⏳ Phase 4: Preset system and GUI
- ⏳ Phase 5: Optimization and polish

---

## Architecture

```
au-plugin/
├── src/
│   ├── esp32_port/          # Ported ESP32 C code
│   │   ├── modal_node.c/.h  # Core modal oscillator
│   │   └── audio_synth.c/.h # Audio synthesis
│   ├── dsp_core/            # C++ DSP wrappers
│   │   ├── ModalVoice.cpp/.h
│   │   ├── VoiceAllocator.cpp/.h
│   │   └── TopologyEngine.cpp/.h
│   ├── au_wrapper/          # AU plugin interface
│   │   ├── ModalAttractorsAU.h
│   │   ├── ModalParameters.h
│   │   └── ModalAttractorsEngine.cpp
│   └── gui/                 # (Future) Cocoa GUI
├── Resources/               # Presets, Info.plist
├── Tests/                   # Test applications
│   └── test_modal_voice.cpp
├── CMakeLists.txt           # Build configuration
└── README.md                # This file
```

---

## Building

### Requirements

- **macOS 12.0+** (Monterey or later)
- **CMake 3.20+**
- **Xcode Command Line Tools** (for clang/clang++)
- **C++17 compiler**
- **Logic Pro X** (optional, for DAW testing)

### Phase 1: Standalone DSP Test

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
make

# Run standalone test
./test_modal_voice
```

This will generate a `test_output.wav` file demonstrating the modal oscillator.

### Phase 2: Audio Unit Plugin (macOS with Xcode)

**For macOS users:** See **[BUILD_MACOS.md](BUILD_MACOS.md)** for complete Xcode build instructions.

**Quick start:**
```bash
# Generate Xcode project
./build_xcode.sh xcode

# Open in Xcode
open build-xcode/ModalAttractors.xcodeproj

# Or build from command line
./build_xcode.sh build
```

**Important:** The AU plugin requires a proper Xcode project with AudioUnit frameworks. The `BUILD_MACOS.md` guide provides step-by-step instructions for creating the Xcode project, adding source files, linking frameworks, and testing in Logic Pro X.

The installed plugin will appear in Logic Pro X under: **AU Instruments → Carsten Bund → Modal Attractors**

### Build Options

```bash
# Enable SIMD optimizations (Accelerate framework)
cmake -DENABLE_SIMD=ON ..

# Build for specific architecture
cmake -DCMAKE_OSX_ARCHITECTURES="arm64" ..

# Build universal binary (Intel + Apple Silicon)
cmake -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ..
```

---

## Testing

### Phase 1: Standalone Test

The `test_modal_voice` application validates the core DSP port:

```bash
./test_modal_voice
```

**What it does:**
- Creates a single `ModalVoice` instance
- Configures 4 modal oscillators with harmonic relationships
- Triggers a note (MIDI A3, velocity 0.8)
- Renders 5 seconds of audio at 48kHz
- Outputs to `test_output.wav`

**Success criteria:**
- ✓ Output spectrum matches ESP32 reference
- ✓ No audible clicks or artifacts
- ✓ Stable oscillation with proper decay

### Analyzing Output

Use any audio analysis tool to inspect `test_output.wav`:

```bash
# macOS: Open in QuickTime Player
open test_output.wav

# Or use FFmpeg for spectral analysis
ffmpeg -i test_output.wav -lavfi showspectrumpic=s=1024x512 spectrum.png
```

---

## Technical Details

### Modal Synthesis

Each voice consists of **4 complex modes** evolving according to:

```
ȧₖ = (-γₖ + iωₖ)aₖ + uₖ(t)
```

Where:
- `aₖ` = complex amplitude of mode k
- `ωₖ` = angular frequency (rad/s)
- `γₖ` = damping coefficient
- `uₖ(t)` = excitation input

**Integration method:** Exact exponential integration for numerical stability

### Audio Rendering

- **Sample rates:** 44.1, 48, 88.2, 96 kHz (configurable)
- **Polyphony:** Up to 32 voices (default 16)
- **Control rate:** 500 Hz (2ms timestep)
- **Latency:** Depends on AU host buffer size

### Key Features

1. **Physics-based synthesis** - Modal resonator model from acoustics research
2. **Network coupling** - Voices interact through various topologies:
   - Ring/Chain
   - Small-world (Watts-Strogatz)
   - Clustered/Modular
   - Hub-and-spoke
   - Random
   - Complete graph
3. **Emergent behavior** - Complex timbres from simple interactions
4. **Efficient C core** - Ported from real-time ESP32 firmware

---

## Development Roadmap

### Phase 1: Core DSP Port ✅ (Current)

**Status:** Complete
**Deliverables:**
- Ported ESP32 modal_node and audio_synth
- C++ wrappers (ModalVoice, VoiceAllocator, TopologyEngine)
- Standalone test application

**Validation:**
- [ ] Output matches ESP32 spectrum (>95% correlation)
- [ ] Works at 44.1, 48, 96 kHz
- [ ] CPU usage <1% per voice @ 48kHz
- [ ] No memory leaks

### Phase 2: Audio Unit Shell 🔄 (Next)

**Goal:** Basic AU plugin loads in Logic Pro X
**Tasks:**
- Xcode AU project setup
- AUInstrumentBase subclass implementation
- MIDI event handling
- Voice allocation integration
- AU parameter definitions
- auval validation

### Phase 3: Voice Coupling & Topologies

**Goal:** Multi-voice coupling with network topologies
**Tasks:**
- Implement coupling matrix system
- Port topology generators from Python
- Topology parameter exposure
- CPU optimization (sparse matrices)

### Phase 4: Preset System & GUI

**Goal:** Professional UI and preset library
**Tasks:**
- AU preset serialization
- Factory preset library (20+ presets)
- Cocoa GUI (topology visualizer, parameter controls)
- Preset browser

### Phase 5: Optimization & Release

**Goal:** Production-ready plugin
**Tasks:**
- SIMD optimization (Accelerate framework)
- Apple Silicon native build
- Code signing & notarization
- Beta testing
- User manual

---

## API Reference

### C Core (ESP32 Port)

#### `modal_node_t`

Core modal oscillator state with 4 complex modes.

```c
void modal_node_init(modal_node_t* node, uint8_t node_id, node_personality_t personality);
void modal_node_set_mode(modal_node_t* node, uint8_t mode_idx, float omega, float gamma, float weight);
void modal_node_step(modal_node_t* node);  // Update dynamics
void modal_node_apply_poke(modal_node_t* node, const poke_event_t* poke);
```

#### `audio_synth_t`

Audio synthesis from modal state.

```c
void audio_synth_init(audio_synth_t* synth, const modal_node_t* node, float sample_rate);
void audio_synth_render(audio_synth_t* synth, float* outL, float* outR, uint32_t num_frames);
```

### C++ DSP Wrapper

#### `ModalVoice`

Per-voice modal oscillator with MIDI support.

```cpp
class ModalVoice {
public:
    void initialize(float sample_rate);
    void noteOn(uint8_t midi_note, float velocity);
    void noteOff();
    void renderAudio(float* outL, float* outR, uint32_t num_frames);
    void setMode(uint8_t mode_idx, float freq_hz, float damping, float weight);
};
```

#### `VoiceAllocator`

Polyphonic voice management.

```cpp
class VoiceAllocator {
public:
    VoiceAllocator(uint32_t max_polyphony);
    ModalVoice* noteOn(uint8_t midi_note, uint8_t velocity);
    void noteOff(uint8_t midi_note);
    void renderAudio(float* outL, float* outR, uint32_t num_frames);
};
```

#### `TopologyEngine`

Network coupling topology generator.

```cpp
class TopologyEngine {
public:
    void generateTopology(TopologyType type, float coupling_strength);
    void updateCoupling(ModalVoice** voices, uint32_t num_voices);
};
```

---

## Contributing

This is an experimental research project. Contributions welcome!

**Areas needing help:**
- Phase 2: Audio Unit implementation (requires AU SDK experience)
- Optimization: SIMD/Accelerate framework integration
- GUI: Cocoa/AppKit interface design
- Testing: Cross-platform validation (Intel vs Apple Silicon)
- Documentation: User manual, tutorial videos

---

## References

- **Proposal:** [AU Plugin Porting Proposal](../docs/AU_PLUGIN_PORTING_PROPOSAL.md)
- **ESP32 Firmware:** `../esp32/firmware/`
- **Python Reference:** `../src/network.py`
- **Apple AU Guide:** [Audio Unit Programming Guide](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/)

---

## License

See main project LICENSE file.

---

## Contact

For technical questions, see the main project repository:
- GitHub: https://github.com/carstenbund/audio-sim

**Phase 1 Status:** Core DSP port complete ✅
**Next Milestone:** AU plugin shell (Phase 2)
