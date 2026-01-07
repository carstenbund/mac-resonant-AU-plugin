# Modal Attractors - Quick Start Guide

Get up and running with the Modal Attractors AU plugin in minutes.

---

## 🎯 What is Modal Attractors?

A **macOS Audio Unit plugin** featuring:
- **Modal synthesis** with 4 coupled resonators per voice
- **Network coupling** between voices (7 topology types)
- **Polyphonic** (up to 32 voices)
- **Real-time MIDI** control
- **Clean audio** (extensively tested, <0.01 max discontinuity)

---

## 📦 What You Get

- **DSP Core**: Validated C++ modal synthesis engine
- **Test Suite**: 7 comprehensive tests with audio analysis
- **AU Plugin**: Full AUv3 implementation for Logic Pro X
- **Documentation**: Complete build and testing guides

---

## 🚀 Three-Step Workflow

### Step 1: Test the DSP Core (5 minutes)

Verify the synthesis engine works before building the plugin.

```bash
# Generate Xcode project
./generate_xcode_project.sh

# Open in Xcode
open build_xcode/ModalAttractors.xcodeproj

# Select scheme: "test_suite"
# Press Cmd+R to run

# ✅ Expected: All 7 tests pass, 10 WAV files generated
```

**Read**: [XCODE_TESTING.md](XCODE_TESTING.md) for detailed testing guide.

**Results**: See [TEST_RESULTS.md](TEST_RESULTS.md) - all tests should pass with zero discontinuities.

---

### Step 2: Build the AU Plugin (30 minutes)

Create the Audio Unit plugin bundle in Xcode.

**Follow the step-by-step guide**: [BUILD_AU_PLUGIN.md](BUILD_AU_PLUGIN.md)

**Summary**:
1. Create new Xcode project (**Audio Unit Extension**)
2. Add source files from `src/au_wrapper/`, `src/dsp_core/`, `src/esp32_port/`
3. Replace `Info.plist` with provided one
4. Configure build settings (C++17, header paths, frameworks)
5. Build (Cmd+B)
6. Install to `~/Library/Audio/Plug-Ins/Components/`

**Validate**:
```bash
auval -v aumi mdla Cbnd
# Should output: * * PASS
```

---

### Step 3: Test in Logic Pro X (5 minutes)

Load and play the plugin in your DAW.

1. **Restart Logic Pro X** (to discover new plugin)
2. **Create Software Instrument track**
3. **Load plugin**: AU Instruments → Carsten Bund → Modal Attractors
4. **Play MIDI notes** → Hear modal synthesis! 🎵

**Parameters to try**:
- **Master Gain**: Overall volume
- **Coupling Strength**: How much voices interact (0 = independent, 1 = fully coupled)
- **Topology**: Network structure (Ring, Small World, Complete, etc.)

---

## 📂 Project Structure

```
mac-resonant-AU-plugin/
├── src/
│   ├── dsp_core/              # Core DSP engine (C++)
│   │   ├── ModalVoice.cpp     # Single voice modal synthesis
│   │   ├── VoiceAllocator.cpp # Polyphonic voice management
│   │   └── TopologyEngine.cpp # Network coupling
│   │
│   ├── esp32_port/            # Low-level synthesis (C)
│   │   ├── modal_node.c       # Modal dynamics (500 Hz)
│   │   └── audio_synth.c      # Audio rendering (48 kHz)
│   │
│   └── au_wrapper/            # Audio Unit wrapper (Obj-C++)
│       ├── ModalAttractorsAU.mm        # Main AU class (AUv3)
│       ├── ModalAttractorsDSPKernel.mm # Real-time DSP kernel
│       ├── ModalAttractorsEngine.cpp   # C++ engine interface
│       ├── ModalParameters.h           # Parameter definitions
│       └── Info.plist                  # AU bundle metadata
│
├── Tests/
│   ├── test_modal_voice.cpp   # Original single-voice test
│   └── test_suite.cpp         # Comprehensive 7-test suite
│
├── Documentation/
│   ├── QUICKSTART.md          # ← You are here
│   ├── BUILD_AU_PLUGIN.md     # Complete AU build guide
│   ├── XCODE_TESTING.md       # Testing in Xcode guide
│   ├── TEST_PLAN.md           # Test methodology
│   ├── TEST_RESULTS.md        # Validation results
│   ├── BUILD_MACOS.md         # macOS build overview
│   └── PYTHON_COMPARISON.md   # Python vs C++ comparison
│
└── Scripts/
    ├── generate_xcode_project.sh  # Create test Xcode project
    ├── run_tests.sh               # Automated test runner
    └── check_audio.py             # WAV file analyzer
```

