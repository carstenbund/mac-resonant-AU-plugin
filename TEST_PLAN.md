# Modal Attractors - Test Plan

## Overview

Comprehensive test suite to validate all aspects of the modal synthesis system before AU plugin deployment.

## Test Cases

### Test 1: Single Voice - Excitation & Decay
**File:** `test_01_single_voice_decay.wav`

**Purpose:** Validate basic modal synthesis behavior

**Configuration:**
- 1 voice, 4 modes
- Mode 0: 220 Hz, damping 0.5, weight 1.0
- Mode 1: 222.2 Hz, damping 0.6, weight 0.7 (slight detune)
- Mode 2: 440 Hz, damping 0.8, weight 0.5 (octave)
- Mode 3: 660 Hz, damping 1.0, weight 0.3 (5th)
- Duration: 5 seconds
- Personality: Resonator

**Expected Result:**
- ✅ Clean exponential decay
- ✅ 2 Hz beating from mode 0/1 detuning
- ✅ Harmonics at 440 Hz and 660 Hz visible
- ✅ No clicks or discontinuities
- ✅ RMS decreases smoothly over time

**Validation:**
```bash
python3 check_audio.py test_01_single_voice_decay.wav
```
- Should show smooth decay envelope
- Max jump < 0.01
- No large discontinuities
- Decay rate 85-95%

---

### Test 2: Multiple Excitations
**File:** `test_02_multiple_excitations.wav`

**Purpose:** Test repeated triggering and pitch changes

**Configuration:**
- 8 excitations at 0.5s intervals
- C major scale (C4-C5)
- Lower damping (0.3) for longer ring
- Duration: 6 seconds

**Expected Result:**
- ✅ 8 distinct attack transients
- ✅ Pitch changes audible
- ✅ Natural overlap/decay between notes
- ✅ No clicks at note transitions

**Validation:**
- Visual: 8 amplitude peaks in waveform
- Audio: Clear melodic sequence
- Analysis: 8 energy bursts at correct intervals

---

### Test 3: Polyphony (4 voices)
**File:** `test_03_polyphony_chord.wav`

**Purpose:** Validate simultaneous voice rendering

**Configuration:**
- 4 voices playing simultaneously
- C major chord: C4 (60), E4 (64), G4 (67), C5 (72)
- Different velocities: 0.8, 0.7, 0.6, 0.5
- Duration: 5 seconds

**Expected Result:**
- ✅ Rich harmonic content (chord)
- ✅ All 4 fundamental frequencies present
- ✅ Natural beating between close harmonics
- ✅ Balanced mix (no single voice dominates)

**Validation:**
- FFT: Peaks at 261.6, 329.6, 392.0, 523.3 Hz
- RMS higher than single voice
- No clipping
- Smooth decay

---

### Test 4: Topology Comparison
**Files:** `test_04a-d_topology_*.wav`

**Purpose:** Compare network coupling behaviors

**Configurations:**
- 4a: Ring topology
- 4b: Small-world topology
- 4c: Complete graph topology
- 4d: No coupling (baseline)

**Setup:**
- 3 voices: C4, E4, G4
- Medium coupling strength: 0.5
- Duration: 4 seconds each

**Expected Results:**
- ✅ 4a (Ring): Localized coupling, slower energy spread
- ✅ 4b (Small-world): Moderate spreading
- ✅ 4c (Complete): Fastest energy redistribution
- ✅ 4d (None): Independent decay, reference

**Validation:**
- Compare decay rates
- Analyze energy distribution over time
- 4c should have most uniform decay
- 4d should show independent voices

---

### Test 5: MIDI Scale Response
**File:** `test_05_midi_scale.wav`

**Purpose:** Test MIDI note tracking accuracy

**Configuration:**
- C major scale up and down (15 notes)
- 0.5s per note
- Voice allocation (4-voice polyphony)
- Duration: 8 seconds

**Expected Result:**
- ✅ Clear melodic sequence
- ✅ Accurate pitch for each note
- ✅ Smooth note transitions
- ✅ Voice stealing works correctly (>4 simultaneous notes)

**Validation:**
- Pitch detection confirms correct frequencies
- No pitch glitches or artifacts
- Natural overlap at transitions

---

### Test 6: Multichannel (8 voices, 8 channels)
**File:** `test_06_multichannel_8voices.wav`

**Purpose:** Test per-voice routing (multi-output capability)

**Configuration:**
- 8 separate voices
- 8 output channels (one per voice)
- Sequential triggering (0.1s intervals)
- Notes: C4-C5 scale
- Duration: 5 seconds

**Expected Result:**
- ✅ Each channel has isolated voice
- ✅ No cross-talk between channels
- ✅ Sequential "drum roll" effect
- ✅ Clear spatial separation

**Validation:**
- Import into DAW with 8 tracks
- Verify each channel independent
- Check timing of triggers
- Visual: staircase pattern in multi-track view

**Use Cases:**
- Multi-speaker installations
- Surround sound mapping
- Individual voice processing in DAW

---

