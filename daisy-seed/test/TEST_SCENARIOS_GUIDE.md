# Automated Test Scenarios Guide

**Comprehensive testing suite for Daisy Seed Modal Resonator**

This document describes all automated test scenarios, their purpose, and expected sonic behaviors.

---

## Quick Start

### Build and Run Test Suite

```bash
# Build test firmware
pio run -e daisy_seed_test

# Upload to Daisy Seed
pio run -e daisy_seed_test --target upload

# Monitor test results (serial output)
pio device monitor -e daisy_seed_test
```

### Operation

1. **Power on** Daisy Seed
2. LED blinks **5 times** to indicate ready
3. LED blinks **3 times** before each test
4. Each test runs for **10 seconds**
5. **2 second pause** between tests
6. Serial output shows real-time statistics
7. Cycle repeats automatically

---

## Test Scenarios

### 1. Single Poke Decay

**Purpose**: Measure modal decay envelope and verify integration accuracy

**What happens**:
- Single MIDI note-on at t=0 (Middle C, velocity 100)
- All 4 modes excited simultaneously
- System decays naturally for 10 seconds
- No additional input

**Expected sound**:
- Initial "ping" with all harmonics
- Exponential decay envelope
- Different modes decay at different rates:
  - Mode 0 (110 Hz): 2-second decay (γ = 0.5)
  - Mode 1 (220 Hz): 1.7-second decay (γ = 0.6)
  - Mode 2 (330 Hz): 1-second decay (γ = 1.0)
  - Mode 3 (55 Hz): 3.3-second decay (γ = 0.3)

**Test validation**:
- Peak amplitude at t=0 should be ~0.7-1.0
- Amplitude should decrease exponentially
- At t=10s, amplitude should be near zero

**Serial output example**:
```
=== Starting Test: Single Poke Decay ===
[    0 ms] Peak: 0.852  Avg: 0.421  Events: 1
[  100 ms] Peak: 0.743  Avg: 0.389  Events: 1
[  200 ms] Peak: 0.648  Avg: 0.341  Events: 1
...
[ 9900 ms] Peak: 0.002  Avg: 0.001  Events: 1
```

---

### 2. Note Sequence

**Purpose**: Verify MIDI note processing and mode re-triggering

**What happens**:
- Plays C major scale: C D E F G A B C
- 800ms per note (total 8 notes in 6.4 seconds)
- Velocity 80 for all notes

**Expected sound**:
- Melodic sequence
- Each note triggers fresh poke
- Overlapping decays create polyphonic texture

**Test validation**:
- 8 note events total
- Regular amplitude pattern (800ms period)
- No clicks or pops between notes

**Use case**:
- Verifies musical playability
- Tests rapid mode re-excitation
- Demonstrates polyphonic decay behavior

---

### 3. Damping Sweep

**Purpose**: Demonstrate effect of damping parameter on decay time

**What happens**:
- Poke event every 1 second (10 total)
- Damping increases linearly from 0 to max (CC1)
- Same note (Middle C) each time

**Expected sound**:
- First pokes: Long decay (low damping)
- Later pokes: Short decay (high damping)
- Progressive "drying out" of resonance

**Test validation**:
- Decay time should decrease over test duration
- Final pokes should be very short (percussive)

**Parameter mapping**:
- t=0s: CC1=0 → γ=0.2 (5-second decay)
- t=5s: CC1=64 → γ=1.1 (0.9-second decay)
- t=10s: CC1=127 → γ=2.2 (0.45-second decay)

---

### 4. Coupling Sweep

**Purpose**: Test coupling strength parameter (network interaction simulation)

**What happens**:
- Random notes every 500ms
- Coupling strength increases from 0 to 1 (CC2)

**Expected sound**:
- Initially: Independent notes
- Later: More "connected" feeling (coupling affects excitation)

**Note**: In single-node mode, coupling is less pronounced. This test is more relevant for multi-node systems.

---

### 5. Velocity Ramp

**Purpose**: Verify velocity scaling of poke strength

**What happens**:
- 12 notes at 800ms intervals
- Velocity increases from 10 to 127
- Same note (Middle C) each time

**Expected sound**:
- First notes: Quiet, gentle
- Later notes: Loud, strong
- Progressive crescendo

**Test validation**:
- Peak amplitude should increase with each note
- Linear relationship between velocity and amplitude

**Serial output pattern**:
```
Note 1 (vel 10):  Peak: 0.08
Note 2 (vel 20):  Peak: 0.16
Note 3 (vel 30):  Peak: 0.24
...
Note 12 (vel 127): Peak: 1.0
```

---

### 6. Rapid Fire

**Purpose**: Stress test - verify stability under high event rate

**What happens**:
- Random notes every 50ms (20 Hz event rate)
- Random pitches (C3 to C5)
- Random velocities (60-100)

**Expected sound**:
- Dense, chaotic texture
- "Machine gun" of modal pokes
- Continuous sound due to overlapping decays