---

## 🎛️ Technical Highlights

### Modal Synthesis Architecture

Each voice has **4 coupled resonators** (modes):
- **Mode 0**: Fundamental (MIDI pitch)
- **Mode 1**: Detuned fundamental (creates beating)
- **Mode 2**: First harmonic (octave)
- **Mode 3**: Second harmonic (octave + fifth)

**Dynamics** (per mode):
```
da_k/dt = (-γ_k + iω_k) * a_k + coupling + excitation
```

- `ω_k`: Resonant frequency
- `γ_k`: Damping coefficient
- `a_k`: Complex amplitude (magnitude = loudness)

**Audio synthesis**:
```
output = Σ |a_k| * sin(2π * f_k * t)
```

### Network Coupling

**7 Topology Types**:
1. **Ring**: Each voice connects to neighbors (cyclic)
2. **Small World**: Ring + random long-range connections
3. **Clustered**: Groups of tightly coupled voices
4. **Hub-Spoke**: Central hub connected to all voices
5. **Random**: Probabilistic connections
6. **Complete**: Every voice connected to all others
7. **None**: Independent voices (no coupling)

**Coupling equation**:
```
coupling_i = κ * Σ (a_j - a_i)  // where j are neighbors
```

### Real-Time Architecture

- **Control rate**: 500 Hz (2 ms updates for modal dynamics)
- **Audio rate**: 48 kHz (audio rendering)
- **Buffer size**: 512 samples (~10.6 ms latency)
- **Polyphony**: Up to 32 voices (configurable)

---

## 🧪 Test Suite Overview

**7 Test Cases** validate all functionality:

| Test | What It Tests | Duration | Channels |
|------|---------------|----------|----------|
| **1. Single Voice Decay** | Basic modal synthesis | 5.0s | Stereo |
| **2. Multiple Excitations** | Sequential note triggers | 6.0s | Stereo |
| **3. Polyphony** | 4-voice chord | 5.0s | Stereo |
| **4. Topology (x4)** | Network coupling types | 4.0s each | Stereo |
| **5. MIDI Scale** | Pitch tracking | 8.0s | Stereo |
| **6. Multichannel** | 8-channel routing | 5.0s | **8 channels** |
| **7. Parameter Sweep** | Damping control | 10.0s | Stereo |

**Success Criteria**:
- ✅ Zero phase discontinuities (max jump <0.02)
- ✅ No clipping
- ✅ Smooth exponential decay (>85%)
- ✅ Correct MIDI pitch tracking

**Current Results**: **ALL TESTS PASSING** ✨
- Best test: 0.0008 max sample jump (Test 5 - MIDI Scale)
- Average: 0.005-0.015 max jumps (inaudible)

---

## 🔧 System Requirements

**Development**:
- macOS 12.0+ (Monterey or later)
- Xcode 14+
- 4 GB RAM
- Apple Silicon (M1/M2/M3) or Intel Mac

**Runtime** (for end users):
- macOS 10.15+ (Catalina or later)
- DAW with AU support (Logic Pro X, GarageBand, Live, etc.)
- 200 MB disk space

---

## 📝 Key Files Reference

### Testing
- **`generate_xcode_project.sh`** - One-command Xcode test project generation
- **`run_tests.sh`** - Automated test runner with audio analysis
- **`check_audio.py`** - WAV file quality analyzer (no dependencies)

### Building AU Plugin
- **`src/au_wrapper/Info.plist`** - Bundle configuration (critical!)
- **`src/au_wrapper/ModalAttractorsAU.mm`** - Main AU implementation
- **`BUILD_AU_PLUGIN.md`** - Complete build instructions

### Documentation
- **`TEST_RESULTS.md`** - Detailed test validation report
- **`XCODE_TESTING.md`** - Testing guide (300+ lines)
- **`BUILD_MACOS.md`** - macOS build overview

---

## 🎵 Making Your First Sound

Once the plugin is loaded in Logic Pro X:

1. **Enable MIDI** (click R on track to record-enable)
2. **Play a note** (C3 / MIDI 60)
3. **Hear the decay** - Resonant modal synthesis!

**Try these experiments**:

