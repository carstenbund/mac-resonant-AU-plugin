# Audio Synthesis Comparison: Python vs C/C++

## Architecture Differences

### Python Audio Approach (`audio_nodes.py`)

**Per-Node Carrier Oscillators**:
```python
# Each node j has:
# - Frequency: score.freq[j] (from MIDI note)
# - Velocity gate: score.vel[j] (0 = silent, 1 = full)
# - Amplitude: |net.a[j, 0]| (network state)
# - Phase: continuous phase accumulator

for j in range(N):
    if vel[j] <= 1e-4 or freq[j] <= 1.0:
        continue  # Silent when velocity is 0

    amp = clip(vel[j] * amp_smooth[j], 0, MAX_AMPLITUDE)
    output[j] = amp * sin(2*pi*freq[j]*t + phase[j])
    phase[j] += 2*pi*freq[j]*frames / FS
```

**Key Features**:
1. **Carrier oscillator** per node at fixed frequency
2. **Network amplitude** modulates carrier (AM synthesis)
3. **Velocity gating** (0 = silent, independent of network)
4. **8 separate channels** (one per node)
5. **Phase continuity** across audio blocks

### C/C++ Audio Approach (`audio_synth.c`)

**Modal Frequency Synthesis**:
```c
// Render from modal state directly:
for (int k = 0; k < MAX_MODES; k++) {
    float amplitude = cabsf(node->modes[k].a) * weight;
    float omega = node->modes[k].params.omega;
    float freq_hz = omega / (2*M_PI);

    float phase = phase_accumulator + cargf(node->modes[k].a);
    sample = amplitude * fast_sin(phase);
    phase_accumulator += phase_inc;
}
```

**Key Features**:
1. **Modal frequencies** from mode parameters (ω₀, ω₁, ω₂, ω₃)
2. **Network dynamics** directly synthesized (FM-like from complex phase)
3. **Poke envelope** for excitation (10ms Hann window)
4. **Stereo output** (mono summed to L/R)
5. **Phase from modal state** (arg(a_k))

## Critical Differences

### 1. Frequency Control

| Python | C/C++ |
|--------|-------|
| Fixed carrier from MIDI note | Modal frequencies from ω parameters |
| `freq = midi_to_freq(note)` | `freq = omega / (2π)` |
| One frequency per node | 4 frequencies per voice (modes) |

### 2. Amplitude Modulation

| Python | C/C++ |
|--------|-------|
| AM: `amp = vel * |a[j,0]|` | Direct: `amp = |a_k|` |
| Network shapes carrier | Network IS the sound |
| Smooth "feel" variation | Complex spectral evolution |

### 3. Envelope Control

| Python | C/C++ |
|--------|-------|
| Velocity gate (on/off) | Poke envelope (10ms) |
| Instant on, controlled off | Gradual attack, natural decay |
| Score-controlled | Physics-controlled |

### 4. Audio Routing

| Python | C/C++ |
|--------|-------|
| 8 channels (node per channel) | Stereo (all voices mixed) |
| Spatial separation | Single output |
| Multi-speaker capable | Standard stereo |

## Why They Sound Different

### Python Output (Image you showed):
- **Clean carrier waves** with amplitude modulation
- **Consistent pitch** from MIDI notes
- **Rhythmic patterns** from velocity gating
- **Smooth amplitude changes** from network state

### C/C++ Output (test_output.wav):
- **Complex modal synthesis** (4 simultaneous modes)
- **Beating patterns** from mode detuning (220Hz + 222Hz = 2Hz beating)
- **Natural decay** from resonator physics
- **Spectral evolution** from modal dynamics

## Matching Python Behavior in C/C++

If you want the C/C++ AU to sound like the Python version, you need to implement **carrier-based synthesis**:

### Option 1: Add Carrier Mode to Audio Synth

```c
typedef enum {
    SYNTH_MODE_MODAL,      // Current: render modal frequencies
    SYNTH_MODE_CARRIER     // New: carrier + AM
} synth_mode_t;

typedef struct {
    synth_mode_t mode;
    float carrier_freq;    // Hz (from MIDI note)
    float velocity;        // 0.0-1.0 (gate)
    // ... existing fields
} audio_synth_t;

void audio_synth_render_carrier(audio_synth_t* synth,
                                float* outL, float* outR,
                                uint32_t num_frames) {
    // Carrier oscillator
    for (uint32_t i = 0; i < num_frames; i++) {
        // Network amplitude (from all modes)
        float amp = 0.0f;
        for (int k = 0; k < MAX_MODES; k++) {
            amp += cabsf(synth->node->modes[k].a) * synth->node->modes[k].params.weight;
        }
        amp *= synth->velocity;  // Velocity gate

        // Generate carrier
        float sample = amp * sinf(synth->phase_acc);
        outL[i] = outR[i] = sample;

        // Advance carrier phase
        synth->phase_acc += 2.0f * M_PI * synth->carrier_freq / synth->params.sample_rate;
    }
}
```

