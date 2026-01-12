# Node-Based Architecture Refactoring Plan

## Current vs. Desired Architecture

### ❌ Current (Incorrect)
- MIDI allocates "voices"
- Polyphony = 16 voices
- Each voice = 1 modal oscillator
- Coupling between 16 entities

### ✅ Desired (Correct)
- MIDI allocates **nodes** (network identities)
- Polyphony = up to 16 nodes
- Each node = 4 internal modes/voices
- Coupling between nodes (not between individual modes)
- Total: 16 nodes × 4 modes = 64 internal oscillators

---

## Conceptual Model

```
MIDI Note On → Allocate Node → Node contains 4 Modes

Node[0] ←─ coupling ─→ Node[1]
  ├─ Mode 0             ├─ Mode 0
  ├─ Mode 1             ├─ Mode 1
  ├─ Mode 2             ├─ Mode 2
  └─ Mode 3             └─ Mode 3
```

**Key insight:** The network topology connects nodes, not individual modes.

---

## Refactoring Steps

### Step 1: Rename Core Concepts

**File:** `DSP/ModalVoice.h/.cpp`
- Rename `ModalVoice` → `ModalNode` (or keep name but update comments)
- **Current state:** This file represents ONE modal oscillator
- **Change needed:** Make it represent a NODE with 4 internal modes

**Option A: Minimal change**
- Keep `ModalVoice` as-is (single modal oscillator)
- Create new `ModalNode` that owns 4 `ModalVoice` instances

**Option B: Refactor**
- Rename `ModalVoice` → `ModalMode`
- Create `ModalNode` that owns `ModalMode modes[4]`

**Recommendation:** Option A for now (less risky, faster)

### Step 2: Create NodeState Structure

**New file:** `DSP/ModalNode.h`

```cpp
/**
 * @brief A single node in the network (MIDI voice)
 *
 * Each node contains 4 internal modal oscillators that are
 * coupled together. The network topology couples nodes together,
 * not individual modes.
 */
class ModalNode {
public:
    static constexpr uint32_t NUM_MODES = 4;

    ModalNode();
    ~ModalNode();

    /**
     * Initialize node
     * @param sampleRate Sample rate in Hz
     * @param nodeIndex Index of this node (0-15)
     */
    void initialize(float sampleRate, uint32_t nodeIndex);

    /**
     * Trigger node with MIDI note
     * @param midiNote MIDI note number
     * @param velocity Normalized velocity (0.0-1.0)
     */
    void noteOn(uint8_t midiNote, float velocity);

    /**
     * Release node
     */
    void noteOff();

    /**
     * Update node state (control rate)
     */
    void update();

    /**
     * Render audio from all 4 modes
     * @param outL Left channel
     * @param outR Right channel
     * @param numFrames Number of frames
     */
    void render(float* outL, float* outR, uint32_t numFrames);

    /**
     * Apply coupling force from other nodes
     * @param couplingForce Coupling force vector
     */
    void applyCoupling(float couplingForce);

    /**
     * Get current state for coupling calculation
     */
    float getStateForCoupling() const;

    /**
     * Check if node is active
     */
    bool isActive() const { return active_; }

    /**
     * Get MIDI note
     */
    uint8_t getMidiNote() const { return midiNote_; }

    /**
     * Get node age (for voice stealing)
     */
    uint32_t getAge() const { return age_; }

    /**
     * Get node amplitude (for quietest-first stealing)
     */
    float getAmplitude() const;

private:
    ModalVoice* modes_[NUM_MODES];  // 4 internal modal oscillators

    uint8_t midiNote_;
    float velocity_;
    bool active_;
    uint32_t age_;
    uint32_t nodeIndex_;

    // Per-mode parameters (TODO: wire to AU parameters)
    float modeFreqMult_[NUM_MODES];
    float modeDamping_[NUM_MODES];
    float modeWeight_[NUM_MODES];
};
```

### Step 3: Update VoiceAllocator → NodeAllocator

**File:** `DSP/VoiceAllocator.h/.cpp`

**Changes:**
1. Rename class to `NodeAllocator` (or keep name, update comments)
2. Change internal storage:
   ```cpp
   // OLD:
   ModalVoice** voices_;              // Wrong: allocates voices

   // NEW:
   ModalNode** nodes_;                 // Correct: allocates nodes
   uint32_t maxNodes_;                 // Max 16
   uint32_t activeNodeLimit_;          // Current limit (1-16)
   ```

3. Update MIDI mapping:
   ```cpp
   // Unchanged - maps note to node index
   int8_t note_to_node_[128];  // -1 = none
   ```

4. Update allocation logic:
   ```cpp
   ModalNode* noteOn(uint8_t midi_note, float velocity) {
       // Only allocate from nodes[0..activeNodeLimit-1]
       // ...
   }
   ```

### Step 4: Add nodeCount Parameter

**File:** `Parameters/ModalEffectExtensionParameterAddresses.h`

```cpp
typedef NS_ENUM(AUParameterAddress, ModalEffectExtensionParameterAddress) {
    // Global parameters
    kParam_MasterGain = 0,
    kParam_CouplingStrength = 1,
    kParam_Topology = 2,
    kParam_NodeCount = 3,        // NEW: 1-16, default 16

    // Per-mode parameters (Mode 0)
    kParam_Mode0_Frequency = 4,  // Shifted by +1
    // ... rest shifted
};
```

**File:** `Parameters/Parameters.swift`

