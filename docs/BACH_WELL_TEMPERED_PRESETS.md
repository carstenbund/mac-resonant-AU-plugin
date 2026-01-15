# Bach Well-Tempered Traditional Character Presets

## Overview

Four new character presets based on historical temperament principles and Bach's contrapuntal practice. These presets encode "well-tempered" thinking—not pure harmonic ratios, but functional intervals with controlled impurity for expressive key color and motion.

## Character IDs

- **Character 15**: Bach-T1 Soprano
- **Character 16**: Bach-T2 Alto
- **Character 17**: Bach-T3 Tenor
- **Character 18**: Bach-T4 Bass

## Design Philosophy

### Historical Context

Bach's practical reality was **well-tempered/circulating temperaments**, not just intonation:
- Fifths are nearly pure but slightly narrowed/widened
- Thirds are expressive, sometimes sweet, sometimes gritty
- **Key character matters** (g minor has a specific gravity)
- Controlled impurity generates motion and expression

### Implementation Approach

Each character uses a clear harmonic scaffold (fundamental, fifth, third, octave regions) with small **temperament offsets** per micro-voice:

- **±2 cents**: Subtle historical realism
- **±6 cents**: Expressive human friction
- **±12 cents**: Audible tension for mechanical brilliance

Not random—chosen by role and voice function.

---

## Character Specifications

### Character 15: Bach-T1 Soprano

**Role**: Anchor voice, bright but disciplined

**Frequency Multipliers** (with temperament offsets):
```
v0: 1.000000 (1/1, 0 cents)    - CORE line truth
v1: 1.498275 (3/2, -2 cents)   - BODY fifth light (historically narrowed)
v2: 2.001154 (2/1, +1 cent)    - EDGE octave lift (slight brilliance)
v3: 1.204163 (6/5, +6 cents)   - AURA minor third tint (expressive color)
```

**Damping**: `[0.4, 0.5, 0.6, 0.8]`
**Weights**: `[1.00, 0.45, 0.20, 0.12]`
**Personality**: Resonator

**Recommended Wave Shapes**:
- v0: Sine (pure subject line)
- v1: Sine (clear fifth support)
- v2: Triangle (bright octave projection)
- v3: Sine (subtle temperament shimmer)

**Result**: Subject stays readable but gains living temperament character.

---

### Character 16: Bach-T2 Alto

**Role**: Key color + voice-like midrange

**Frequency Multipliers**:
```
v0: 1.000000 (1/1, 0 cents)    - CORE
v1: 1.202776 (6/5, +4 cents)   - BODY minor third (not pure—expressive)
v2: 1.331029 (4/3, -3 cents)   - EDGE fourth (suspension architecture)
v3: 1.501733 (3/2, +2 cents)   - AURA fifth (nasal reed quality)
```

**Damping**: `[0.5, 0.6, 0.7, 0.8]`
**Weights**: `[1.00, 0.55, 0.25, 0.18]`
**Personality**: Resonator

**Recommended Wave Shapes**:
- v0: Triangle (voice-like core)
- v1: Triangle (expressive third)
- v2: Sine (suspension support)
- v3: Pulse 25% (nasal reed character)

**Result**: Traditional but distinctly colored, like a baroque reed stop.

---

### Character 17: Bach-T3 Tenor

**Role**: Engine room with controlled grit

**Frequency Multipliers**:
```
v0: 1.000000 (1/1, 0 cents)    - CORE
v1: 1.493085 (3/2, -8 cents)   - BODY fifth (stressed, audible tension)
v2: 1.257223 (5/4, +10 cents)  - EDGE major third (ambiguity shadow)
v3: 1.994312 (2/1, -5 cents)   - AURA octave mist/shimmer
```

**Damping**: `[0.5, 0.7, 0.9, 0.8]`
**Weights**: `[1.00, 0.35, 0.20, 0.12]`
**Personality**: Resonator

**Recommended Wave Shapes**:
- v0: Sine (stable foundation)
- v1: Triangle (controlled fifth stress)
- v2: Sawtooth (quiet! shadow ambiguity)
- v3: Pulse 10% (octave shimmer)

**Result**: Tenor "moves the air" around the piece—mechanical brilliance without sci-fi.

---

### Character 18: Bach-T4 Bass

**Role**: Lawful temperament, organ foundation

**Frequency Multipliers**:
```
v0: 0.500000 (1/2, 0 cents)    - CORE gravity (sub-octave foundation)
v1: 1.000000 (1/1, 0 cents)    - CORE fundamental (pure anchor)
v2: 2.002310 (2/1, +2 cents)   - BODY octave reinforcement (slight life)
v3: 3.995400 (4/1, -2 cents)   - AURA double octave (architecture)
```

**Damping**: `[0.3, 0.4, 0.5, 0.7]`
**Weights**: `[0.70, 1.00, 0.35, 0.18]`
**Personality**: Resonator