**Test validation**:
- ~200 events in 10 seconds
- No audio dropouts or buffer overruns
- CPU should remain under 20%
- No clicking or distortion

**Use case**:
- Tests real-time performance
- Verifies excitation queue handling
- Demonstrates polyphonic capacity

---

### 7. Mode Isolation

**Purpose**: Hear each mode individually

**What happens**:
- Test divided into 4 segments (2.5s each)
- Segment 1: Only Mode 0 (110 Hz)
- Segment 2: Only Mode 1 (220 Hz)
- Segment 3: Only Mode 2 (330 Hz)
- Segment 4: Only Mode 3 (55 Hz)
- Poke at start of each segment

**Expected sound**:
- Segment 1: Deep fundamental tone
- Segment 2: Octave higher, shorter decay
- Segment 3: Bright, fast decay
- Segment 4: Sub-bass rumble, long decay

**Test validation**:
- Clear frequency separation
- Mode weights correctly applied
- Demonstrates timbral control

---

### 8. Harmonic Series

**Purpose**: Demonstrate harmonic relationships

**What happens**:
- Plays harmonic series of C2 (MIDI 36)
- Notes: C2, C3, C4, G4, C5, E5, G5, Bb5, C6, D6
- 1 second per note

**Expected sound**:
- Rising pitch sequence
- Intervals get smaller (converging on C)
- Natural harmonic timbre

**Musical significance**:
- These are the natural harmonics of a vibrating string
- Demonstrates modal resonator can play traditional pitch content
- Tests wide frequency range (65 Hz to 1175 Hz)

---

### 9. Resonance Sweep

**Purpose**: Demonstrate resonance parameter effect

**What happens**:
- Poke every 800ms
- Resonance sweeps in triangle wave: 0 → 127 → 0
- CC71 controls resonance (inverse damping)

**Expected sound**:
- 0-5s: Resonance increasing (longer decays)
- 5-10s: Resonance decreasing (shorter decays)
- Mid-point: Maximum "ringing"

**Test validation**:
- Decay time should follow triangle pattern
- Maximum resonance at t=5s
- Demonstrates expressive control

---

### 10. Brightness Sweep

**Purpose**: Demonstrate timbral control via Mode 2 gain

**What happens**:
- Poke every 600ms
- Brightness (Mode 2 gain) increases linearly (CC74)

**Expected sound**:
- Early: Dark, mellow (low Mode 2)
- Later: Bright, sharp (high Mode 2)
- Progressive "opening" of tone

**Test validation**:
- High-frequency content increases
- Spectral centroid rises
- Demonstrates timbral expressiveness

**Mode 2 characteristics**:
- Frequency: 330 Hz
- Fast decay (γ = 1.0)
- Provides "brightness" to timbre

---

### 11. Pitch Bend Sweep

**Purpose**: Verify pitch bend implementation

**What happens**:
- Single poke at t=0
- Continuous pitch bend sweep: -2 semitones → +2 semitones

**Expected sound**:
- Sustained tone with gliding pitch
- Smooth frequency modulation
- No clicks during bend

**Test validation**:
- Frequency should change smoothly
- Pitch range: 110 Hz → 116.5 Hz → 123.5 Hz
- All modes affected proportionally

**Technical note**:
- Pitch bend modifies mode ω parameters
- Standard ±2 semitone range
- Useful for expressive performance

---

### 12. Chaos Mode

**Purpose**: Stress test and sonic exploration

**What happens**:
- Random notes at random intervals (50-500ms)
- Random pitches (C2 to C6)
- Random velocities (40-127)
- Random CC changes every 200ms (damping, resonance, brightness, coupling)

**Expected sound**:
- Unpredictable, evolving texture
- Constantly changing timbre and decay
- "Generative music" quality

**Test validation**:
- System remains stable under random input
- No crashes or audio artifacts
- Demonstrates robustness

**Use case**:
- Stress testing
- Finding unexpected sonic possibilities
- Verifying all parameter interactions work

---

## Interpreting Serial Output

### Real-time Statistics

```
[ 1500 ms] Peak: 0.743  Avg: 0.389  Events: 3
    ^         ^           ^           ^
    |         |           |           └─ Total events triggered
    |         |           └─ Average amplitude this interval
    |         └─ Peak amplitude this interval (last 100ms)
    └─ Elapsed time since test start
```

### Test Summary

```
=== Test Complete: Damping Sweep ===
Duration: 10000 ms
Events triggered: 10
Peak amplitude: 0.952
Average amplitude: 0.284
Samples processed: 480000
```

**Metrics**:
- **Duration**: Actual test runtime (should be ~10000ms)
- **Events**: MIDI note-ons triggered
- **Peak amplitude**: Highest sample value (0.0-2.0 range)
- **Average amplitude**: Mean absolute amplitude
- **Samples processed**: Total audio samples (48000 Hz × 10s = 480000)

---

## Audio Analysis

### Expected Amplitude Ranges

