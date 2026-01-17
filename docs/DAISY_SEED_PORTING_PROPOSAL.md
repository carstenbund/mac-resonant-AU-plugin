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

#### Step 2.4: Hardware UI Integration (Pots, Switches, LEDs, LCD)

The Daisy Seed has extensive GPIO and peripherals for building a complete hardware interface:

**Available I/O:**
- 8x ADC inputs (12-bit, 0-3.3V) → Potentiometers, CV inputs
- 28x GPIO pins → Buttons, encoders, gate inputs
- 2x SPI buses → OLED/TFT displays, shift registers
- 2x I2C buses → OLED displays, LED drivers (TLC59711)
- Multiple UART/I2S/USB for expansion

##### Option A: Potentiometers + LEDs (Simple Panel)

**Hardware:**
- 8x 10kΩ linear potentiometers
- 4x RGB LEDs (mode indicators)
- 2x momentary switches (preset up/down)
- 1x SPST toggle (personality switch)

**Pinout Example:**
```cpp
// ADC (Potentiometers)
#define POT_MODE0_FREQ    ADC_0  // GPIO 15
#define POT_MODE0_DAMPING ADC_1  // GPIO 16
#define POT_MODE1_FREQ    ADC_2  // GPIO 17
#define POT_MODE1_DAMPING ADC_3  // GPIO 18
#define POT_COUPLING      ADC_4  // GPIO 19
#define POT_MASTER_GAIN   ADC_5  // GPIO 20
#define POT_MODE_MIX      ADC_6  // GPIO 21
#define POT_BRIGHTNESS    ADC_7  // GPIO 22

// Digital I/O (Buttons)
#define BTN_PRESET_UP     GPIO_23
#define BTN_PRESET_DOWN   GPIO_24
#define SWITCH_PERSONALITY GPIO_25

// PWM (RGB LEDs - mode indicators)
#define LED_MODE0_R       GPIO_1
#define LED_MODE0_G       GPIO_2
#define LED_MODE0_B       GPIO_3
// ... (repeat for modes 1-3)
```

**Implementation:**
```cpp
// In setup()
hw.adc.Init(AdcChannelConfig, 8);  // Initialize 8 ADC channels
hw.adc.Start();

pinMode(BTN_PRESET_UP, INPUT_PULLUP);
pinMode(BTN_PRESET_DOWN, INPUT_PULLUP);
pinMode(SWITCH_PERSONALITY, INPUT_PULLUP);

// In loop() - 100Hz UI update rate
void ProcessHardwareUI() {
    static uint32_t last_ui_update = 0;
    if (System::GetNow() - last_ui_update < 10) return;  // 100Hz
    last_ui_update = System::GetNow();

    // Read potentiometers (0.0 - 1.0)
    float pot_values[8];
    for (int i = 0; i < 8; i++) {
        pot_values[i] = hw.adc.GetFloat(i);
    }

    // Map to modal parameters
    // Mode 0 frequency: 50 Hz - 1000 Hz
    float freq0 = 50.0f + pot_values[0] * 950.0f;
    modal_node_set_mode(&node, 0,
                       freq_to_omega(freq0),
                       pot_values[1] * 3.0f,  // Damping 0-3
                       node.modes[0].params.weight);

    // Mode 1 frequency: 100 Hz - 2000 Hz
    float freq1 = 100.0f + pot_values[2] * 1900.0f;
    modal_node_set_mode(&node, 1,
                       freq_to_omega(freq1),
                       pot_values[3] * 3.0f,
                       node.modes[1].params.weight);

    // Coupling strength
    node.coupling_strength = pot_values[4];

    // Master gain
    audio_synth_set_gain(&synth, pot_values[5]);

    // Mode mix (crossfade between modes)
    float mix = pot_values[6];
    node.modes[0].params.weight = 1.0f - mix;
    node.modes[1].params.weight = mix;

    // Brightness (Mode 2 gain)
    node.modes[2].params.weight = pot_values[7];

    // Read buttons (with debounce)
    static bool btn_up_last = false;
    bool btn_up = !digitalRead(BTN_PRESET_UP);
    if (btn_up && !btn_up_last) {
        LoadNextPreset();
    }
    btn_up_last = btn_up;

    // Read personality switch
    bool is_oscillator = digitalRead(SWITCH_PERSONALITY);
    node.personality = is_oscillator ? PERSONALITY_SELF_OSCILLATOR
                                     : PERSONALITY_RESONATOR;

    // Update mode indicator LEDs (brightness = mode energy)
    for (int k = 0; k < MAX_MODES; k++) {
        float energy = cabsf(node.modes[k].a);
        SetRGBLED(k, energy, 0.2f, 0.5f);  // Hue varies by mode
    }
}

void SetRGBLED(int led_idx, float brightness, float hue, float saturation) {
    // Convert HSV to RGB
    // ... (standard HSV->RGB conversion)

    // PWM output
    analogWrite(LED_MODE0_R + led_idx * 3, r * 255);
    analogWrite(LED_MODE0_G + led_idx * 3, g * 255);
    analogWrite(LED_MODE0_B + led_idx * 3, b * 255);
}
```

