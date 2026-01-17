# Daisy Seed Porting Proposal

**Porting ESP32 Modal Resonator DSP to Daisy Seed Platform**

**Version:** 1.0
**Date:** 2026-01-17
**Status:** Proposal
**Target Platform:** Electrosmith Daisy Seed (STM32H750)

---

## Executive Summary

This document proposes a port of the ESP32-based distributed modal resonator network to the Daisy Seed platform. The Daisy Seed offers significant advantages including integrated high-quality audio codec, more powerful ARM Cortex-M7 processor, and USB host capability for MIDI input. While the core DSP code is highly portable, the audio interface and networking layers require platform-specific adaptations.

**Key Findings:**
- Core DSP code (modal_node.c) is ~95% portable with minimal changes
- Audio synthesis requires adapter layer for Daisy's audio codec
- USB MIDI replaces ESP-NOW for external control
- Inter-node networking requires alternative approach (see Options section)
- Build system transitions from ESP-IDF to Arduino/libDaisy

---

## 1. Platform Comparison

### ESP32-S3 (Current)

| Component | Specification |
|-----------|--------------|
| **CPU** | Xtensa LX7 dual-core @ 240 MHz |
| **RAM** | 512 KB SRAM |
| **Flash** | 4-8 MB (external) |
| **Audio** | I2S to external DAC (PCM5102A) |
| **Network** | ESP-NOW (proprietary 2.4GHz) |
| **MIDI** | Via UART/GPIO |
| **FPU** | Single precision |
| **ADC** | 12-bit, 2 SPS |

### Daisy Seed (Target)

| Component | Specification |
|-----------|--------------|
| **CPU** | ARM Cortex-M7 @ 480 MHz |
| **RAM** | 512 KB SRAM + 64 MB SDRAM |
| **Flash** | 16 KB internal + 8 MB QSPI |
| **Audio** | WM8731 codec (24-bit, 96kHz) |
| **Network** | None (USB host available) |
| **MIDI** | USB MIDI host/device |
| **FPU** | Double precision hardware |
| **ADC** | 16-bit SAR ADC |

### Advantages of Daisy Seed

1. **Higher Performance**: 2x clock speed, hardware double-precision FPU
2. **Superior Audio**: Integrated codec with 24-bit resolution vs external I2S DAC
3. **More Memory**: 64 MB SDRAM for sample buffers, large wavetables
4. **Better MIDI**: Native USB MIDI host (plug-and-play with controllers)
5. **Robust Build**: Integrated audio eliminates external wiring issues
6. **Better Tooling**: libDaisy provides mature audio framework

### Challenges

1. **No ESP-NOW**: Must find alternative for inter-node networking
2. **Different RTOS**: FreeRTOS vs custom Daisy scheduler (or bare Arduino)
3. **Audio API Changes**: ESP32 I2S → Daisy AudioCallback pattern
4. **Pin Mapping**: Different GPIO layout

---

## 2. Architecture Overview

### What Stays the Same (Portable)

#### Core DSP Layer (~95% portable)
```c
esp32/firmware/main/core/
├── modal_node.h          // ✅ No changes needed
├── modal_node.c          // ✅ 1-2 minor tweaks (RNG, complex.h)
```

**Changes required:**
- Replace `esp_random()` with `rand()` or ARM RNG
- Verify `complex.h` compatibility (ARM CMSIS may require adaptation)

#### Audio Synthesis Logic (~80% portable)
```c
esp32/firmware/main/audio/
├── audio_synth.h         // ⚠️ Minimal changes (remove I2S references)
├── audio_synth.c         // ⚠️ Keep synthesis, replace I2S calls
```

**Changes required:**
- Extract pure synthesis code (sine generation, mode mixing)
- Replace I2S write with Daisy AudioCallback pattern
- Adapt buffer sizes to Daisy's callback model (typically 48-256 samples)

### What Changes (Platform-Specific)