```swift
ParameterSpec(
    address: .kParam_NodeCount,
    identifier: "nodeCount",
    name: "Node Count",
    units: .indexed,
    valueRange: 1...16,
    defaultValue: 16,
    flags: [.flag_IsWritable, .flag_IsReadable]
)
```

### Step 5: Update TopologyEngine

**File:** `DSP/TopologyEngine.h/.cpp`

**Current:** Assumes fixed `num_voices_`

**Change:**
```cpp
class TopologyEngine {
public:
    TopologyEngine(uint32_t maxNodes);  // Max 16

    /**
     * Set active node count and regenerate topology
     * @param nodeCount Active node count (1-16)
     * Thread-safe: can be called from parameter thread
     */
    void setNodeCount(uint32_t nodeCount);

    /**
     * Generate topology for current node count
     * Thread-safe: builds new matrix, atomic swap
     */
    void generateTopology(TopologyType type, float strength);

    /**
     * Update coupling (only for active nodes)
     * @param nodes Array of node pointers
     * @param nodeCount Number of active nodes
     */
    void updateCoupling(ModalNode** nodes, uint32_t nodeCount);

private:
    uint32_t maxNodes_;         // Always 16
    uint32_t activeNodeCount_;  // Current 1-16

    // Coupling matrix is [activeNodeCount × activeNodeCount]
    // Reallocated when activeNodeCount changes
};
```

### Step 6: Update SynthEngine

**File:** `DSP/SynthEngine.h/.cpp`

**Changes:**
1. Replace `VoiceAllocator` with `NodeAllocator`
2. Update parameter handling for `kParam_NodeCount`
3. Handle node count changes:
   ```cpp
   void setParameter(uint32_t paramId, float value) {
       switch (paramId) {
           case kParam_NodeCount: {
               uint32_t newCount = std::clamp((uint32_t)value, 1u, 16u);
               if (newCount != nodeCount_) {
                   // Fade out nodes above new limit
                   for (uint32_t i = newCount; i < nodeCount_; i++) {
                       nodeAllocator_->releaseNode(i);
                   }
                   nodeCount_ = newCount;
                   nodeAllocator_->setActiveNodeLimit(newCount);
                   // Regenerate topology for new count
                   topologyEngine_->setNodeCount(newCount);
               }
               break;
           }
           // ...
       }
   }
   ```

### Step 7: Update Documentation

**Files to update:**
- `AU_IMPLEMENTATION_GUIDE.md`
- `IMPLEMENTATION_STATUS.md`
- All class/function comments

**Key messaging:**
- "16-voice poly synth where each voice is 4-oscillator/modal"
- "Network couples nodes (not individual modes)"
- "nodeCount parameter controls active topology size"

---

## Implementation Priority

### Phase 1: Minimal Viable Refactor (Do Now)
1. ✅ Add `kParam_NodeCount` parameter (shifted all existing params)
2. ✅ Update parameter tree in Swift
3. ✅ Add documentation about node architecture
4. ⏸️ Keep current implementation working (call "voice" a "node" in comments)

### Phase 2: Proper Node Structure (Later)
1. Create `ModalNode` class
2. Refactor `VoiceAllocator` → `NodeAllocator`
3. Update topology to respect node count
4. Wire per-mode parameters to node internals

---

## State Compatibility Strategy

**Version 1 (current state):**
```json
{
  "version": 1,
  "masterGain": 0.7,
  "couplingStrength": 0.3,
  "topology": 0
  // No nodeCount
}
```

**Version 2 (with nodeCount):**
```json
{
  "version": 2,
  "nodeCount": 16,  // NEW: defaults to 16 if missing
  "masterGain": 0.7,
  "couplingStrength": 0.3,
  "topology": 0
}
```

**Migration rule:**
- If `nodeCount` missing → default to 16 (full compatibility)
- If `version` missing → assume version 1

---

## Benefits of This Architecture

### Cleaner Conceptual Model
- ✅ Network = 16 nodes
- ✅ Each node = mini 4-oscillator synth
- ✅ Topology couples nodes (meaningful)
- ✅ MIDI note → allocate node (intuitive)

### CPU Efficiency
- ✅ Topology dimension = nodeCount (not fixed 16)
- ✅ Unused nodes don't participate in coupling
- ✅ Control-rate updates scale with active nodes

### Scalability
- ✅ nodeCount can be automated by host
- ✅ Easy to add UI later (node count slider)
- ✅ Per-mode parameters ready for future wiring

### Musical Utility
- ✅ Lower node count = simpler network (clearer patterns)
- ✅ Higher node count = complex emergent behavior
- ✅ User can explore network size as creative parameter

---

## Testing After Refactor

1. **Sanity check:** nodeCount=1 should work (single node, 4 modes)
2. **Polyphony test:** Play 16 notes simultaneously
3. **Voice stealing:** Play 17th note with nodeCount=16
4. **nodeCount change:** Change from 16→8 while notes playing
5. **State save/load:** Verify nodeCount persists

---

## Current Status

- [x] Understand node vs voice distinction
- [x] Document refactoring plan
- [ ] Add kParam_NodeCount to parameter addresses
- [ ] Update Parameters.swift
- [ ] Update SynthEngine to handle nodeCount
- [ ] Create ModalNode class (future)
- [ ] Refactor allocator (future)

**Next immediate step:** Add the nodeCount parameter to make the plugin future-proof, even if full node architecture comes later.
