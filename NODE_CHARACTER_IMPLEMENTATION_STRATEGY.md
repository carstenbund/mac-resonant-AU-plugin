# Node Character System: Implementation Strategy

**Date:** January 9, 2026
**Branch:** claude/investigate-modal-attractors-IgNe1
**Based on:** User proposal digest

---

## Executive Summary

Transform ModalAttractors from **polyphonic voice pool** (up to 16 identical voices) into **5-node character network** (fixed nodes with distinct identities).

**Core Change:** Replace dynamic voice allocation with persistent node excitation.

---

## Current vs. Proposed Architecture

### Current System (ModalAttractorsExtension)

```
MIDI Note On → Find free voice or steal → Allocate voice
                ↓
          Voice pool (0-16)
          All use same mode parameters
                ↓
          Topology couples voices
```

**Characteristics:**
- Dynamic allocation (voices come and go)
- Voice stealing when polyphony exceeded
- Global mode parameters (all voices identical)
- Up to 16 simultaneous voices

### Proposed System (Node Character Network)

```
MIDI Note On → Route to node(s) → Excite node
                ↓
          5 Fixed Nodes (persistent)
          Node 0: Character A
          Node 1: Character B
          Node 2: Character C
          Node 3: Character D
          Node 4: Character E
                ↓
          Topology couples nodes
```

**Characteristics:**
- Fixed allocation (5 nodes always exist)
- No voice stealing (nodes are excited, not allocated)
- Per-node characters (each node has identity)
- Notes excite nodes based on routing strategy

---

## Key Design Questions

Before implementation, we need to decide:

### 1. Note Routing Strategy

**How do MIDI notes map to nodes?**

**Option A: Round-Robin**
```cpp
node_index = note_counter % 5;
```
- Simple, predictable
- Notes distributed evenly
- No pitch relationship

**Option B: Pitch-Based Zones**
```cpp
// MIDI range: 0-127
if (note < 36) node_index = 0;      // Bass zone
else if (note < 60) node_index = 1; // Low zone
else if (note < 72) node_index = 2; // Mid zone
else if (note < 96) node_index = 3; // High zone
else node_index = 4;                 // Top zone
```
- Predictable pitch→node mapping
- Characters can have pitch roles
- **Matches your "Vibrant Bass" concept**

**Option C: Broadcast (All Nodes)**
```cpp
for (int i = 0; i < 5; i++) {
    nodes[i]->excite(note, velocity);
}
```
- Full network excitation
- Complex coupling behavior
- Potentially overwhelming

**Option D: Velocity-Based**
```cpp
// Soft notes → lower nodes, hard notes → upper nodes
node_index = (int)(velocity * 4.99f);
```

**Option E: Configurable Mapping**
- Add parameter: "Note Routing Mode" (enum)
- User selects strategy

**QUESTION 1:** Which routing strategy do you want to start with?

### 2. Polyphony Semantics

**With 5 nodes, what happens when multiple notes are played?**

**Option A: Node Excitation (No Limit)**
- Node can be excited multiple times while still ringing
- Excitations accumulate (additive pokes)
- Natural network behavior
- **Most aligned with modal physics**

**Option B: Note Queuing**
- Each node tracks one "primary" note
- New note replaces old (re-trigger)
- Simpler, more predictable

**Option C: Note Stacking**
- Each node can hold multiple simultaneous notes
- Frequencies added/mixed
- Complex but powerful

**QUESTION 2:** Should nodes be re-excitable while ringing, or one-note-at-a-time?

### 3. Global vs. Character Parameters

**Current system has global mode parameters. How do they interact with characters?**

**Option A: Characters Override Globals (Proposed)**
- Character fully defines node parameters
- Global mode params ignored or removed
- Clean separation

**Option B: Global + Character Offset**
- Global params are base values
- Character applies offsets/multipliers
- More flexible, more complex

**Option C: Character Templates + Live Tweaking**
- Character sets initial values
- Global params can modify on top
- Best of both worlds, but UI complexity

**QUESTION 3:** Should character selection completely override mode parameters, or work with them?

### 4. Existing Polyphony Parameter

**Current system has `kParam_Polyphony` (1-32). What should happen to it?**

**Option A: Remove** - Node count is always 5

**Option B: Repurpose** - Becomes "Node Count" (1-5) for experimentation

**Option C: Keep Read-Only** - Shows "5" always

**QUESTION 4:** Keep/remove/repurpose polyphony parameter?

---

## Implementation Phases

### Phase 1: Core Architecture (Minimal Viable Character System)