#### Audio Driver Layer (Complete Rewrite)
```c
// ESP32 approach (push model)
void audio_task(void* params) {
    while(1) {
        int16_t* buf = audio_synth_generate_buffer();
        i2s_write(I2S_NUM_0, buf, size);
    }
}

// Daisy approach (pull/callback model)
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    audio_synth_generate_buffer_interleaved(out[0], out[1], size);
}
```

**Key Differences:**
- ESP32: FreeRTOS task pushes buffers to I2S peripheral
- Daisy: DMA interrupt pulls buffers via callback (audio-rate, high priority)

#### Network Layer

**ESP32:**
```c
esp32/firmware/main/network/
├── esp_now_manager.c     // ❌ Not portable (ESP-NOW specific)
├── protocol.c            // ✅ Protocol definitions portable
```

**Daisy Options:**

1. **Option A: USB MIDI Only (Simplest)**
   - Single standalone node
   - Controlled via USB MIDI from DAW/hardware
   - No inter-node networking

2. **Option B: MIDI TRS Chain**
   - MIDI OUT → next node's MIDI IN
   - Poke messages encoded as SysEx
   - Limited to ~16 nodes (MIDI latency)

3. **Option C: I2C/SPI Bus**
   - Hardware bus for local node cluster (2-8 nodes)
   - One master, multiple slaves
   - Requires custom PCB or breadboard

4. **Option D: ESP32 WiFi Bridge**
   - Daisy Seed nodes use UART/SPI to ESP32 modules
   - ESP32s handle ESP-NOW networking
   - Daisy focuses on audio DSP

**Recommendation:** Start with **Option A** for single-node prototype, then **Option D** for distributed system.

---

## 3. Detailed Porting Plan

### Phase 1: Single-Node Standalone (1-2 weeks)

**Goal:** Get modal resonator working on Daisy Seed with USB MIDI control.

#### Step 1.1: Project Setup
```
daisy-seed/
├── src/
│   ├── main.cpp                    // Arduino entry point
│   ├── core/
│   │   ├── modal_node.h           // Copy from ESP32
│   │   ├── modal_node.c           // Copy + minor edits
│   │   └── modal_node_config.h    // Daisy-specific config
│   ├── audio/
│   │   ├── audio_synth.h          // Adapted from ESP32
│   │   ├── audio_synth.cpp        // Rewritten for Daisy
│   │   └── daisy_audio_adapter.cpp
│   └── midi/
│       └── usb_midi_handler.cpp   // New: USB MIDI input
├── lib/
│   └── DaisySP/                   // Daisy DSP library (optional)
└── platformio.ini                 // Or Arduino IDE config
```

#### Step 1.2: Core DSP Adaptation
```c
// modal_node.c changes

// Replace ESP32 random with standard C
#include <stdlib.h>

float random_phase(void) {
    return ((float)rand() / RAND_MAX) * 2.0f * M_PI;
}

// Complex math: use ARM CMSIS if needed
#include <complex.h>  // Standard C99 complex
// OR
#include "arm_math.h"  // CMSIS DSP (faster)
```

