# Physical Modal Presets

## Overview

The modal synthesis engine now includes 8 physically-derived modal presets based on real acoustic objects. These presets provide authentic timbres for bells, plates, glasses, bars, drums, and strings.

**Location**: The presets are available in two places:
- **UI Layer** (primary): `ModalAttractorsFramework/UI/Utilities/CharacterTemplates.swift` - accessible from the AU plugin UI
- **DSP Layer** (optional): `modal_node.c/h` - available for standalone C usage

Users will access these presets through the character selector in the AU plugin interface.

## Available Presets

### 0. Church Bell
**Description**: Western church bell with hum, fundamental, tierce, and quint

Classic church bell sound with the characteristic "minor third" timbre from the tierce partial.

**Mode Ratios**:
- Mode 0: 1.000× (Fundamental - strongest, longest decay)
- Mode 1: 1.190× (Tierce - minor third above, defines bell character)
- Mode 2: 1.500× (Quint - perfect fifth)
- Mode 3: 2.000× (Nominal - octave, decays faster)

**Best for**: Bell towers, carillons, tubular bells

---

### 1. Circular Plate
**Description**: Flat circular plate (cymbal, gong) - Rayleigh modes

Bright, metallic sound with inharmonic overtones characteristic of cymbals and gongs.

**Mode Ratios** (Rayleigh modes):
- Mode 0: 1.000× ((0,1) mode)
- Mode 1: 2.081× ((1,1) mode)
- Mode 2: 3.413× ((2,1) mode)
- Mode 3: 3.891× ((0,2) mode)

**Best for**: Cymbals, gongs, tam-tams, metallic percussion

---

### 2. Wine Glass
**Description**: Cylindrical shell resonance (wine glass rim)

Clear, ringing tone with near-harmonic overtones. Models the resonance when rubbing a wine glass rim.

**Mode Ratios** (cylindrical shell):
- Mode 0: 1.000× (Fundamental)
- Mode 1: 2.280× (First overtone)
- Mode 2: 3.650× (Second overtone)
- Mode 3: 5.130× (Third overtone)

**Best for**: Glass instruments, crystalline sounds, singing bowls

---

### 3. Free Bar
**Description**: Rectangular bar with free ends (chime, vibraphone)

Warm, mellow tone with widely-spaced partials. Classic sound of vibraphones and tubular chimes.

**Mode Ratios** (free-free boundary):
- Mode 0: 1.000× (Fundamental)
- Mode 1: 2.756× (Second mode)
- Mode 2: 5.404× (Third mode)
- Mode 3: 8.933× (Fourth mode)

**Best for**: Vibraphones, tubular bells, wind chimes

---

### 4. Tuned Bar
**Description**: Arch-tuned bar (marimba, xylophone) with harmonic overtones

Musical harmonic overtones achieved through arch tuning. Produces pitch-stable melodic tones.

**Mode Ratios** (tuned):
- Mode 0: 1.000× (Fundamental)
- Mode 1: 4.000× (2 octaves - tuned)
- Mode 2: 10.00× (~3 octaves + fifth)
- Mode 3: 18.00× (~4 octaves)

**Best for**: Marimbas, xylophones, kalimbas, melodic mallet instruments

---

### 5. Drum Membrane
**Description**: Circular membrane (kettledrum, timpani)

Low, resonant with closely-spaced overtones. Characteristic of tuned drums.

**Mode Ratios** (circular membrane):
- Mode 0: 1.000× ((0,1) fundamental)
- Mode 1: 1.593× ((1,1) mode)
- Mode 2: 2.136× ((2,1) mode)
- Mode 3: 2.296× ((0,2) mode)

**Best for**: Timpani, kettledrums, tabla, congas

---

### 6. Small Bell
**Description**: Small handbell or bicycle bell (higher inharmonicity)

Bright, high-pitched with fast attack. More stretched overtones than large bells.

**Mode Ratios**:
- Mode 0: 1.000× (Fundamental - shorter decay)
- Mode 1: 1.350× (Stretched minor third)
- Mode 2: 1.700× (Stretched fifth)
- Mode 3: 2.200× (Upper partial)

**Best for**: Handbells, bicycle bells, desk bells, small metallic percussion

---

### 7. Harmonic String
**Description**: Idealized string with perfect harmonic overtones

Perfect harmonic series for comparison. Use when you want traditional string-like tones.

**Mode Ratios**:
- Mode 0: 1.000× (Fundamental)
- Mode 1: 2.000× (Octave)
- Mode 2: 3.000× (Fifth above octave)
- Mode 3: 4.000× (Two octaves)