**Goal:** Get 5 fixed nodes working with hardcoded characters

**Tasks:**

1. **NodeCharacter Struct**
   ```cpp
   // File: NodeCharacter.h
   struct NodeCharacter {
       // Mode parameters (4 modes × 3 params)
       float mode_freq_mult[4];
       float mode_damping[4];
       float mode_weight[4];

       // Voice parameters
       node_personality_t personality;

       // Excitation parameters
       float poke_strength;
       float poke_duration_ms;

       // Network behavior
       float coupling_response_gain;  // How strongly this node reacts to neighbors

       const char* name;
   };

   // Preset library
   extern const NodeCharacter CHARACTER_VIBRANT_BASS;
   extern const NodeCharacter CHARACTER_DARK_NODE;
   extern const NodeCharacter CHARACTER_BRIGHT_BELL;
   extern const NodeCharacter CHARACTER_GLASSY_SHIMMER;
   extern const NodeCharacter CHARACTER_DRONE_HUB;

   extern const NodeCharacter* CHARACTER_LIBRARY[5];
   ```

2. **Refactor VoiceAllocator → NodeManager**
   ```cpp
   class NodeManager {
   public:
       NodeManager();  // Always creates exactly 5 nodes

       void initialize(float sample_rate);

       // Node management
       void setNodeCharacter(uint8_t node_idx, uint8_t character_id);
       ModalVoice* getNode(uint8_t node_idx);  // Direct access to nodes

       // Note handling (no allocation/stealing)
       void exciteNode(uint8_t node_idx, uint8_t midi_note, float velocity);
       void releaseNode(uint8_t node_idx);
       void allNotesOff();

       // Rendering
       void updateNodes();
       void renderAudio(float* outL, float* outR, uint32_t num_frames);

   private:
       ModalVoice* nodes_[5];  // Fixed 5 nodes
       NodeCharacter current_characters_[5];  // Active character for each node

       uint32_t excitation_counter_;  // For round-robin routing
   };
   ```

3. **Character Application**
   ```cpp
   void NodeManager::setNodeCharacter(uint8_t node_idx, uint8_t character_id) {
       if (node_idx >= 5) return;
       if (character_id >= 5) return;

       const NodeCharacter* character = CHARACTER_LIBRARY[character_id];
       current_characters_[node_idx] = *character;

       ModalVoice* node = nodes_[node_idx];

       // Apply character to node
       float base_freq = 440.0f;  // Reference frequency
       for (uint8_t m = 0; m < 4; m++) {
           float freq = base_freq * character->mode_freq_mult[m];
           node->setMode(m, freq, character->mode_damping[m], character->mode_weight[m]);
       }

       node->setPersonality(character->personality);
       // Store poke params for use during excitation
   }
   ```

4. **Update SynthEngine**
   - Replace `VoiceAllocator* voiceAllocator_` with `NodeManager* nodeManager_`
   - Update `noteOn` handling to route to nodes (use selected strategy)
   - Remove voice stealing logic

5. **Add Character Parameters**
   ```cpp
   // In ModalAttractorsExtensionParameterAddresses.h
   enum {
       // ... existing params ...
       kParam_Node0_Character = 20,
       kParam_Node1_Character = 21,
       kParam_Node2_Character = 22,
       kParam_Node3_Character = 23,
       kParam_Node4_Character = 24,
       kNumParams = 25
   };
   ```

6. **Update TopologyEngine**
   - Change `num_voices_` → always 5
   - Remove dynamic node count logic
   - Simplify coupling matrix to fixed 5×5

**Deliverable:** 5 nodes with hardcoded characters, basic note routing working

**Estimated Complexity:** Medium (3-5 hours)

---

### Phase 2: Character Library & Parameters

**Goal:** Expose character selection via parameters, define initial character library

**Tasks:**