#### Step 1.3: Audio Synthesis Adapter
```cpp
// daisy_audio_adapter.cpp

#include "daisy_seed.h"
#include "audio_synth.h"

using namespace daisy;

DaisySeed hw;
audio_synth_t synth;
modal_node_t node;

// Daisy audio callback (called at 48kHz, typically 48-sample blocks)
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    // Generate stereo output from 4 modes
    for (size_t i = 0; i < size; i++) {
        float left = 0.0f;
        float right = 0.0f;

        // Mix modes to stereo (example: modes 0,1 → L, modes 2,3 → R)
        left = audio_synth_get_mode_sample(&synth, 0, i) +
               audio_synth_get_mode_sample(&synth, 1, i);
        right = audio_synth_get_mode_sample(&synth, 2, i) +
                audio_synth_get_mode_sample(&synth, 3, i);

        out[0][i] = left * 0.5f;   // Headroom
        out[1][i] = right * 0.5f;
    }

    // Advance synthesis state (phase accumulators, envelopes)
    audio_synth_advance(&synth, size);
}

void setup() {
    // Initialize Daisy Seed hardware
    hw.Init();
    hw.SetAudioBlockSize(48); // 1ms blocks @ 48kHz
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    // Initialize modal node
    modal_node_init(&node, 0, PERSONALITY_RESONATOR);
    modal_node_set_mode(&node, 0, freq_to_omega(440.0f), 0.5f, 1.0f);
    modal_node_set_mode(&node, 1, freq_to_omega(442.0f), 0.6f, 0.8f);
    modal_node_set_mode(&node, 2, freq_to_omega(880.0f), 1.0f, 0.3f);
    modal_node_set_mode(&node, 3, freq_to_omega(55.0f), 0.1f, 0.5f);

    // Initialize audio synthesis
    audio_synth_init(&synth, &node);

    // Start audio
    hw.StartAudio(AudioCallback);
}

void loop() {
    // Control rate tasks (500 Hz)
    static uint32_t last_control = 0;
    if (System::GetNow() - last_control > 2) {  // 2ms
        modal_node_step(&node);
        last_control = System::GetNow();
    }

    // Handle MIDI, UI, etc.
    System::Delay(1);
}
```

#### Step 1.4: USB MIDI Integration
```cpp
// usb_midi_handler.cpp

#include "daisysp.h"
#include "modal_node.h"

extern modal_node_t node;

void ProcessMIDI() {
    hw.ProcessDigitalControls();

    auto midi_event = hw.midi.PopEvent();

    switch(midi_event.type) {
        case NoteOn: {
            // Trigger poke on note-on
            poke_event_t poke;
            poke.source_node_id = 0;
            poke.strength = midi_event.velocity / 127.0f;
            poke.phase_hint = -1.0f;  // Random phase

            // Mode weights based on note velocity
            poke.mode_weights[0] = 1.0f;
            poke.mode_weights[1] = 0.8f;
            poke.mode_weights[2] = 0.5f;
            poke.mode_weights[3] = 0.3f;

            modal_node_apply_poke(&node, &poke);
            break;
        }

        case ControlChange: {
            // Map CC to mode parameters
            if (midi_event.control_number == 1) {  // Mod wheel
                float gamma = midi_event.value / 127.0f * 2.0f;
                node.modes[0].params.gamma = gamma;
            }
            break;
        }
    }
}
```

**Deliverables:**
- [ ] Single Daisy Seed running modal resonator
- [ ] USB MIDI control (note-on triggers poke)
- [ ] 4-channel audio output (mapped to stereo)
- [ ] Stable 48kHz audio without clicks/dropouts

---

### Phase 2: Optimization & Features (1 week)

#### Step 2.1: Performance Tuning
- Profile CPU usage (Daisy has built-in profiler)
- Optimize modal integration (use CMSIS DSP functions)
- Implement SIMD optimizations if needed (ARM NEON)

#### Step 2.2: Enhanced Audio Features
```cpp
// Multi-channel output options

// Option A: Quad output (if using Daisy Patch or custom board)
out[0][i] = mode_0_sample;
out[1][i] = mode_1_sample;
out[2][i] = mode_2_sample;
out[3][i] = mode_3_sample;

// Option B: Stereo with panning
float pan = cargf(node.modes[0].a) / M_PI;  // -1 to 1
left = mix * (1.0f - pan) * 0.5f;
right = mix * (1.0f + pan) * 0.5f;

// Option C: Spatial audio (binaural, Ambisonics)
// Use DaisySP Oscillator/ReverbSc for spatial effects
```

#### Step 2.3: CV Input (Optional)
```cpp
// Daisy Seed has 8 ADC inputs
// Use for real-time parameter control

void ProcessCV() {
    hw.ProcessAnalogControls();

    float cv1 = hw.GetAdcValue(0);  // 0-1
    float cv2 = hw.GetAdcValue(1);

    // Map CV to modal parameters
    node.modes[0].params.omega = freq_to_omega(110.0f + cv1 * 880.0f);
    node.coupling_strength = cv2;
}
```