| Scenario | Peak Amplitude | Avg Amplitude |
|----------|----------------|---------------|
| Single Poke | 0.7-1.0 | 0.2-0.4 |
| Note Sequence | 0.6-0.9 | 0.3-0.5 |
| Damping Sweep | 0.7-1.0 | 0.2-0.4 |
| Velocity Ramp | 0.1-1.0 | 0.3-0.6 |
| Rapid Fire | 0.8-1.2 | 0.5-0.8 |
| Chaos | 0.5-1.2 | 0.4-0.7 |

**If values are consistently higher:**
- May indicate clipping or gain misconfiguration
- Check `MAX_AMPLITUDE_SCALE` in audio_synth.c

**If values are too low:**
- Check mode weights and master gain
- Verify poke strength calculation

---

## Recording Test Audio

### Hardware Setup

1. Connect Daisy Seed audio outputs to audio interface
2. Set interface to 48kHz sample rate
3. Enable monitoring in DAW

### Recommended Settings

- **Input gain**: Adjust for -12dB to -6dB peaks
- **Format**: 24-bit WAV or FLAC
- **Sample rate**: 48kHz (match Daisy output)

### Recording Procedure

```bash
# Upload test firmware
pio run -e daisy_seed_test --target upload

# Start recording in DAW

# Monitor serial output to track scenarios
pio device monitor -e daisy_seed_test

# Stop recording after all 12 scenarios complete (~2.5 minutes)
```

### Post-Processing

Each 10-second segment can be:
- Trimmed into separate files
- Analyzed spectrally
- Compared against expected behavior
- Used as sound design material

---

## Customizing Tests

### Change Test Duration

Edit `test_scenarios.h`:
```cpp
#define TEST_DURATION_MS 5000  // 5 seconds instead of 10
```

### Add New Scenario

1. Add to enum in `test_scenarios.h`:
```cpp
typedef enum {
    ...
    TEST_SCENARIO_MY_TEST,
    TEST_SCENARIO_COUNT
} test_scenario_t;
```

2. Implement in `test_scenarios.cpp`:
```cpp
void test_scenario_my_test(test_runner_t* runner) {
    // Your test logic here
    if (runner->elapsed_ms % 500 == 0) {
        simulate_note_on(runner, 60, 100);
    }
}
```

3. Add to switch statement in `test_runner_update()`

### Change Modal Configuration

Edit `main_test.cpp` in `setup()`:
```cpp
// Example: Create bell-like resonator
modal_node_set_mode(&node, 0, freq_to_omega(523.25f), 0.3f, 1.0f);  // C5
modal_node_set_mode(&node, 1, freq_to_omega(659.26f), 0.4f, 0.7f);  // E5
```

---

## Troubleshooting

### No serial output
- Check USB connection
- Verify monitor speed: 115200 baud
- Ensure `hw.StartLog(true)` is called in setup()

### Tests not progressing
- Check LED blinks (3 = test start, 1 = inter-test delay)
- Verify test firmware uploaded (not normal main.cpp)
- Serial output should show test names

### Audio dropouts
- Reduce audio block size: `hw.SetAudioBlockSize(96);`
- Lower control rate: `CONTROL_PERIOD_MS = 4`
- Check CPU usage (should be <20%)

### Unexpected sounds
- Verify modal parameters (frequencies, damping, weights)
- Check amplitude ranges in serial output
- Compare against expected behavior in this guide

---

## Advanced: Regression Testing

### Create Reference Recordings

```bash
# Record test suite output
pio run -e daisy_seed_test --target upload
# [Record to test_suite_reference.wav]

# After code changes, record again
# [Record to test_suite_new.wav]

# Compare waveforms/spectra
# Should be identical for same test scenarios
```

### Automated Analysis (Python)

```python
import librosa
import numpy as np

# Load test recordings
ref, sr = librosa.load('test_suite_reference.wav', sr=48000)
new, sr = librosa.load('test_suite_new.wav', sr=48000)

# Compare peak amplitudes per segment
for i in range(12):  # 12 scenarios
    start = i * 12 * sr  # 12s per test (10s + 2s pause)
    end = start + 10 * sr

    ref_peak = np.max(np.abs(ref[start:end]))
    new_peak = np.max(np.abs(new[start:end]))

    diff = abs(ref_peak - new_peak)
    print(f"Scenario {i}: ref={ref_peak:.3f} new={new_peak:.3f} diff={diff:.3f}")
```

---

## Summary

The automated test suite provides:

✅ **Verification**: DSP correctness and stability
✅ **Documentation**: Audio examples of each parameter
✅ **Exploration**: Discover sonic possibilities
✅ **Regression**: Detect unintended changes
✅ **Performance**: Stress test under load

**Total test time**: ~2.5 minutes (12 scenarios × 10s + 2s pauses)

**No external MIDI controller needed** - all scenarios are fully automated!

---

## Next Steps

1. **Run test suite** and listen to all scenarios
2. **Record output** for reference
3. **Analyze serial logs** for performance metrics
4. **Customize tests** for your use case
5. **Add new scenarios** as needed

Happy testing! 🎵