**Recommended Wave Shapes**:
- v0: Square (sub-bass gravity)
- v1: Sine (pure fundamental pillar)
- v2: Triangle (octave reinforcement)
- v3: Sine (architectural overtone)

**Result**: Strong baroque organ pillar—living but not sterile.

---

## Usage Recommendations

### Four-Voice Fugue Setup

Assign one character per node (0-3) for a traditional SATB layout:
- **Node 0**: Bach-T1 Soprano (Character 15)
- **Node 1**: Bach-T2 Alto (Character 16)
- **Node 2**: Bach-T3 Tenor (Character 17)
- **Node 3**: Bach-T4 Bass (Character 18)
- **Node 4**: (Optional 5th voice - use Soprano or Alto)

### Routing Mode

**Pitch Zones** recommended for SATB voicing:
- Soprano: High register (C4-C6)
- Alto: Mid-high (G3-G5)
- Tenor: Mid-low (C3-C5)
- Bass: Low (E2-E4)

**Round Robin** for distributed counterpoint with equal voice treatment.

### Coupling Settings

- **Topology**: Ring (sequential voice coupling) or Small-World (Bach-like voice leading)
- **Coupling Strength**: 0.2-0.4 (subtle influence without dominance)

### Dynamic Temperament Offsets (Advanced)

Vary the characters by section for psychological arc:

**Exposition**:
- Use pure characters (minimal offsets)
- Wave shapes: mostly sine/triangle

**Episode/Development**:
- Switch to full Bach characters (offsets active)
- Wave shapes: allow pulse/saw in inner voices

**Closing/Recapitulation**:
- Return to purer ratios or decrease coupling
- Return to "lawful" sine-dominant textures

This mimics: **order → wandering → return**

---

## Wave Shape Integration

**Important**: Wave shape parameters are currently **global** (not per-character). The recommended wave shapes above describe the *intended sonic design*, but must be set manually at the plugin level.

### Global Wave Shape Parameter

**Parameter ID**: `kParam_WaveShape` (27)
**Values**:
- 0 = Sine
- 1 = Sawtooth
- 2 = Triangle
- 3 = Square
- 4 = Pulse 25%
- 5 = Pulse 10%

**Parameter ID**: `kParam_PulseWidth` (28)
**Range**: 0.01-0.99 (0.5 = square wave)

### Recommended Global Settings

For **traditional/fugal** textures:
- **Wave Shape**: 0 (Sine) or 2 (Triangle)
- **Pulse Width**: 0.5 (if using pulse shapes)

For **character-rich** textures:
- Experiment with Pulse 25% for reed-like quality
- Use Triangle for vocal character

For **aggressive/modern** textures:
- Sawtooth at low weight for brightness
- Square for aggressive bass foundation

---

## Technical Notes

### Cent Offset Calculations

Frequency multipliers are calculated as:
```
freq_mult = target_ratio × 2^(cents/1200)
```

Examples:
- 3/2 with -2 cents: `1.5 × 2^(-2/1200) = 1.498275`
- 6/5 with +6 cents: `1.2 × 2^(6/1200) = 1.204163`

### Why These Offsets Work

1. **Functional intervals, not overtone purity**
   Uses fifths, thirds, octaves, fourths (not harmonic series)

2. **Controlled impurity as expressiveness**
   Small cent offsets = key color and motion
   Not "random detune" but temperament pressure

3. **Preserves counterpoint readability**
   Subject line stays sine-dominant and stable
   Complexity lives in alto/tenor inner machinery

---

## Validation

All characters pass validation checks:
- ✅ Frequency multipliers: 0.1-20.0
- ✅ Damping: 0.01-10.0
- ✅ Weights: 0.0-1.0
- ✅ Poke strength: 0.0-1.0
- ✅ Poke duration: 1.0-50.0 ms
- ✅ Coupling response: 0.1-2.0

---

## Future Enhancements

### Per-Character Wave Shapes

Currently wave shapes are global. Future enhancement could add per-character or per-mode wave shape selection for:
- Soprano: Pure sine fundamental
- Alto: Triangle with pulse aura
- Tenor: Mixed saw/triangle
- Bass: Square sub-bass with sine fundamental

This would fully realize the intended sonic architecture.

### Character Evolution

Add parameter automation to vary:
- Temperament offsets by section
- Wave shape morphing
- Dynamic coupling response

---

## References

- Well-tempered systems: Werckmeister III, Kirnberger III
- Bach's key characteristics: Mattheson's "Affektenlehre"
- Modal synthesis: Julius O. Smith III, "Physical Audio Signal Processing"
- Historical temperaments: Owen Jorgensen, "Tuning"

---

**Character IDs**: 15-18
**Added**: January 2026
**Version**: 1.0