1. **Define Initial Characters**
   ```cpp
   // NodeCharacter.cpp

   const NodeCharacter CHARACTER_VIBRANT_BASS = {
       .mode_freq_mult = {1.0f, 2.0f, 3.0f, 5.0f},    // Strong harmonics
       .mode_damping = {0.3f, 0.5f, 0.8f, 1.2f},      // Low damping
       .mode_weight = {1.0f, 0.8f, 0.6f, 0.4f},       // Fundamental-focused
       .personality = PERSONALITY_RESONATOR,
       .poke_strength = 0.7f,
       .poke_duration_ms = 15.0f,
       .coupling_response_gain = 0.8f,
       .name = "Vibrant Bass"
   };

   const NodeCharacter CHARACTER_DARK_NODE = {
       .mode_freq_mult = {1.0f, 1.5f, 2.2f, 3.1f},    // Subdued upper
       .mode_damping = {0.8f, 1.2f, 1.8f, 2.5f},      // Strong damping
       .mode_weight = {0.8f, 0.4f, 0.2f, 0.1f},       // Dark timbre
       .personality = PERSONALITY_RESONATOR,
       .poke_strength = 0.4f,
       .poke_duration_ms = 8.0f,
       .coupling_response_gain = 0.5f,                 // Absorptive
       .name = "Dark Node"
   };

   const NodeCharacter CHARACTER_BRIGHT_BELL = {
       .mode_freq_mult = {1.0f, 2.76f, 5.40f, 8.93f}, // Inharmonic (bell ratios)
       .mode_damping = {0.4f, 0.6f, 0.5f, 0.7f},      // Mixed decay
       .mode_weight = {0.7f, 0.9f, 1.0f, 0.8f},       // Bright upper
       .personality = PERSONALITY_RESONATOR,
       .poke_strength = 0.6f,
       .poke_duration_ms = 5.0f,                       // Sharp attack
       .coupling_response_gain = 1.0f,
       .name = "Bright Bell"
   };

   const NodeCharacter CHARACTER_GLASSY_SHIMMER = {
       .mode_freq_mult = {1.0f, 2.01f, 4.03f, 11.2f}, // Near-harmonic + high
       .mode_damping = {0.5f, 0.6f, 0.7f, 0.4f},      // Shimmering high
       .mode_weight = {0.6f, 0.7f, 0.6f, 0.9f},       // Emphasize high mode
       .personality = PERSONALITY_RESONATOR,
       .poke_strength = 0.5f,
       .poke_duration_ms = 12.0f,
       .coupling_response_gain = 0.9f,
       .name = "Glassy Shimmer"
   };

   const NodeCharacter CHARACTER_DRONE_HUB = {
       .mode_freq_mult = {1.0f, 1.002f, 1.498f, 2.0f}, // Near-unison low
       .mode_damping = {0.1f, 0.15f, 0.2f, 0.3f},      // Very low damping
       .mode_weight = {1.0f, 0.9f, 0.7f, 0.5f},        // Rich low spectrum
       .personality = PERSONALITY_SELF_OSCILLATOR,     // Sustains
       .poke_strength = 0.3f,                          // Gentle
       .poke_duration_ms = 20.0f,
       .coupling_response_gain = 1.2f,                  // Strong coupling response
       .name = "Drone Hub"
   };

   const NodeCharacter* CHARACTER_LIBRARY[5] = {
       &CHARACTER_VIBRANT_BASS,
       &CHARACTER_DARK_NODE,
       &CHARACTER_BRIGHT_BELL,
       &CHARACTER_GLASSY_SHIMMER,
       &CHARACTER_DRONE_HUB
   };
   ```

2. **Wire Parameters to Engine**
   - Add character parameters to Parameters.swift
   - Handle parameter changes in SynthEngine::setParameter()
   - Update character selection calls NodeManager::setNodeCharacter()

3. **Update UI (SwiftUI)**
   - Add 5 character pickers (one per node)
   - Remove or hide global mode parameter controls
   - Optional: Visual representation of 5-node network

**Deliverable:** User-selectable characters via AU parameters

**Estimated Complexity:** Medium (2-3 hours)

---

### Phase 3: Advanced Routing & Interaction

**Goal:** Implement note routing strategies, refine coupling behavior

**Tasks:**

1. **Note Routing Modes**
   - Implement 2-3 routing strategies
   - Add parameter: kParam_NoteRouting (enum)
   - Options: RoundRobin, PitchZones, Broadcast

2. **Per-Node Coupling Gain**
   - Update TopologyEngine to use character's coupling_response_gain
   - Scale coupling inputs by this gain
   - Allows "hub" vs "absorber" roles

3. **Note-Off Handling**
   - Decide: release node immediately, or let it ring?
   - Implement chosen strategy

4. **Excitation Accumulation**
   - If node excited while ringing, add excitation
   - Test stability (prevent runaway)

**Deliverable:** Flexible note routing, stable multi-excitation

**Estimated Complexity:** Medium (3-4 hours)

---

### Phase 4: Polish & Testing

**Goal:** Refinement, documentation, testing

**Tasks:**

1. **Character Tuning**
   - Listen to each character in isolation
   - Test characters in different topologies
   - Adjust ratios/damping for musicality