##### Option B: OLED Display (128x64, I2C)

**Hardware:** Adafruit SSD1306 OLED or similar

**Wiring:**
```
OLED SDA → GPIO 12 (I2C1 SDA)
OLED SCL → GPIO 11 (I2C1 SCL)
OLED VCC → 3.3V
OLED GND → GND
```

**Library:** Use Adafruit_SSD1306 or U8g2

**Implementation:**
```cpp
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
    // ... existing setup ...

    Wire.begin();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
}

void UpdateDisplay() {
    static uint32_t last_display_update = 0;
    if (System::GetNow() - last_display_update < 50) return;  // 20Hz
    last_display_update = System::GetNow();

    display.clearDisplay();

    // Title
    display.setCursor(0, 0);
    display.println("Modal Resonator");

    // Mode parameters (compact display)
    for (int k = 0; k < MAX_MODES; k++) {
        float freq = node.modes[k].params.omega / (2.0f * M_PI);
        float gamma = node.modes[k].params.gamma;
        float amp = cabsf(node.modes[k].a);

        display.setCursor(0, 16 + k * 12);
        display.print("M");
        display.print(k);
        display.print(": ");
        display.print((int)freq);
        display.print("Hz ");

        // Energy bar
        int bar_width = (int)(amp * 40.0f);
        if (bar_width > 40) bar_width = 40;
        for (int i = 0; i < bar_width; i++) {
            display.drawPixel(80 + i, 16 + k * 12, SSD1306_WHITE);
        }
    }

    display.display();
}
```

##### Option C: LED Bar Graph (Parameter Feedback)

**Hardware:** 4x 10-segment LED bar graphs (LM3914 drivers or shift registers)

**Purpose:** Real-time visualization of mode amplitudes

**Implementation with 74HC595 Shift Register:**
```cpp
#define LED_DATA  GPIO_26
#define LED_CLOCK GPIO_27
#define LED_LATCH GPIO_28

void UpdateLEDBars() {
    static uint32_t last_led_update = 0;
    if (System::GetNow() - last_led_update < 20) return;  // 50Hz
    last_led_update = System::GetNow();

    uint8_t led_data[4] = {0};  // 4 modes, 8 LEDs each

    for (int k = 0; k < MAX_MODES; k++) {
        float energy = cabsf(node.modes[k].a);

        // Convert to 8-LED bar (0-8 LEDs lit)
        int leds_lit = (int)(energy * 8.0f);
        if (leds_lit > 8) leds_lit = 8;

        // Create bit pattern (e.g., 0b00111111 for 6 LEDs)
        led_data[k] = (1 << leds_lit) - 1;
    }

    // Shift out data
    digitalWrite(LED_LATCH, LOW);
    for (int k = 0; k < 4; k++) {
        shiftOut(LED_DATA, LED_CLOCK, MSBFIRST, led_data[k]);
    }
    digitalWrite(LED_LATCH, HIGH);
}
```

##### Option D: TFT LCD Display (Color, 240x320, SPI)