### Test 7: Parameter Sweep (Damping)
**File:** `test_07_damping_sweep.wav`

**Purpose:** Validate parameter control and ranges

**Configuration:**
- 5 segments, 2 seconds each
- Damping values: 0.1, 0.3, 0.5, 0.8, 1.5
- Constant frequency: 440 Hz (A4)
- Duration: 10 seconds

**Expected Result:**
- ✅ Segment 1 (γ=0.1): Very long decay (~10s)
- ✅ Segment 2 (γ=0.3): Medium decay (~3s)
- ✅ Segment 3 (γ=0.5): Fast decay (~1s)
- ✅ Segment 4 (γ=0.8): Very fast decay (~0.5s)
- ✅ Segment 5 (γ=1.5): Nearly immediate decay

**Validation:**
- Measure decay time (t to -20dB) for each segment
- Should follow exponential: T₆₀ ≈ 13.8/γ
- Visual: Clearly different envelope shapes

---

## Running Tests

### Quick Test
```bash
cd build
./test_suite
```

### Full Test with Analysis
```bash
./run_tests.sh
```

This will:
1. Build test suite if needed
2. Run all 7 test cases
3. Generate WAV files
4. Analyze each output with check_audio.py
5. Display summary report

### Manual Analysis

**Waveform Inspection:**
```bash
# Open in audio editor
open build/test_*.wav

# Or in Audacity/Logic/etc for detailed analysis
```

**Spectral Analysis:**
```bash
# Generate spectrogram (requires ffmpeg)
ffmpeg -i build/test_01_single_voice_decay.wav \
       -lavfi showspectrumpic=s=1280x720 \
       test_01_spectrum.png
```

**Automated Checks:**
```bash
# Check all tests
cd build
for f in test_*.wav; do
    echo "=== $f ==="
    python3 ../check_audio.py "$f"
done
```

---

## Success Criteria

### Critical (Must Pass)
- [ ] No audio discontinuities (max jump < 0.01)
- [ ] No clipping (peak < 1.0)
- [ ] Smooth exponential decay
- [ ] Correct pitch tracking
- [ ] No DC offset (< 0.001)

### Important (Should Pass)
- [ ] Proper polyphony (4+ voices)
- [ ] Clean note transitions
- [ ] Accurate parameter response
- [ ] Multichannel isolation

### Optional (Nice to Have)
- [ ] Topology differences audible
- [ ] Voice stealing works correctly
- [ ] Network coupling visible in spectrum

---

## Debugging Failed Tests

### Clicks/Pops Detected
**Problem:** Phase discontinuities or buffer issues
**Check:**
- Phase accumulator continuity
- Buffer initialization
- Modal state update timing

### Wrong Pitch
**Problem:** MIDI to frequency conversion
**Check:**
- midi_to_freq() implementation
- freq_to_omega() conversion
- Phase increment calculation

### No Decay
**Problem:** Modal integration or damping
**Check:**
- Damping parameter (gamma) applied correctly
- Exponential integration
- Control rate (500 Hz) working

### Clipping
**Problem:** Sum of modes exceeds limits
**Check:**
- Mode weights sum
- Per-mode amplitude scaling
- Output clamping logic

### Wrong Topology Behavior
**Problem:** Coupling matrix or update
**Check:**
- Topology generation
- Coupling input calculation
- applyCoupling() implementation

---

## Comparison with Python Reference

To validate against Python implementation:

1. **Generate Python reference:**
```python
# In Python environment
from src.network import ModalNetwork, NetworkParams
import numpy as np

params = NetworkParams(K=4, N=1, omega=[...], gamma=[...])
net = ModalNetwork(params)
net.a[0,:] = [0.1+0.1j, ...]  # Initial state

for i in range(5000):
    net.step()
    # Record modal state
```

2. **Compare outputs:**
- Modal amplitudes over time
- Decay rates
- Frequency content
- Network coupling effects

3. **Expected correlation:**
- Amplitude envelopes: >95% correlation
- Frequency peaks: <1 Hz error
- Decay rates: Within 10%

---

## Integration Testing Checklist

Before deploying to AU plugin:

- [ ] All 7 tests pass
- [ ] No audio artifacts
- [ ] Polyphony works (4+ voices)
- [ ] MIDI tracking accurate
- [ ] Parameters respond correctly
- [ ] Multichannel routing verified
- [ ] Topology switching works
- [ ] CPU usage acceptable (<5% per voice)
- [ ] No memory leaks (run with valgrind)
- [ ] Real-time safe (no malloc in audio thread)

---

## Next Steps After Tests Pass

1. **Profile performance:**
```bash
# macOS
instruments -t "Time Profiler" test_suite
```

2. **Test in AU plugin:**
- Load in Logic Pro X
- Play with MIDI keyboard
- Verify all parameters
- Test automation

3. **Stress test:**
- Many simultaneous notes
- Rapid note changes
- Parameter sweeps during playback
- Long sessions (hours)

4. **User testing:**
- Musical context
- Different genres
- Performance scenarios
- Production use cases