**Deliverables:**
- [ ] Optimized DSP (target <50% CPU @ 48kHz)
- [ ] CV input mapping
- [ ] LED status indicators
- [ ] Preset system (save/load from flash)

---

### Phase 3: Multi-Node Networking (2-3 weeks)

**Recommended Approach: Option D (ESP32 WiFi Bridge)**

#### Architecture
```
[Daisy Seed 1] ←UART→ [ESP32-C3] ←ESP-NOW→ [ESP32-C3] ←UART→ [Daisy Seed 2]
      ↓                                                               ↓
  Audio Codec                                                   Audio Codec
```

#### Step 3.1: ESP32 Bridge Firmware
```c
// Simplified ESP32 code (just networking bridge)

void esp32_bridge_main() {
    // Initialize ESP-NOW
    esp_now_init();

    // Initialize UART to Daisy
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        // ...
    };
    uart_param_config(UART_NUM_1, &uart_config);

    while (1) {
        // Forward UART → ESP-NOW
        if (uart_has_data()) {
            uint8_t msg[256];
            int len = uart_read(msg, sizeof(msg));
            esp_now_broadcast(msg, len);
        }

        // Forward ESP-NOW → UART
        if (esp_now_has_message()) {
            uint8_t msg[256];
            int len = esp_now_receive(msg, sizeof(msg));
            uart_write(msg, len);
        }
    }
}
```

#### Step 3.2: Daisy UART Protocol
```cpp
// Daisy side: Serialize poke messages via UART

void SendPokeToNetwork(const poke_event_t* poke) {
    uint8_t msg[32];
    msg[0] = MSG_POKE;
    msg[1] = poke->source_node_id;
    memcpy(&msg[2], &poke->strength, sizeof(float));
    memcpy(&msg[6], poke->mode_weights, sizeof(poke->mode_weights));

    hw.uart.PollTx(msg, 32);
}

void ReceivePokesFromNetwork() {
    if (hw.uart.Readable()) {
        uint8_t msg[32];
        hw.uart.PollRx(msg, 32, 10);

        if (msg[0] == MSG_POKE) {
            poke_event_t poke;
            poke.source_node_id = msg[1];
            memcpy(&poke.strength, &msg[2], sizeof(float));
            memcpy(poke.mode_weights, &msg[6], sizeof(poke.mode_weights));

            modal_node_apply_poke(&node, &poke);
        }
    }
}
```

**Alternative: MIDI SysEx Networking (No ESP32 needed)**
```cpp
// Use MIDI chain for poke distribution
// Node 0 → MIDI OUT → Node 1 → MIDI OUT → Node 2 ...

void SendPokeMIDI(const poke_event_t* poke) {
    // SysEx format: F0 7D <node_id> <poke_data> F7
    uint8_t sysex[16];
    sysex[0] = 0xF0;  // SysEx start
    sysex[1] = 0x7D;  // Manufacturer ID (non-commercial)
    sysex[2] = poke->source_node_id;
    sysex[3] = (uint8_t)(poke->strength * 127.0f);
    // ... encode mode_weights
    sysex[15] = 0xF7;  // SysEx end

    hw.midi.SendMessage(sysex, 16);
}
```

**Deliverables:**
- [ ] 2-node prototype with ESP32 bridges
- [ ] Poke message routing
- [ ] Latency measurement (<10ms node-to-node)
- [ ] Alternative: MIDI chain demo

---

## 4. Memory and Performance Estimates

### RAM Usage (Single Node)

| Component | ESP32 | Daisy Seed |
|-----------|-------|------------|
| Modal state | ~512 bytes | ~512 bytes |
| Audio buffers | 3.84 KB (4ch×480×16bit×4buf) | 1.92 KB (stereo×48×32bit×2) |
| Network queue | 2 KB | 0 (or UART buf 256B) |
| FreeRTOS stack | ~16 KB | 0 (Arduino loop) |
| **Total** | **~22 KB** | **~2.7 KB** |