**Best for**: Strings, pads, harmonic drones, comparison reference

---

## Usage Example

### Swift UI (Primary Method)

The presets are available through the UI character selector:

```swift
import ModalAttractorsFramework

// Get all available presets
let presets = CharacterTemplates.all
print("Available presets: \(CharacterTemplates.names)")

// Apply a preset to the parameter store
let churchBell = CharacterTemplates.churchBell
churchBell.apply(to: parameterStore, nodeIndex: 0)

// Or apply by index
if let preset = CharacterTemplates.template(at: 15) {  // Church Bell (index 15)
    preset.apply(to: parameterStore, nodeIndex: 0)
}

// Access preset properties
print("Name: \(churchBell.name)")
print("Description: \(churchBell.description)")
print("Mode 1 ratio: \(churchBell.mode1.frequency)")  // 1.190 (tierce)
```

**In the AU Plugin UI:**
1. Open the Character Editor tab
2. Select a node (0-4)
3. Choose from the preset dropdown
4. The new physical presets appear at the bottom of the list:
   - Church Bell
   - Circular Plate
   - Wine Glass
   - Free Bar
   - Tuned Bar
   - Drum Membrane
   - Small Bell
   - Harmonic String

### C API (Optional - for standalone usage)

```c
#include "modal_node.h"

modal_node_t node;
modal_node_init(&node, 0, PERSONALITY_RESONATOR);

// Apply church bell preset at A4 (440 Hz), damping = 0.5
modal_node_apply_preset(&node, 0, 440.0f, 0.5f);

// List all available presets
uint8_t num_presets = modal_node_get_num_presets();
for (uint8_t i = 0; i < num_presets; i++) {
    const char* name = modal_node_get_preset_name(i);
    const char* desc = modal_node_get_preset_description(i);
    printf("%d. %s\n   %s\n\n", i, name, desc);
}
```

## Parameter Tuning Guide

### Fundamental Frequency
- **MIDI tracking**: Use MIDI note frequency directly
- **Fixed pitch**: Use a constant (e.g., 220 Hz for A3)
- **Detuned ensemble**: Apply small random offsets per voice

### Base Damping
- **Quick decay** (percussion): 0.8 - 2.0
- **Medium decay** (bells): 0.3 - 0.8
- **Long decay** (gongs): 0.1 - 0.3
- **Sustained** (singing bowls): 0.05 - 0.15

### Adjusting Individual Modes
After applying a preset, you can fine-tune individual modes:

```c
// Apply preset first
modal_node_apply_preset(&node, 1, 440.0f, 0.5f);  // Circular Plate

// Then adjust mode 3 (reduce high overtone weight)
modal_node_set_mode(&node, 3,
    node.modes[3].params.omega,  // Keep frequency
    node.modes[3].params.gamma,   // Keep damping
    0.15f);                       // Reduce weight
```

## References

The mode ratios are derived from:

1. **Fletcher & Rossing** (1998): *The Physics of Musical Instruments* (2nd ed.)
   - Chapter 10: Bells
   - Chapter 3: Bar vibrations

2. **Rayleigh** (1945): *The Theory of Sound* (2nd ed.)
   - Volume 1, Chapter 9: Vibrating plates
   - Volume 1, Chapter 8: Vibrating membranes

3. **Rossing** (2000): *Science of Percussion Instruments*
   - Chapters on bars, plates, and membranes

4. **Benade** (1976): *Fundamentals of Musical Acoustics*
   - Section on mode frequencies and overtone structures

## Next Steps

### Adding More Presets
To add new presets, edit `modal_node.c` and add entries to the `MODAL_PRESETS` array:

```c
{
    .name = "My Object",
    .description = "Description of object physics",
    .modes = {
        {freq_ratio_0, damping_ratio_0, weight_0},
        {freq_ratio_1, damping_ratio_1, weight_1},
        {freq_ratio_2, damping_ratio_2, weight_2},
        {freq_ratio_3, damping_ratio_3, weight_3},
    }
},
```

### UI Integration
Consider exposing presets in the AU parameter tree:

- Add a discrete parameter "Character" with 8 choices
- Map preset index to parameter value
- Call `modal_node_apply_preset()` when parameter changes
- Combine with damping/frequency controls for full expression

### Preset Interpolation (Advanced)
For smooth morphing between characters:

```c
void modal_node_apply_preset_blend(modal_node_t* node,
                                   uint8_t preset_a, uint8_t preset_b,
                                   float blend,  // 0.0 = preset_a, 1.0 = preset_b
                                   float fundamental_freq_hz,
                                   float base_damping);
```

This could enable smooth transitions between bell → plate → glass timbres.
