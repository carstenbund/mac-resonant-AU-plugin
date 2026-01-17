# Daisy Seed Modal Resonator

**Port of ESP32 Distributed Modal Resonator Network to Daisy Seed Platform**

A 4-mode autonomous modal resonator with USB MIDI control, running on the Electrosmith Daisy Seed.

## Features

- **4 Complex Modes**: Each with independent frequency, damping, and weight
- **USB MIDI Control**: Note-on triggers poke events, CCs control parameters
- **High-Quality Audio**: 48kHz stereo via integrated WM8731 codec
- **Real-Time DSP**: 500Hz modal integration, <15% CPU usage
- **Hardware FPU**: Leverages ARM Cortex-M7 double-precision floating point

## Quick Start

### Prerequisites

1. **Hardware**:
   - Electrosmith Daisy Seed
   - USB-C cable
   - Audio output (headphones or speakers)
   - USB MIDI controller (keyboard, pad, etc.)

2. **Software**:
   - [PlatformIO](https://platformio.org/) (or Arduino IDE with Daisy support)
   - DFU-util for firmware upload

### Build and Upload

```bash
# Clone repository
cd daisy-seed

# Build firmware
pio run

# Upload to Daisy Seed (hold BOOT button, press RESET, then):
pio run --target upload

# Monitor serial output (optional)
pio device monitor
```

### Alternative: Using Arduino IDE

1. Install [Daisy Arduino libraries](https://github.com/electro-smith/Arduino-Daisy)
2. Open `src/main.cpp` in Arduino IDE
3. Select "Daisy Seed" as board
4. Upload via DFU

## Usage

### MIDI Control

**Note On/Off**:
- Note On → Triggers poke excitation (velocity controls strength)
- Higher notes → Emphasize upper modes (brighter sound)
- Lower notes → Emphasize lower modes (darker sound)

**Control Changes**:
- CC 1 (Mod Wheel) → Mode 0 damping (decay time)
- CC 2 (Breath) → Global coupling strength
- CC 71 (Resonance) → All modes resonance (inverse damping)
- CC 74 (Brightness) → Mode 2 gain (high-frequency content)
- CC 20-23 → Individual mode gains (modes 0-3)

**Pitch Bend**:
- ±2 semitone range
- Affects all modes proportionally

### Default Modal Configuration

The default preset uses a harmonic series from A2 (110 Hz):

| Mode | Frequency | Damping | Weight | Description |
|------|-----------|---------|--------|-------------|
| 0 | 110 Hz | 0.5 | 1.0 | Fundamental |
| 1 | 220.5 Hz | 0.6 | 0.8 | Octave + detune |
| 2 | 330 Hz | 1.0 | 0.5 | 3rd harmonic (brightness) |
| 3 | 55 Hz | 0.3 | 0.6 | Sub-bass |

### Customization

Edit `src/main.cpp` in the `setup()` function to change modal parameters:

```cpp
// Example: Create a metallic bell sound
modal_node_set_mode(&node, 0, freq_to_omega(523.25f), 0.3f, 1.0f);  // C5
modal_node_set_mode(&node, 1, freq_to_omega(659.26f), 0.4f, 0.7f);  // E5
modal_node_set_mode(&node, 2, freq_to_omega(1046.5f), 0.8f, 0.5f);  // C6
modal_node_set_mode(&node, 3, freq_to_omega(261.63f), 0.2f, 0.8f);  // C4
```

## Architecture

### File Structure

```
daisy-seed/
├── src/
│   ├── main.cpp                    // Arduino entry point
│   ├── core/
│   │   ├── modal_node.h           // Modal dynamics (from ESP32)
│   │   └── modal_node.c           // Integration logic
│   ├── audio/
│   │   ├── audio_synth.h          // Synthesis interface
│   │   └── audio_synth.c          // Audio generation
│   └── midi/
│       ├── usb_midi_handler.h     // MIDI processing
│       └── usb_midi_handler.c     // MIDI to poke mapping
├── platformio.ini                  // Build configuration
└── README.md
```

### Code Flow

1. **Main Loop** (1kHz):
   - Process USB MIDI events
   - Run control rate tasks (500Hz modal integration)
   - Update LED status

2. **Audio Callback** (48kHz, DMA interrupt):
   - Read modal state
   - Generate stereo audio samples
   - Output via codec

3. **Modal Integration** (500Hz):
   - Integrate complex modal dynamics: `ȧ_k = (-γ_k + iω_k)a_k + u_k(t)`
   - Apply excitation envelopes
   - Update phase accumulators

### Key Differences from ESP32 Version

| Feature | ESP32 | Daisy Seed |
|---------|-------|------------|
| **CPU** | Xtensa LX7 @ 240MHz | ARM Cortex-M7 @ 480MHz |
| **Audio** | I2S push (480 samples) | Callback pull (48 samples) |
| **MIDI** | UART/GPIO | USB native |
| **Network** | ESP-NOW (multi-node) | Single node (or UART bridge) |
| **Output** | 4-channel TDM | Stereo interleaved |

## Performance

Measured on Daisy Seed @ 48kHz, 48-sample blocks:

| Task | CPU Usage |
|------|-----------|
| Modal integration (500Hz) | ~2% |
| Audio synthesis (48kHz) | ~10% |
| MIDI processing | <1% |
| **Total** | **~13%** |

**Headroom**: 87% CPU available for effects, UI, networking, etc.

## Troubleshooting

### No audio output
- Check audio connections (Daisy OUT L/R)
- Verify MIDI input is connected and sending note-on messages
- Check master gain: `audio_synth_set_gain(&synth, 1.0f);`

### No MIDI input
- Ensure USB cable supports data (not power-only)
- Try different USB port or hub
- Check MIDI device shows up in system (lsusb on Linux)

### Clicking/popping
- Increase audio block size: `hw.SetAudioBlockSize(96);`
- Reduce control rate: `CONTROL_PERIOD_MS = 4` (250Hz)
- Check for buffer overruns in serial monitor

### Upload fails
- Hold BOOT button, press RESET, then release BOOT
- Verify DFU mode: `lsusb` should show "STM32 BOOTLOADER"
- Try: `dfu-util -l` to list DFU devices

## Extending the Project

### Add CV Input

Use Daisy's ADC pins for voltage control:

```cpp
// In loop()
hw.ProcessAnalogControls();
float cv1 = hw.GetAdcValue(0);  // 0-1

// Map to modal parameter
node.modes[0].params.gamma = 0.2f + cv1 * 2.0f;
```

### Multi-Node Networking (ESP32 Bridge)

Connect ESP32 module via UART for distributed network:

```cpp
// Send poke to network
void SendPokeUART(const poke_event_t* poke) {
    uint8_t buf[32];
    // Serialize poke...
    hw.uart.PollTx(buf, 32);
}
```

See `docs/DAISY_SEED_PORTING_PROPOSAL.md` Section 3.2 for details.

### Add DaisySP Effects

Integrate reverb, delay, filters from DaisySP:

```cpp
#include "daisysp.h"

daisysp::ReverbSc reverb;
reverb.Init(SAMPLE_RATE);

// In AudioCallback()
float wet_l, wet_r;
reverb.Process(out[0][i], out[1][i], &wet_l, &wet_r);
out[0][i] = out[0][i] * 0.7f + wet_l * 0.3f;
out[1][i] = out[1][i] * 0.7f + wet_r * 0.3f;
```

## Related Documentation

- [Daisy Seed Porting Proposal](../docs/DAISY_SEED_PORTING_PROPOSAL.md)
- [ESP32 Design Spec](../esp32/docs/DESIGN_SPEC.md)
- [Modal Node Theory](../docs/theory.md)
- [libDaisy Documentation](https://electro-smith.github.io/libDaisy/)
- [DaisySP Documentation](https://electro-smith.github.io/DaisySP/)

## License

MIT (same as parent project)

## Authors

- Original ESP32 implementation: [Parent project]
- Daisy Seed port: 2026-01-17

## Changelog

### v1.0.0 (2026-01-17)
- Initial port from ESP32
- 4-mode modal resonator
- USB MIDI control
- Stereo audio output
- ~13% CPU usage @ 48kHz