**Daisy Advantage:** 64 MB SDRAM available for wavetables, long delays, etc.

### CPU Usage Estimates (48kHz, Single Node)

| Task | ESP32 (240 MHz) | Daisy (480 MHz) |
|------|----------------|-----------------|
| Modal integration (500 Hz) | ~5% | ~2% |
| Audio synthesis (48kHz) | ~25% | ~10% |
| MIDI/Network | ~5% | ~2% |
| **Total** | **~35%** | **~14%** |

**Headroom:** Daisy Seed has ~85% CPU available for effects, larger mode counts, etc.

---

## 5. Build System

### Option A: Arduino IDE
```ini
# platformio.ini

[env:daisy_seed]
platform = ststm32
board = daisy_seed
framework = arduino
lib_deps =
    electro-smith/DaisySP@^1.0.0
    electro-smith/libDaisy@^7.0.0
build_flags =
    -D AUDIO_BLOCK_SIZE=48
    -D SAMPLE_RATE=48000
    -O2
    -mfpu=fpv5-d16
    -mfloat-abi=hard
```

### Option B: libDaisy (Pure C++)
```makefile
# Makefile

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

# Core sources
C_SOURCES = src/core/modal_node.c
CPP_SOURCES = src/main.cpp src/audio/audio_synth.cpp

# Compiler flags
OPT = -O2
CFLAGS += -mfpu=fpv5-d16 -mfloat-abi=hard

include $(LIBDAISY_DIR)/Makefile
```

**Recommendation:** Use **PlatformIO** with Arduino framework for easy dependency management.

---

## 6. Testing Plan

### Unit Tests
```cpp
// Test modal integration accuracy
void test_modal_exponential_decay() {
    modal_node_t node;
    modal_node_init(&node, 0, PERSONALITY_RESONATOR);
    modal_node_set_mode(&node, 0, freq_to_omega(440.0f), 1.0f, 1.0f);

    // Initial energy
    node.modes[0].a = 1.0f + 0.0if;

    // Simulate 1 second
    for (int i = 0; i < CONTROL_RATE_HZ; i++) {
        modal_node_step(&node);
    }

    // Should decay to ~0.368 (e^-1)
    float final_amp = cabsf(node.modes[0].a);
    assert(fabs(final_amp - 0.368f) < 0.01f);
}
```

### Audio Quality Tests
- [ ] THD+N measurement (should be <1% @ 1kHz)
- [ ] Frequency response (20Hz-20kHz ±1dB)
- [ ] Click/pop detection on poke events
- [ ] CPU load under sustained poke barrage

### Integration Tests
- [ ] USB MIDI latency (<5ms note-on to audio)
- [ ] Network latency (if using ESP32 bridge)
- [ ] Multi-node phase coherence measurement

---

## 7. Risks and Mitigations

### Risk 1: Complex Number Support
**Issue:** ARM toolchain may not have full C99 complex support.

**Mitigation:**
- Use CMSIS DSP library (`arm_cmplx_mag_f32`, etc.)
- Or implement simple complex struct:
```c
typedef struct { float re; float im; } complex_f32;
```

### Risk 2: Audio Buffer Size Mismatch
**Issue:** ESP32 uses 480-sample buffers (10ms). Daisy typically uses 48-256 samples.

**Mitigation:**
- Refactor `audio_synth_generate_buffer()` to be block-size agnostic
- Use ring buffer if needed

### Risk 3: Timing Jitter
**Issue:** Arduino `loop()` is not hard real-time.

**Mitigation:**
- Keep control loop lightweight (<1ms)
- Use hardware timer interrupt if needed
- Audio callback is already DMA-driven (hard real-time)

### Risk 4: Network Latency (ESP32 Bridge)
**Issue:** UART + ESP-NOW adds latency.