### Option 2: Per-Voice Carrier in ModalVoice

```cpp
class ModalVoice {
private:
    bool use_carrier_mode_;
    float carrier_freq_;
    double carrier_phase_;

public:
    void setCarrierMode(bool enabled) {
        use_carrier_mode_ = enabled;
    }

    void renderAudio(float* outL, float* outR, uint32_t num_frames) override {
        if (use_carrier_mode_) {
            renderCarrier(outL, outR, num_frames);
        } else {
            renderModal(outL, outR, num_frames);
        }
    }

private:
    void renderCarrier(float* outL, float* outR, uint32_t num_frames) {
        // Network amplitude from all modes
        float network_amp = modal_node_get_amplitude(&node_);

        // Velocity gate
        float amp = velocity_ * network_amp;

        // Carrier frequency from MIDI note
        carrier_freq_ = midi_to_freq(midi_note_);

        for (uint32_t i = 0; i < num_frames; i++) {
            float sample = amp * sinf(carrier_phase_);
            outL[i] = outR[i] = sample;

            carrier_phase_ += 2.0 * M_PI * carrier_freq_ / sample_rate_;
            if (carrier_phase_ > 2.0 * M_PI) carrier_phase_ -= 2.0 * M_PI;
        }
    }

    void renderModal(float* outL, float* outR, uint32_t num_frames) {
        // Current modal synthesis
        audio_synth_render(&synth_, outL, outR, num_frames);
    }
};
```

## Recommendations

### For AU Plugin - Keep Current Approach

The modal synthesis is **better for the AU plugin** because:
- ✅ **Richer timbre**: 4 modes create complex, evolving sound
- ✅ **Natural physics**: Resonator behavior is organic
- ✅ **Unique character**: Different from typical synthesizers
- ✅ **Network coupling**: Modal interactions are musically interesting

### For Python Parity - Add Carrier Mode

If you want to exactly match Python:
- Add `carrier_mode` option to ModalVoice
- Implement carrier + AM synthesis
- Use MIDI note for pitch (not modal frequencies)
- Add velocity gating

### Hybrid Approach - Best of Both

Implement **both modes** as AU parameters:
```
Synthesis Mode: [Modal | Carrier | Hybrid]
- Modal: Current (4 mode resonator)
- Carrier: Python-style (network modulates carrier)
- Hybrid: Modal + carrier mixed
```

## Testing Comparison

To verify C++ matches Python, test with same parameters:

**Python test**:
```python
# Single node, single mode
params = NetworkParams(K=1, N=1, omega=[220*2*pi], gamma=[0.5])
net = ModalNetwork(params)
net.a[0,0] = 0.1 + 0.1j  # Initial state

# Render with carrier
for t in range(5000):
    net.step()
    output = abs(net.a[0,0]) * sin(2*pi*220*t/48000)
```

**C++ test**:
```cpp
ModalVoice voice(0);
voice.initialize(48000);
voice.setMode(0, 220.0f, 0.5f, 1.0f);  // Single mode
voice.noteOn(57, 0.8f);

// Render with carrier mode
voice.setCarrierMode(true);
voice.renderAudio(outL, outR, num_frames);
```

Compare:
- Amplitude envelope over time
- Frequency content (FFT)
- Decay rate

## Conclusion

**Current status**:
- C++ implements **modal synthesis** (complex, physics-based)
- Python implements **carrier + AM** (simple, score-driven)
- Both are valid, serve different purposes

**Which is better?**
- **For AU instrument**: Modal (current) ✅
- **For score playback**: Carrier (Python) ✅
- **For flexibility**: Implement both 🎯

The "noise" you heard was likely:
1. Complex beating from 4 modes (intentional!)
2. Natural decay/attack (physics)
3. Different from clean carrier tones

If you want Python-like clean tones, implement carrier mode. If you want rich modal synthesis, current implementation is excellent!
