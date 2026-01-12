# ModalEffect Parameter Calibration Guide

## Parameter Mappings

### 1. Body Size (0.0 - 1.0)
**What it controls**: Base resonator frequency/pitch

**Mapping**:
- 0.0 = C2 (MIDI note 36, ~65 Hz) - Large resonant body (bass)
- 0.5 = C4 (MIDI note 60, ~262 Hz) - Medium body (default)
- 1.0 = C6 (MIDI note 96, ~1047 Hz) - Small body (high pitch)

**Test procedure**:
1. Set Mix = 100%, Excite = 0.5, Morph = 0, Material = 0.5
2. Play steady percussive input (kick drum, click track)
3. Sweep Body Size from 0 to 1
4. **Expected**: Pitch should sweep from low bass to high bell tones
5. **Calibration**: Verify pitch at 0.0, 0.5, 1.0 matches expected MIDI notes

### 2. Material (0.0 - 1.0)
**What it controls**: Damping/resonance time (NOT YET IMPLEMENTED)

**Target mapping**:
- 0.0 = High damping (short decay, dead sound)
- 0.5 = Medium damping (natural decay)
- 1.0 = Low damping (long ringing, bell-like)

**Test procedure** (once implemented):
1. Set Mix = 100%, Excite = 0.5, Morph = 0, Body Size = 0.5
2. Trigger single transient
3. Sweep Material from 0 to 1
4. **Expected**: Decay time should increase from ~100ms to several seconds

**STATUS**: ⚠️ Parameter currently not connected to synthesis engine

### 3. Excite (0.0 - 1.0)
**What it controls**: Input sensitivity / trigger threshold

**Mapping**:
- 0.0 = No excitation (silent)
- 0.1 = Very high threshold (only loud transients trigger)
- 0.5 = Medium sensitivity (moderate transients trigger)
- 1.0 = Maximum sensitivity (soft inputs trigger resonance)

**Test procedure**:
1. Set Mix = 100%, Body Size = 0.5, Morph = 0, Material = 0.5
2. Play input with varying dynamics (soft to loud)
3. Sweep Excite from 0 to 1
4. **Expected**:
   - At 0.0: Complete silence
   - At 0.1-0.3: Only loud hits trigger
   - At 0.5-0.7: Most transients trigger
   - At 0.8-1.0: Even soft inputs trigger

**Current behavior**: Controls both threshold and velocity scaling

### 4. Morph (0.0 - 1.0)
**What it controls**: Pitch tracking amount

**Mapping**:
- 0.0 = Fixed pitch (uses Body Size parameter only)
- 0.5 = 50% blend between Body Size and detected input pitch
- 1.0 = Full pitch tracking (follows input pitch)

**Test procedure**:
1. Set Mix = 100%, Excite = 0.5, Body Size = 0.5, Material = 0.5
2. Play melodic input with clear pitch (voice, guitar, synth)
3. Set Morph = 0.0
4. **Expected**: Fixed pitch regardless of input
5. Set Morph = 1.0
6. **Expected**: Output pitch follows input pitch

**Pitch detection**: Zero-crossing rate method (100ms window)
- Range: 60 Hz - 2000 Hz
- Works best with: Monophonic input, clear pitch content
- May struggle with: Polyphonic input, noisy/percussive content

### 5. Mix (0.0 - 1.0)
**What it controls**: Dry/wet signal blend

**Mapping**:
- 0.0 = 100% dry (input passthrough)
- 0.5 = 50% dry + 50% wet
- 1.0 = 100% wet (only resonator output)

**Test procedure**:
1. Set Excite = 0.5, Body Size = 0.5
2. Play continuous input
3. Sweep Mix from 0 to 1
4. **Expected**: Smooth crossfade from dry to processed sound

## Calibration Test Sequence

### Test 1: Basic Excitation
```
Input: Kick drum or click track at ~80 BPM
Body Size: 0.5
Material: 0.5
Excite: Start at 0.0, increase to 1.0
Morph: 0.0
Mix: 1.0 (100% wet)

Expected: At Excite=0, complete silence. As Excite increases, resonator triggers on each transient.
```

### Test 2: Pitch Range (Body Size)
```
Input: Consistent transients (metronome, click)
Body Size: Sweep 0.0 → 0.5 → 1.0
Material: 0.5
Excite: 0.5
Morph: 0.0
Mix: 1.0

Expected: Pitch sweep from C2 (low) → C4 (mid) → C6 (high)
Measure: Record MIDI notes at 0.0, 0.25, 0.5, 0.75, 1.0
```

### Test 3: Pitch Tracking (Morph)
```
Input: Sine wave sweep or melodic phrase (100-500 Hz)
Body Size: 0.5 (C4 = 262 Hz)
Material: 0.5
Excite: 0.5
Morph: 0.0 → 0.5 → 1.0
Mix: 1.0

At Morph=0.0: Should output C4 regardless of input
At Morph=0.5: Should blend between C4 and input pitch
At Morph=1.0: Should track input pitch closely
```

### Test 4: Dry/Wet Balance
```
Input: Pink noise or full mix
Body Size: 0.5
Material: 0.5
Excite: 0.5
Morph: 0.0
Mix: Sweep 0.0 → 1.0

Expected: Smooth crossfade, no volume jumps
Verify: -6dB at Mix=0.5 (equal power crossfade check)
```

## Known Issues & TODO

1. **Material parameter not implemented** - Currently not connected to damping
   - TODO: Map material to global damping in NodeManager
   - Target: material → inversely to damping (1.0 = min damping)

2. **Pitch detection quality** - Zero-crossing method is simple but limited
   - Works poorly on: complex/polyphonic input, drums, noise
   - TODO: Consider autocorrelation or AMDF for better accuracy

3. **Scratchy sound** - Likely from rapid retriggering
   - Current fix: Auto note-off when energy drops
   - TODO: Add minimum inter-onset interval (IOI)

4. **No polyphonic tracking** - Only detects/plays one note at a time
   - This is by design for now (monophonic resonant body)

## Debugging Output

To verify parameter values are being received:
1. Add debug prints in `modal_attractors_engine_process()`:
   ```cpp
   printf("Params - Body:%.2f Mat:%.2f Exc:%.2f Morph:%.2f Mix:%.2f\n",
          bodySize, material, excite, morph, mix);
   ```

2. Check detected pitch:
   ```cpp
   printf("Detected: %.1f Hz (MIDI %d), Target: MIDI %d\n",
          detected_freq, detected_note, target_note);
   ```

3. Monitor onset detection:
   ```cpp
   if (noteOn triggered) {
       printf("Onset! Energy: %.4f, Velocity: %.2f, Note: %d\n",
              energy, velocity, target_note);
   }
   ```