**Mitigation:**
- Use 921600 baud UART (vs 115200)
- Minimize message size (<32 bytes)
- Measure end-to-end latency (target <10ms)

---

## 8. Cost Analysis

### Single Node Cost

| Component | ESP32 Solution | Daisy Seed Solution |
|-----------|----------------|---------------------|
| MCU board | ESP32-S3 $8 | Daisy Seed $30 |
| Audio DAC | PCM5102A $2 | Integrated (included) |
| MIDI interface | UART circuit $1 | USB cable $5 |
| Enclosure | $5 | $5 |
| **Total** | **$16** | **$40** |

### 16-Node Network Cost

| Component | ESP32 Solution | Daisy Seed + ESP32 Bridge |
|-----------|----------------|--------------------------|
| Audio nodes | 16 × ESP32 + DAC = $160 | 16 × Daisy Seed = $480 |
| Network | Built-in ESP-NOW | 16 × ESP32-C3 = $64 |
| **Total** | **$160** | **$544** |

**Trade-off:** Daisy Seed costs ~3x more but offers superior audio quality and performance.

**Recommendation:**
- Prototype with 1-2 Daisy Seeds
- Production: Consider cost-optimized ESP32 for large installations
- Hybrid: Daisy Seed "master" node + ESP32 "slave" nodes

---

## 9. Implementation Timeline

### Minimal Viable Product (4-6 weeks)

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| **Phase 1.1-1.2** | 1 week | Core DSP ported, compiles |
| **Phase 1.3** | 1 week | Audio output working |
| **Phase 1.4** | 1 week | USB MIDI control |
| **Phase 2** | 1 week | Optimization, CV, presets |
| **Phase 3** | 2 weeks | Multi-node (ESP32 bridge or MIDI chain) |

### Extended Features (Optional)

- **Eurorack Module**: Design PCB with CV/Gate I/O (2-3 weeks)
- **Mobile App**: Bluetooth configuration (iOS/Android) (4-6 weeks)
- **Advanced Synthesis**: Karplus-Strong, waveshaping (1-2 weeks)

---

## 10. Code Migration Checklist

### Files to Copy (Minimal Changes)
- [x] `core/modal_node.h` → Direct copy
- [x] `core/modal_node.c` → Replace RNG function
- [x] `network/protocol.h` → Protocol definitions (reusable)

### Files to Adapt
- [ ] `audio/audio_synth.h` → Remove I2S, add callback interface
- [ ] `audio/audio_synth.c` → Extract synthesis, rewrite buffer management

### Files to Rewrite
- [ ] `audio/audio_i2s.c` → New: `daisy_audio_adapter.cpp`
- [ ] `network/esp_now_manager.c` → New: `uart_bridge.cpp` or `midi_sysex.cpp`
- [ ] `main.c` → New: `main.cpp` (Arduino setup/loop)

### New Files Needed
- [ ] `midi/usb_midi_handler.cpp`
- [ ] `config/preset_manager.cpp` (save to QSPI flash)
- [ ] `ui/cv_input.cpp` (if using CV)

---

## 11. Recommended Next Steps

### Immediate Actions (This Week)
1. **Order Hardware**: 2× Daisy Seed boards ($60)
2. **Setup Toolchain**: Install PlatformIO + Daisy libraries
3. **Test Build**: Compile "Hello World" Daisy example

### Short Term (2-4 Weeks)
1. **Port Core DSP**: Get modal_node.c compiling on Daisy
2. **Audio Output**: Implement basic sine wave AudioCallback
3. **MIDI Input**: USB MIDI triggers poke events

### Medium Term (1-2 Months)
1. **Single-Node Demo**: Standalone playable instrument
2. **Documentation**: User manual for MIDI mapping
3. **Testing**: Stability, audio quality benchmarks

### Long Term (3-6 Months)
1. **Multi-Node Prototype**: 4-node network with ESP32 bridges
2. **Eurorack Version**: PCB design for modular synth integration
3. **Performance Tool**: Live patching, preset morphing