2. **Stability Testing**
   - Self-oscillator nodes in Complete topology
   - Rapid note input (stress test)
   - Coupling feedback prevention

3. **Documentation**
   - Update USER_GUIDE with character descriptions
   - Document routing strategies
   - Add example patches

4. **Preset System** (Optional)
   - Save/load character configurations
   - Share presets

**Deliverable:** Polished, stable, documented system

**Estimated Complexity:** Medium (2-3 hours)

---

## Architecture Considerations

### What Stays the Same

✅ **Modal oscillator core** (`modal_node.c`) - no changes needed
✅ **Topology engine** - just fix node count to 5
✅ **Pitch bend** - still applies to all active nodes
✅ **Coupling mechanism** - mode-0 driven, same math
✅ **Audio rendering** - same mixing approach

### What Changes

❌ **VoiceAllocator** → NodeManager (fixed 5 nodes)
❌ **Dynamic allocation** → Persistent excitation
❌ **Voice stealing** → None (nodes always exist)
❌ **Global mode params** → Per-node characters
❌ **Polyphony concept** → Node count (5)

### What Gets Added

➕ **NodeCharacter struct** - parameter bundles
➕ **Character library** - preset definitions
➕ **Character selection parameters** (×5)
➕ **Note routing strategies**
➕ **Per-node coupling gain**

---

## Parameter Changes

### Parameters to Remove/Repurpose

| Current Parameter | Action |
|-------------------|--------|
| kParam_Mode0_Frequency | Remove (per-node now) |
| kParam_Mode0_Damping | Remove |
| kParam_Mode0_Weight | Remove |
| kParam_Mode1_* | Remove |
| kParam_Mode2_* | Remove |
| kParam_Mode3_* | Remove |
| kParam_PokeStrength | Remove (per-character) |
| kParam_PokeDuration | Remove (per-character) |
| kParam_Polyphony | Repurpose or remove |
| kParam_NodeCount | Remove (always 5) |

**Total removed: ~15 parameters**

### Parameters to Add

| New Parameter | Range | Default |
|---------------|-------|---------|
| kParam_Node0_Character | 0-4 | 0 (Vibrant Bass) |
| kParam_Node1_Character | 0-4 | 1 (Dark Node) |
| kParam_Node2_Character | 0-4 | 2 (Bright Bell) |
| kParam_Node3_Character | 0-4 | 3 (Glassy Shimmer) |
| kParam_Node4_Character | 0-4 | 4 (Drone Hub) |
| kParam_NoteRouting | 0-2 | 0 (Round-Robin) |

**Total added: 6 parameters**

### Parameters That Stay

| Parameter | Purpose |
|-----------|---------|
| kParam_MasterGain | Global output level |
| kParam_CouplingStrength | Global coupling scale |
| kParam_Topology | Network structure |
| kParam_Personality | Optional global override |

**Net change: -9 parameters** (20 → 11)

**Result: Simpler, more focused UI**

---

## Code Impact Estimate

### Files to Create

- `ModalAttractorsExtension/DSP/NodeCharacter.h` (~100 lines)
- `ModalAttractorsExtension/DSP/NodeCharacter.cpp` (~150 lines)
- `ModalAttractorsExtension/DSP/NodeManager.h` (~120 lines)
- `ModalAttractorsExtension/DSP/NodeManager.cpp` (~300 lines)

**Total new code: ~670 lines**

### Files to Modify

- `SynthEngine.h` (~30 line changes)
- `SynthEngine.cpp` (~80 line changes)
- `TopologyEngine.h` (~20 line changes)
- `TopologyEngine.cpp` (~40 line changes)
- `ModalAttractorsExtensionParameterAddresses.h` (~30 line changes)
- `Parameters.swift` (~100 line changes)
- `ModalAttractorsExtensionMainView.swift` (~150 line changes)

**Total modifications: ~450 lines**

### Files to Deprecate

- `VoiceAllocator.h` (move to NodeManager)
- `VoiceAllocator.cpp`

**Total deprecated: ~400 lines**

**Net code change: +720 lines**

---

## Risk Assessment

### Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Coupling instability with self-oscillators | Medium | High | Limit coupling_response_gain, test thoroughly |
| Excitation accumulation runaway | Low | High | Clamp mode amplitudes, test rapid notes |
| Character library not musical | Medium | Medium | Iterate based on listening tests |
| Note routing confusing | Low | Medium | Start with simple round-robin |