### Experiment 1: Topology Comparison
- Play a chord (C-E-G)
- **Topology = None** → Hear 3 independent decays
- **Topology = Complete** → Hear energy sharing, slower decay
- **Topology = Ring** → Hear coupling patterns

### Experiment 2: Coupling Strength
- Play held note
- **Coupling = 0.0** → Pure resonator
- **Coupling = 0.5** → Moderate interaction
- **Coupling = 1.0** → Strong network effects

### Experiment 3: Polyphonic Patterns
- Play rapid arpeggios
- Notice voice stealing (when >16 notes)
- Try different topologies while playing

---

## 🐛 Troubleshooting

### Tests don't run in Xcode

**Solution**: Set working directory in scheme settings:
- Product → Scheme → Edit Scheme
- Run → Options → Use custom working directory: `$PROJECT_DIR/build_xcode`

### Plugin doesn't appear in Logic

**Solution**:
1. Check installation: `ls ~/Library/Audio/Plug-Ins/Components/ModalAttractors.appex`
2. Validate: `auval -v aumi mdla Cbnd`
3. Restart Logic Pro X
4. Check Console.app for errors

### No audio output from plugin

**Solution**:
1. Make sure MIDI is triggering (track record-enabled)
2. Check Master Gain parameter isn't 0
3. Verify tests passed (confirms DSP works)
4. Attach Xcode debugger to Logic and check `render()` is called

### Clicking/popping in audio

**Solution**: This was fixed! If you hear clicks:
1. Verify you're using latest code (phase fix applied)
2. Run Test 1 - should show max jump <0.01
3. Check buffer size in Logic isn't too small (<64)

---

## 🎓 Learning More

### Understanding Modal Synthesis

Modal synthesis models physical resonances. Each mode represents a vibration frequency:
- **Guitar string**: Fundamental + harmonics
- **Bell**: Inharmonic modes (non-integer ratios)
- **Modal Attractors**: Coupled nonlinear oscillators

**Further reading**:
- Julius O. Smith III: "Physical Audio Signal Processing"
- Fletcher & Rossing: "The Physics of Musical Instruments"

### Network Science & Audio

The topology engine applies graph theory to synthesis:
- **Small-world networks**: Watts & Strogatz (1998)
- **Scale-free networks**: Barabási & Albert (1999)
- **Coupled oscillators**: Kuramoto model

---

## 🚀 What's Next?

### Immediate (You)
1. ✅ Test DSP core
2. ✅ Build AU plugin
3. ✅ Load in Logic Pro X
4. 🎵 Make music!

### Future Enhancements
- [ ] Add UI (macOS view controller with knobs)
- [ ] Implement preset system
- [ ] Add more parameters (per-mode frequency/damping control)
- [ ] Create factory presets (bell, string, gong, etc.)
- [ ] Add modulation (LFO, envelope follower)
- [ ] Support more topologies
- [ ] Add trigger types (noise, phase kick - from Python version)
- [ ] Optimize performance (SIMD, multithreading)

---

## 📞 Support & Contribution

**Issues**: See troubleshooting sections in:
- **BUILD_AU_PLUGIN.md** - Build issues
- **XCODE_TESTING.md** - Testing issues
- **TEST_PLAN.md** - Debugging guide

**Architecture questions**: See:
- **PYTHON_COMPARISON.md** - Python vs C++ synthesis comparison
- **AUDIO_SYNTHESIS_COMPARISON.md** - Detailed audio architecture

---

## ✅ Checklist

Use this to track your progress:

- [ ] Clone repository
- [ ] Install Xcode + Command Line Tools
- [ ] Run `./generate_xcode_project.sh`
- [ ] Open `build_xcode/ModalAttractors.xcodeproj`
- [ ] Run test_suite (all 7 tests pass)
- [ ] Analyze WAV files (verify clean audio)
- [ ] Create AU plugin Xcode project
- [ ] Add source files to target
- [ ] Configure build settings
- [ ] Build plugin (Cmd+B succeeds)
- [ ] Install to `~/Library/Audio/Plug-Ins/Components/`
- [ ] Validate with `auval`
- [ ] Restart Logic Pro X
- [ ] Load Modal Attractors plugin
- [ ] Play MIDI notes (hear synthesis!)
- [ ] Experiment with parameters
- [ ] 🎉 **You're done!**

---

**Enjoy your Modal Attractors synth!** 🎹✨