---

## 12. Open Questions

### Q1: Complex Number Library
Should we use:
- **A:** Standard C99 `<complex.h>` (simplest)
- **B:** ARM CMSIS DSP (fastest)
- **C:** Custom struct (most portable)

**Recommendation:** Start with A, optimize to B if needed.

### Q2: Network Architecture
For 16-node system:
- **A:** MIDI chain (no extra hardware, but latency)
- **B:** ESP32 UART bridges (extra cost, low latency)
- **C:** Single Daisy Seed + 15× ESP32 audio nodes (hybrid)

**Recommendation:** Prototype with A, production with B if latency is critical.

### Q3: Audio Channel Mapping
- **A:** Stereo sum (all modes mixed to L/R)
- **B:** Quad output (4 discrete channels)
- **C:** Spatial audio (binaural, Ambisonics)

**Recommendation:** Start with A for Daisy Seed, use B if custom hardware.

---

## 13. Conclusion

The ESP32 modal resonator DSP is well-suited for porting to Daisy Seed. The core algorithms are platform-agnostic, and the Daisy platform offers significant advantages in audio quality and processing power. The main challenges are:

1. **Audio interface adaptation** (manageable with AudioCallback pattern)
2. **Network layer replacement** (solvable with ESP32 bridge or MIDI)

**Recommended Approach:**
- Phase 1: Single-node Daisy Seed prototype (4-6 weeks)
- Phase 2: Evaluate networking options based on latency requirements
- Phase 3: Scale to multi-node system if network performance is acceptable

The resulting instrument will have superior audio fidelity and lower CPU usage compared to the ESP32 implementation, making it suitable for both standalone use and integration into larger modular/Eurorack systems.

---

## Appendix A: Daisy Seed Pinout Reference

```
Daisy Seed GPIO Mapping (for CV/Gate expansion)

Audio:
- Codec: WM8731 (I2S, built-in)
- ADC0-7: GPIO 15-22 (CV inputs, 0-3.3V)
- DAC: GPIO 17,18 (optional auxiliary)

MIDI:
- USB: Built-in USB-C connector
- UART TX/RX: GPIO 13,14 (for ESP32 bridge)

Digital I/O:
- Gate In: GPIO 23 (trigger input)
- Gate Out: GPIO 24 (envelope out)
- LED: GPIO 28 (status indicator)

I2C (for expansion):
- SCL: GPIO 11
- SDA: GPIO 12
```

---

## Appendix B: Sample Code Repository Structure

```
daisy-resonant-network/
├── README.md
├── platformio.ini
├── src/
│   ├── main.cpp                      // Arduino entry point
│   ├── config.h                      // Build-time configuration
│   ├── core/
│   │   ├── modal_node.h              // From ESP32 (unchanged)
│   │   ├── modal_node.c              // From ESP32 (minor edits)
│   │   └── types.h                   // Platform-agnostic types
│   ├── audio/
│   │   ├── audio_synth.h             // Synthesis interface
│   │   ├── audio_synth.cpp           // Pure synthesis code
│   │   └── daisy_audio_driver.cpp    // Daisy AudioCallback
│   ├── midi/
│   │   ├── usb_midi_handler.h
│   │   └── usb_midi_handler.cpp
│   ├── network/
│   │   ├── protocol.h                // From ESP32 (unchanged)
│   │   ├── uart_bridge.cpp           // ESP32 bridge (optional)
│   │   └── midi_sysex.cpp            // MIDI network (alternative)
│   └── ui/
│       ├── cv_inputs.cpp             // Analog CV reading
│       └── preset_manager.cpp        // Flash storage
├── test/
│   ├── test_modal_node.cpp           // Unit tests
│   └── test_audio_quality.cpp
└── docs/
    ├── DAISY_QUICKSTART.md
    └── MIDI_MAPPING.md
```

---

**Document Status:** Ready for Review
**Next Action:** Approve proposal → Order hardware → Begin Phase 1