### Compatibility Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Breaks existing presets | High | Version migration, or accept breaking change |
| Parameter IDs change | Medium | Careful remapping |
| User confusion | Medium | Clear documentation, migration guide |

---

## Testing Strategy

### Unit Tests

1. **NodeCharacter application**
   ```cpp
   TEST(NodeManager, ApplyCharacter) {
       NodeManager mgr;
       mgr.initialize(48000.0f);
       mgr.setNodeCharacter(0, 0);  // Vibrant Bass

       ModalVoice* node = mgr.getNode(0);
       // Verify mode parameters match character
   }
   ```

2. **Note routing**
   ```cpp
   TEST(NodeManager, RoundRobinRouting) {
       // Excite 7 notes, verify round-robin pattern
   }
   ```

3. **Coupling with per-node gains**
   ```cpp
   TEST(TopologyEngine, PerNodeCouplingGain) {
       // Verify coupling_response_gain scales inputs correctly
   }
   ```

### Integration Tests

1. **5-node network rendering**
   - Play chord, verify audio output
   - Check all 5 nodes contribute

2. **Character switching while playing**
   - Excite node
   - Change character
   - Verify smooth transition

3. **Self-oscillator stability**
   - Set all nodes to Drone Hub
   - Complete topology
   - Verify no runaway

### Musical Tests

1. **Character identity**
   - Play each character solo
   - Verify distinct sonic personality

2. **Network interaction**
   - Test different topology + character combinations
   - Verify emergent behavior

3. **Note routing feel**
   - Test round-robin vs pitch zones
   - Verify playability

---

## Open Questions for User

Before proceeding with implementation, please confirm:

### 1. Note Routing Strategy (Priority: HIGH)
**Question:** Which routing mode should be the default?
- **A)** Round-robin (simple, even distribution)
- **B)** Pitch zones (bass notes → node 0, etc.)
- **C)** Broadcast (all nodes always excited)
- **D)** Make it a parameter (selectable)

**Recommendation:** Start with round-robin, add parameter later.

### 2. Polyphony Semantics (Priority: HIGH)
**Question:** Can a node be excited multiple times while ringing?
- **A)** Yes, excitations accumulate (additive pokes)
- **B)** No, new note replaces old (re-trigger)

**Recommendation:** Option A (accumulate) - more aligned with physical systems.

### 3. Global Parameters (Priority: MEDIUM)
**Question:** What happens to existing global mode parameters?
- **A)** Remove completely (characters fully control)
- **B)** Keep as hidden/advanced (character + offset)
- **C)** Keep visible (user can tweak on top of character)

**Recommendation:** Option A (remove) - cleaner UX, matches proposal.

### 4. Existing Presets (Priority: LOW)
**Question:** Handle compatibility with existing presets?
- **A)** Breaking change (accept loss)
- **B)** Migration logic (map old params to characters)
- **C)** Dual mode (toggle between old/new system)

**Recommendation:** Option A - clean break, early enough in development.

### 5. Character Library Expansion (Priority: LOW)
**Question:** How should users create custom characters?
- **A)** Fixed 5 characters (no customization)
- **B)** Preset save/load (later feature)
- **C)** Full character editor in UI (complex)

**Recommendation:** Start with A, add B later if desired.

---

## Implementation Priority: My Recommendation

**Phase 1A (Do First):** Core architecture + hardcoded characters
- Create NodeCharacter struct
- Implement NodeManager with 5 fixed nodes
- Apply hardcoded characters (no parameters yet)
- Implement simple round-robin routing
- Update SynthEngine to use NodeManager
- **Goal:** Prove the concept works

**Phase 1B:** Character parameters
- Add 5 character selection parameters
- Wire to NodeManager
- Remove old mode parameters
- **Goal:** User-controllable characters

**Phase 2 (Do Second):** Polish
- Tune character library (listening tests)
- Stability testing
- Documentation
- **Goal:** Musical and stable

**Phase 3 (Optional):** Advanced features
- Note routing modes
- Per-node level trim
- Character intensity slider
- **Goal:** Flexibility

---

## Next Steps

To proceed, I need your answers to the 5 open questions above.

Then I can:
1. ✅ Create NodeCharacter.h/cpp with initial library
2. ✅ Refactor VoiceAllocator → NodeManager
3. ✅ Update SynthEngine to use NodeManager
4. ✅ Add character parameters
5. ✅ Test and iterate

**Estimated total implementation time:** 10-15 hours (spread across phases)

---

**Ready to begin implementation once you confirm design decisions.**