**Hardware:** ILI9341 TFT display (Adafruit 2.8" or similar)

**Features:**
- Full-color spectral display
- Waveform visualization
- Menu system
- Parameter editing

**Wiring (SPI):**
```
TFT CS   → GPIO 7
TFT DC   → GPIO 8
TFT MOSI → GPIO 10 (SPI MOSI)
TFT SCK  → GPIO 9  (SPI SCK)
TFT RST  → GPIO 6
```

**Implementation:**
```cpp
#include <Adafruit_ILI9341.h>

#define TFT_CS   7
#define TFT_DC   8
#define TFT_RST  6
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
    // ... existing setup ...
    tft.begin();
    tft.setRotation(1);  // Landscape
    tft.fillScreen(ILI9341_BLACK);
}

void UpdateTFTDisplay() {
    static uint32_t last_update = 0;
    if (System::GetNow() - last_update < 100) return;  // 10Hz
    last_update = System::GetNow();

    // Draw mode waveforms
    for (int k = 0; k < MAX_MODES; k++) {
        int y_base = 40 + k * 60;

        // Mode info
        tft.setCursor(10, y_base - 15);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setTextSize(1);

        float freq = node.modes[k].params.omega / (2.0f * M_PI);
        tft.print("Mode ");
        tft.print(k);
        tft.print(": ");
        tft.print((int)freq);
        tft.print(" Hz");

        // Waveform (real-time phase)
        float amp = cabsf(node.modes[k].a);
        float phase = cargf(node.modes[k].a);

        for (int x = 0; x < 280; x++) {
            float t = (x / 280.0f) * 2.0f * M_PI;
            float y = amp * sinf(t + phase) * 20.0f;
            int pixel_y = y_base + (int)y;

            uint16_t color = GetModeColor(k);
            tft.drawPixel(20 + x, pixel_y, color);
        }
    }

    // Draw spectral display (frequency bars)
    // ... (FFT or mode frequency display)
}

uint16_t GetModeColor(int mode_idx) {
    switch (mode_idx) {
        case 0: return ILI9341_RED;
        case 1: return ILI9341_GREEN;
        case 2: return ILI9341_BLUE;
        case 3: return ILI9341_YELLOW;
        default: return ILI9341_WHITE;
    }
}
```

##### Option E: Rotary Encoders (Precise Parameter Control)

**Hardware:**
- 4x rotary encoders with push buttons (e.g., EC11)
- Each encoder controls one mode

**Implementation:**
```cpp
#include <Encoder.h>

Encoder enc_mode0(GPIO_1, GPIO_2);
Encoder enc_mode1(GPIO_3, GPIO_4);
Encoder enc_mode2(GPIO_5, GPIO_6);
Encoder enc_mode3(GPIO_7, GPIO_8);

#define ENC_BTN0 GPIO_23
#define ENC_BTN1 GPIO_24
#define ENC_BTN2 GPIO_25
#define ENC_BTN3 GPIO_26

typedef enum {
    EDIT_FREQUENCY,
    EDIT_DAMPING,
    EDIT_WEIGHT,
    EDIT_COUNT
} edit_mode_t;

edit_mode_t edit_modes[MAX_MODES] = {EDIT_FREQUENCY, EDIT_FREQUENCY,
                                      EDIT_FREQUENCY, EDIT_FREQUENCY};

void ProcessEncoders() {
    // Mode 0 encoder
    long pos0 = enc_mode0.read();
    if (pos0 != 0) {
        switch (edit_modes[0]) {
            case EDIT_FREQUENCY: {
                float freq = node.modes[0].params.omega / (2.0f * M_PI);
                freq += pos0 * 1.0f;  // 1 Hz per click
                if (freq < 20.0f) freq = 20.0f;
                if (freq > 2000.0f) freq = 2000.0f;
                modal_node_set_mode(&node, 0, freq_to_omega(freq),
                                   node.modes[0].params.gamma,
                                   node.modes[0].params.weight);
                break;
            }
            case EDIT_DAMPING:
                node.modes[0].params.gamma += pos0 * 0.01f;
                break;
            case EDIT_WEIGHT:
                node.modes[0].params.weight += pos0 * 0.01f;
                break;
        }
        enc_mode0.write(0);
    }

    // Button press cycles edit mode
    if (digitalRead(ENC_BTN0) == LOW) {
        delay(20);  // Debounce
        edit_modes[0] = (edit_mode_t)((edit_modes[0] + 1) % EDIT_COUNT);
        while (digitalRead(ENC_BTN0) == LOW);  // Wait for release
    }

    // Repeat for modes 1-3...
}
```

##### Option F: Complete Eurorack Panel Design

**Features:**
- 8x potentiometers (mode parameters)
- 4x rotary encoders (fine tuning)
- 4x CV inputs (external modulation)
- 4x gate inputs (trigger pokes)
- 1x OLED display (128x64)
- 4x RGB LEDs (mode indicators)
- 2x audio outputs (L/R)

**Panel Layout (16HP Eurorack):**
```
┌─────────────────────────────────┐
│  MODAL RESONATOR    [OLED]      │
│                                  │
│  [POT]  [POT]  [POT]  [POT]     │
│  Freq0  Damp0  Freq1  Damp1     │
│                                  │
│  [POT]  [POT]  [POT]  [POT]     │
│  Freq2  Damp2  Freq3  Damp3     │
│                                  │
│  [ENC0] [ENC1] [ENC2] [ENC3]    │
│   🔴    🟢    🔵    🟡         │
│                                  │
│  CV0  CV1  CV2  CV3             │
│  GATE0 GATE1 GATE2 GATE3        │
│                                  │
│  OUT_L  OUT_R                    │
└─────────────────────────────────┘
```

**CV Input Processing:**
```cpp
void ProcessCVGateInputs() {
    // CV inputs (0-5V → 0-1.0, use voltage divider to 3.3V)
    float cv[4];
    for (int i = 0; i < 4; i++) {
        cv[i] = hw.adc.GetFloat(i + 4);  // ADC channels 4-7
    }

    // CV0 → Mode 0 frequency (V/Oct)
    float volts = cv[0] * 5.0f;
    float freq = 440.0f * powf(2.0f, volts - 3.0f);  // A4 at 3V
    node.modes[0].params.omega = freq_to_omega(freq);

    // CV1 → Mode 0 damping
    node.modes[0].params.gamma = cv[1] * 3.0f;

    // Gate inputs (trigger pokes)
    static bool gate_last[4] = {false};
    for (int i = 0; i < 4; i++) {
        bool gate = digitalRead(GPIO_23 + i);
        if (gate && !gate_last[i]) {
            // Gate rising edge → trigger poke
            poke_event_t poke;
            poke.source_node_id = 0;
            poke.strength = 0.8f;
            poke.phase_hint = -1.0f;  // Random
            poke.mode_weights[i] = 1.0f;  // Target mode
            modal_node_apply_poke(&node, &poke);
        }
        gate_last[i] = gate;
    }
}
```

##### Hardware Recommendations

| Component | Recommended Part | Price | Notes |
|-----------|------------------|-------|-------|
| **Pots** | Alpha 9mm linear | $0.50 ea | Smooth, reliable |
| **Encoders** | EC11 rotary encoder | $1.50 ea | Detents, push button |
| **OLED** | SSD1306 128x64 I2C | $5 | Low power, crisp |
| **TFT LCD** | ILI9341 2.8" SPI | $15 | Color, fast refresh |
| **RGB LEDs** | WS2812B (NeoPixel) | $0.30 ea | Individually addressable |
| **LED Bars** | 10-segment bar graph | $2 ea | Visual feedback |
| **Switches** | Tactile 6mm button | $0.10 ea | Panel mount |
| **Jacks** | Thonkiconn 3.5mm | $1 ea | Eurorack standard |

**Deliverables:**
- [ ] Optimized DSP (target <50% CPU @ 48kHz)
- [ ] CV input mapping (8 channels)
- [ ] Potentiometer support (8 pots)
- [ ] Button/encoder input
- [ ] RGB LED mode indicators (4 LEDs)
- [ ] OLED display integration (128x64)
- [ ] Optional: TFT display (240x320)
- [ ] Preset system (save/load from flash)
- [ ] Hardware UI update rate: 100Hz

**Performance Impact:**
- ADC reading: <1% CPU
- OLED update (20Hz): ~2% CPU
- TFT update (10Hz): ~5% CPU
- LED PWM: negligible (hardware)
- **Total with full UI: ~20% CPU** (still 65% headroom!)

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
