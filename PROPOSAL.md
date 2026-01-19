# ModalAttractors Plugin Enhancement Proposal

**Date:** 2026-01-19
**Branch:** `claude/bind-presets-to-node-ZBBYV`

---

## Executive Summary

This proposal outlines a phased approach to enhance the ModalAttractors AUv3 plugin with:
1. **UI Enhancements:** Add waveform selection to mode parameters, replace sliders with knobs, optimize layout
2. **Architecture Improvements:** Implement robust two-way parameter binding between AUParameterTree and DSP engine

---

## Part A: UI Enhancement (Phased Implementation)

### Current State Analysis

**Tab 2 (Character Editor)** currently displays:
- Node selector (0-4)
- Template loader (15 character presets)
- **Mode Parameters (0-3):** Each mode shows 3 sliders:
  - Frequency Multiplier (0.5-8.0)
  - Damping (0.1-5.0)
  - Weight (0.0-1.0)
- Excitation controls (Poke Strength, Poke Duration)
- Personality selector
- "Apply to Node" button
- "Save as Custom Preset" button

**Missing:** Wave shape selection per mode (20 parameters defined but not exposed in UI)

### Phase 1: Add Waveform Selection to Modes 0-3

#### Implementation Details

**Files to Modify:**
1. `/ModalAttractorsExtension/UI/Components/ModeControlsView.swift`
2. `/ModalAttractorsExtension/UI/CharacterEditorTabView.swift`

**Changes:**

1. **Extend ModeControlsView** to include waveform picker per mode:
   ```swift
   struct ModeControlsView {
       // Current: 3 sliders per mode (Freq, Damping, Weight)
       // Add: 1 picker per mode (WaveShape)

       ForEach(0..<4) { modeIndex in
           VStack {
               Text("Mode \(modeIndex)")

               // New: Wave shape picker
               Picker("Wave", selection: $waveBinding) {
                   ForEach(waveOptions) { option in
                       Text(option.name).tag(option.id)
                   }
               }
               .pickerStyle(.segmented) // or .menu

               // Existing: 3 parameter sliders
               ParameterSlider(...)  // Frequency
               ParameterSlider(...)  // Damping
               ParameterSlider(...)  // Weight
           }
       }
   ```

2. **Bind to existing wave shape parameters:**
   - Parameters already defined: `Node[0-4]_Mode[0-3]_WaveShape` (addresses 27-46)
   - Wave options: Sine, Sawtooth, Triangle, Square, Pulse 25%, Pulse 10%
   - Binding strategy: Use `ParameterWrapper` to connect to correct node/mode parameter

3. **Layout options:**
   - **Option A:** Horizontal segmented control above each mode's sliders
   - **Option B:** Dropdown/menu picker to save vertical space
   - **Option C:** Icon-based picker with waveform glyphs (SF Symbols)

**Recommendation:** Option A (segmented control) for quick visual access, or Option B (menu) if vertical space is constrained.

**Acceptance Criteria:**
- [ ] User can select waveform for each mode (0-3) on currently selected node
- [ ] Waveform selection persists with preset save/load
- [ ] Waveform parameter updates DSP engine in realtime
- [ ] UI reflects current waveform when loading preset or switching nodes

---

### Phase 2: Replace Sliders with Knobs

#### Current Slider Usage

**Sliders currently used for:**
1. Mode parameters (12 sliders on Tab 2):
   - Mode 0-3: Frequency, Damping, Weight
2. Excitation parameters (2 sliders on Tab 2):
   - Poke Strength, Poke Duration
3. Global parameters (2 sliders on Tab 1):
   - Coupling Strength
   - (Master Gain already uses ParameterKnob)

#### Implementation Plan

**Conversion Strategy:**

1. **Keep sliders for:**
   - Coupling Strength (Tab 1) - horizontal slider works well for coupling visualization
   - Poke Duration (Tab 2) - time-based parameters often clearer as sliders

2. **Convert to knobs:**
   - **Mode Frequency Multipliers** (4 knobs) - ratio/multiplier parameters suit knobs
   - **Mode Damping** (4 knobs) - continuous scalar parameters
   - **Mode Weight** (4 knobs) - 0-1 normalized parameters ideal for knobs
   - **Poke Strength** (1 knob) - normalized parameter

**Layout Design:**

```
Mode 0    Mode 1    Mode 2    Mode 3
[Wave▼]   [Wave▼]   [Wave▼]   [Wave▼]
   ⭕        ⭕        ⭕        ⭕     <- Frequency knobs
  Freq     Freq     Freq     Freq
   ⭕        ⭕        ⭕        ⭕     <- Damping knobs
  Damp     Damp     Damp     Damp
   ⭕        ⭕        ⭕        ⭕     <- Weight knobs
 Weight   Weight   Weight   Weight
```

**Component Reuse:**
- Use existing `ParameterKnob.swift` (already implemented with accessibility)
- Customize size: Suggest 60pt diameter for mode knobs, 80pt for excitation knobs
- Add `.parameterKnob()` modifier if needed for consistent styling

**Files to Modify:**
1. `/ModalAttractorsExtension/UI/Components/ModeControlsView.swift` - Replace 12 sliders with 12 knobs
2. `/ModalAttractorsExtension/UI/CharacterEditorTabView.swift` - Replace 2 sliders with knobs for excitation

**Code Example:**
```swift
// Before:
ParameterSlider(parameter: freqWrapper, label: "Frequency", formatString: "%.2f×")

// After:
ParameterKnob(parameter: freqWrapper,
              label: "Frequency",
              size: 60,
              formatString: "%.2f×")
```

**Acceptance Criteria:**
- [ ] All mode parameters (Freq, Damping, Weight) use rotary knobs
- [ ] Knobs maintain accessibility (VoiceOver, keyboard control)
- [ ] Visual consistency across all knob instances
- [ ] Knob drag sensitivity feels natural (200pt vertical = full range)

---

### Phase 3: Finalize Layout and Positions

#### Layout Goals

1. **Visual Hierarchy:** Modes flow left-to-right, parameters stack vertically within each mode
2. **Grouping:** Clear visual separation between mode groups and excitation controls
3. **Density:** Fit comfortably on iPad (1024×768 minimum) without scrolling
4. **Responsiveness:** Adapt gracefully to macOS window resizing

#### Proposed Layout (Character Editor Tab)

```
┌─────────────────────────────────────────────────────────────┐
│  Character Editor                                            │
├─────────────────────────────────────────────────────────────┤
│  Node: [0][1][2][3][4]                    Template: [▼Picker]│
│                                           [Apply to Node]    │
├─────────────────────────────────────────────────────────────┤
│  MODE PARAMETERS                                             │
│  ┌─────────┬─────────┬─────────┬─────────┐                  │
│  │ Mode 0  │ Mode 1  │ Mode 2  │ Mode 3  │                  │
│  ├─────────┼─────────┼─────────┼─────────┤                  │
│  │ [Wave ▼]│ [Wave ▼]│ [Wave ▼]│ [Wave ▼]│                  │
│  │    ⭕   │    ⭕   │    ⭕   │    ⭕   │  Frequency         │
│  │   Freq  │   Freq  │   Freq  │   Freq  │                  │
│  │    ⭕   │    ⭕   │    ⭕   │    ⭕   │  Damping           │
│  │   Damp  │   Damp  │   Damp  │   Damp  │                  │
│  │    ⭕   │    ⭕   │    ⭕   │    ⭕   │  Weight            │
│  │  Weight │  Weight │  Weight │  Weight │                  │
│  └─────────┴─────────┴─────────┴─────────┘                  │
├─────────────────────────────────────────────────────────────┤
│  EXCITATION                                                  │
│     ⭕ Strength      ━━━━━━━ Duration (ms)                   │
├─────────────────────────────────────────────────────────────┤
│  PERSONALITY: (○) Resonator  (○) Self-Oscillator            │
├─────────────────────────────────────────────────────────────┤
│  [Save as Custom Preset]                                     │
└─────────────────────────────────────────────────────────────┘
```

#### Layout Specifications

**Spacing:**
- Mode columns: 16pt horizontal padding
- Vertical stack spacing: 12pt between knob rows
- Section separators: 24pt vertical padding

**Component Sizes:**
- Mode knobs: 60pt diameter
- Excitation knob: 80pt diameter
- Wave picker: Full width of mode column (~120pt)
- Buttons: Standard height (44pt minimum for touch)

**Grid System:**
- Use SwiftUI `HStack` / `VStack` with `.spacing()` modifiers
- Alternatively: `LazyVGrid` with 4 columns for modes
- Ensure minimum widths prevent overlap on small windows

**Responsive Behavior:**
- Minimum window width: 800pt (show all 4 modes side-by-side)
- Below 800pt: Wrap to 2×2 grid or enable horizontal scroll
- macOS: Allow window resize, maintain aspect ratio

**Accessibility:**
- VoiceOver reads: "Mode 0 Frequency, 1.0 times, adjustable"
- Keyboard navigation: Tab order follows visual left-to-right, top-to-bottom
- High contrast mode: Knob outlines remain visible

**Acceptance Criteria:**
- [ ] All 4 modes visible without scrolling on iPad and macOS default window
- [ ] Visual hierarchy clear: Modes → Parameters → Excitation → Personality → Actions
- [ ] Spacing consistent and follows design system
- [ ] Layout adapts to window resize without breaking
- [ ] Touch targets meet 44pt minimum on iOS/iPadOS
- [ ] Dark mode and light mode both well-designed

---

## Part B: Parameter Binding Architecture Improvements

### Current State Problems

1. **Parameter ID drift:** ParamID enum in `SynthEngine.cpp` may diverge from Swift addresses
2. **No two-way binding:** DSP changes don't reflect back to UI automatically
3. **Discrete parameter rounding:** Host automation sends floats, truncation causes off-by-one errors
4. **Default initialization:** DSP engine may not receive AU parameter defaults at startup
5. **Preset binding:** Preset changes update UI but may not reliably flow to DSP

### Architecture Requirements

#### 1. Single Source of Truth for Parameter IDs

**Problem:** `ParamID` enum exists in `SynthEngine.cpp` (private), duplicated logic in `ModalAttractorsExtensionParameterAddresses.h`

**Solution:**

1. **Move `ParamID` enum to shared header:**
   - Create: `/ModalAttractorsExtension/Common/DSP/ModalAttractorsParameterIDs.h`
   - Make it a C-compatible header (`extern "C"` guards for Swift bridging)
   - Include from: `SynthEngine.cpp`, `SynthEngine.h`, Swift bridging header

2. **Update Swift parameter addresses:**
   - In `Parameters.swift`, ensure `ParameterSpec.address` values match `ParamID` values exactly
   - Consider generating Swift enum from C header using script (optional)

**Files to Create/Modify:**
- NEW: `/ModalAttractorsExtension/Common/DSP/ModalAttractorsParameterIDs.h`
- MODIFY: `/ModalAttractorsExtension/DSP/SynthEngine.cpp` (remove enum, include new header)
- MODIFY: `/ModalAttractorsExtension/DSP/SynthEngine.h` (include new header if needed)
- MODIFY: `/ModalAttractorsExtension/Parameters/Parameters.swift` (verify addresses match)
- REMOVE: `/ModalAttractorsExtension/Parameters/ModalAttractorsExtensionParameterAddresses.h` (if redundant)

**Example Header:**
```cpp
// ModalAttractorsParameterIDs.h
#ifndef MODAL_ATTRACTORS_PARAMETER_IDS_H
#define MODAL_ATTRACTORS_PARAMETER_IDS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kParam_MasterGain = 0,
    kParam_CouplingStrength = 1,
    kParam_Topology = 2,
    kParam_NodeCount = 3,
    kParam_Mode0_Frequency = 4,
    // ... all 47 parameters
    kParam_Node4_Mode3_WaveShape = 46,
    kParam_Count = 47
} ModalAttractorsParameterID;

#ifdef __cplusplus
}
#endif

#endif // MODAL_ATTRACTORS_PARAMETER_IDS_H
```

---

#### 2. AU Parameter Binding Glue (AUAudioUnit ↔ SynthEngine)

**Current State:** Parameter tree created but no `implementorValueObserver` or `implementorValueProvider` attached

**Solution:**

**Location:** AUAudioUnit implementation (likely in `ModalAttractorsFramework/Common/AudioUnit/` or `ModalAttractorsExtension/AudioUnit/`)

**Implementation:**

```swift
// In AUAudioUnit subclass init or after parameterTree creation:

// 1. Attach observer: AU parameter changes → DSP
parameterTree.implementorValueObserver = { [weak self] param, value in
    guard let self = self,
          let synthEngine = self.synthEngine else { return }

    let address = UInt32(param.address)
    synthEngine.setParameter(address, value)
}

// 2. Attach provider: DSP state → AU parameter queries
parameterTree.implementorValueProvider = { [weak self] param in
    guard let self = self,
          let synthEngine = self.synthEngine else {
        return param.value  // Fallback to cached value
    }

    let address = UInt32(param.address)
    return synthEngine.getParameter(address)
}

// 3. Initialize DSP with defaults
for param in parameterTree.allParameters {
    param.setValue(param.value, originator: nil)  // Triggers implementorValueObserver
}
```

**Files to Modify:**
- Find AUAudioUnit subclass (search for `class.*AUAudioUnit` in Swift files)
- Add binding code after `parameterTree` initialization
- Ensure `synthEngine` property is accessible in binding closures

**Acceptance Criteria:**
- [ ] UI parameter changes flow to DSP via `implementorValueObserver`
- [ ] Host automation flows to DSP via same path
- [ ] DSP state queryable via `implementorValueProvider` (optional but recommended)
- [ ] Default values applied to DSP at instantiation

---

#### 3. DSP Parameter Robustness (Rounding/Clamping)

**Problem:** Discrete parameters (Topology, Character, WaveShape) receive float values from automation, truncation causes off-by-one

**Solution:**

**File:** `/ModalAttractorsExtension/DSP/SynthEngine.cpp`

**Modify `SynthEngine::setParameter()`:**

```cpp
void SynthEngine::setParameter(uint32_t paramId, float value) {
    // For discrete/indexed parameters, round to nearest int and clamp
    auto roundClamp = [](float v, int min, int max) -> int {
        return std::clamp(static_cast<int>(std::round(v)), min, max);
    };

    switch (paramId) {
        case kParam_Topology:
            // 0..6 (7 topologies)
            topologyEngine->setTopology(roundClamp(value, 0, 6));
            break;

        case kParam_Node0_Character:
        case kParam_Node1_Character:
        case kParam_Node2_Character:
        case kParam_Node3_Character:
        case kParam_Node4_Character: {
            // 0..14 (15 characters)
            int nodeIdx = paramId - kParam_Node0_Character;
            int character = roundClamp(value, 0, 14);
            nodeManager->setNodeCharacter(nodeIdx, character);
            break;
        }

        case kParam_NoteRouting:
            // 0..1 (2 options)
            noteRouting = roundClamp(value, 0, 1);
            break;

        case kParam_MultiExcite:
            // 0..1 (2 options)
            multiExcite = roundClamp(value, 0, 1);
            break;

        case kParam_Node0_Mode0_WaveShape:
        // ... all 20 wave shape parameters
        case kParam_Node4_Mode3_WaveShape: {
            // 0..5 (6 waveforms)
            int nodeIdx = (paramId - kParam_Node0_Mode0_WaveShape) / 4;
            int modeIdx = (paramId - kParam_Node0_Mode0_WaveShape) % 4;
            int waveShape = roundClamp(value, 0, 5);
            nodeManager->setWaveShape(nodeIdx, modeIdx, waveShape);
            break;
        }

        // Continuous parameters: use value directly
        case kParam_MasterGain:
        case kParam_CouplingStrength:
        case kParam_Mode0_Frequency:
        // ... etc
        default:
            // Existing continuous parameter handling
            break;
    }
}
```

**Acceptance Criteria:**
- [ ] Topology selector: automation value 0.9999 → topology 1 (not 0)
- [ ] Character selector: automation value 2.5 → character 3 (rounded, not truncated)
- [ ] WaveShape selector: automation value 3.7 → waveform 4 (rounded)
- [ ] Continuous parameters: unaffected by rounding logic

---

#### 4. Default Synchronization

**Problem:** AUParameter defaults set in Swift, but DSP may initialize with different values

**Solution:** (Already covered in section 2 binding code)

```swift
// After setting up implementorValueObserver:
for param in parameterTree.allParameters {
    param.setValue(param.value, originator: nil)
}
```

**Additional Check:** Verify `Parameters.swift` default values match intended DSP behavior:
- Mode 0: freq=1.0, damping=1.0, weight=1.0 ✓
- Mode 1: freq=2.0, damping=1.2, weight=0.8 ✓
- Mode 2: freq=3.0, damping=1.5, weight=0.6 ✓
- Mode 3: freq=4.5, damping=2.0, weight=0.4 ✓
- All wave shapes: default to Sine (0) ✓

**Files to Verify:**
- `/ModalAttractorsExtension/Parameters/Parameters.swift` (Swift defaults)
- `/ModalAttractorsExtension/DSP/SynthEngine.cpp` (C++ initialization)

---

#### 5. UI Binding Layer (SwiftUI ↔ AUParameterTree)

**Current State:** `ParameterWrapper` exists and works well

**Verification Needed:**

1. **Check `ParameterWrapper` implementation:**
   - Does it use `@Published` for SwiftUI reactivity?
   - Does it read/write `AUParameter.value` directly?
   - Does it handle discrete parameters correctly (Int ↔ Float conversion)?

2. **Wave shape picker binding:**
   ```swift
   // In view:
   let waveParam = parameterTree.parameter(withAddress: addressForNodeMode)
   let waveWrapper = ParameterWrapper(parameter: waveParam)

   Picker("Wave", selection: $waveWrapper.intValue) {  // Need .intValue binding
       ForEach(0..<6) { index in
           Text(waveNames[index]).tag(index)
       }
   }
   ```

3. **If `ParameterWrapper` lacks `.intValue` computed property:**
   ```swift
   extension ParameterWrapper {
       var intValue: Int {
           get { Int(round(value)) }
           set { value = Float(newValue) }
       }
   }
   ```

**Files to Check/Modify:**
- `/ModalAttractorsExtension/UI/Utilities/ParameterWrapper.swift` (if exists)
- `/ModalAttractorsExtension/UI/Utilities/ParameterTree.swift`

**Acceptance Criteria:**
- [ ] SwiftUI views never call `SynthEngine` directly
- [ ] All UI bindings go through `AUParameterTree` and `ParameterWrapper`
- [ ] Discrete parameter pickers use Int bindings (automatic Float conversion)
- [ ] UI updates when parameter changes externally (host automation)

---

#### 6. Spec/Range Alignment Validation

**Audit Required:**

| Parameter | Swift Spec | DSP Behavior | Status |
|-----------|-----------|--------------|--------|
| NodeCount | 1..5, default 5 | Fixed at 5 | ⚠️ Misleading (make read-only or remove) |
| Topology | 0..6 indexed | TopologyEngine supports 0-6 | ✓ Aligned |
| Character | 0..14 indexed | CharacterPreset templates 0-14 | ✓ Aligned |
| WaveShape | 0..5 indexed | 6 waveforms in DSP | ✓ Aligned |
| Mode Freq | 0.5..8.0 | No hard limits in DSP? | ⚠️ Verify clipping |
| Mode Damping | 0.1..5.0 | No hard limits in DSP? | ⚠️ Verify clipping |
| Mode Weight | 0.0..1.0 | Normalized in DSP | ✓ Aligned |

**Actions:**
1. **NodeCount:** If truly fixed at 5, mark parameter as read-only (`flags: .flag_IsReadable`)
2. **Mode ranges:** Verify DSP doesn't clip or exhibit undefined behavior outside spec ranges

**Files to Check:**
- `/ModalAttractorsExtension/DSP/ModalVoice.cpp` (mode parameter handling)
- `/ModalAttractorsExtension/DSP/TopologyEngine.cpp` (node count handling)

---

## Implementation Timeline (Phased)

### Phase 1: Add Waveform Selection (1-2 days)
- [ ] Task 1.1: Modify `ModeControlsView.swift` to add wave pickers
- [ ] Task 1.2: Bind wave pickers to `Node*_Mode*_WaveShape` parameters
- [ ] Task 1.3: Update `CharacterPreset` to include wave shapes
- [ ] Task 1.4: Update `CharacterPresetManager` save/load logic
- [ ] Task 1.5: Test: Load preset → verify waves set correctly
- [ ] Task 1.6: Test: Change wave → verify DSP receives update

### Phase 2: Replace Sliders with Knobs (1 day)
- [ ] Task 2.1: Replace 12 mode sliders with `ParameterKnob` instances
- [ ] Task 2.2: Replace poke strength slider with knob (optional: keep duration as slider)
- [ ] Task 2.3: Adjust knob sizes (60pt for mode, 80pt for excitation)
- [ ] Task 2.4: Test: Verify knob drag feel and accessibility
- [ ] Task 2.5: Test: Dark mode and light mode appearance

### Phase 3: Finalize Layout (1 day)
- [ ] Task 3.1: Implement grid layout for 4 modes (HStack or LazyVGrid)
- [ ] Task 3.2: Add spacing and section dividers
- [ ] Task 3.3: Test responsive behavior (window resize, iPad vs macOS)
- [ ] Task 3.4: Accessibility audit (VoiceOver, keyboard navigation)
- [ ] Task 3.5: Visual QA (alignment, spacing, contrast)

### Phase 4: Parameter Binding Architecture (2-3 days)
- [ ] Task 4.1: Create `ModalAttractorsParameterIDs.h` shared header
- [ ] Task 4.2: Update `SynthEngine.cpp` to include shared header
- [ ] Task 4.3: Verify Swift parameter addresses match C++ enum
- [ ] Task 4.4: Add `implementorValueObserver` and `implementorValueProvider` to AUAudioUnit
- [ ] Task 4.5: Initialize DSP with default values at startup
- [ ] Task 4.6: Add rounding/clamping logic to `SynthEngine::setParameter()` for discrete params
- [ ] Task 4.7: Audit and fix any range alignment issues
- [ ] Task 4.8: Test: Host automation → verify discrete params round correctly
- [ ] Task 4.9: Test: Preset load → verify all params flow to DSP
- [ ] Task 4.10: Test: UI change → verify DSP updates immediately

### Phase 5: Integration Testing (1 day)
- [ ] Task 5.1: End-to-end test: Load preset → edit mode → change wave → apply to node → save preset
- [ ] Task 5.2: Test in DAW: Logic Pro automation, Ableton Live, etc.
- [ ] Task 5.3: Verify preset persistence across plugin instances
- [ ] Task 5.4: Performance test: Verify no audio glitches during parameter changes
- [ ] Task 5.5: Memory test: Check for leaks in parameter binding closures

**Total Estimated Effort:** 6-8 days of focused development

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Parameter ID mismatch causes audio glitches | High | Use shared header, add validation asserts |
| Knob UI too small for precise editing | Medium | Add fine-tune mode (Shift+drag for 10× precision) |
| Wave picker clutters UI on small screens | Medium | Use compact menu picker instead of segmented control |
| Preset backward compatibility broken | High | Version preset format, migrate old presets automatically |
| Real-time thread safety in setParameter | High | Use lock-free event queue (already implemented) |
| Default sync causes audio dropout at init | Low | Defaults set before audio starts, not during render |

---

## Success Metrics

**UI Enhancements:**
- User can select waveform for each mode without confusion
- Knob interface reduces clicks compared to sliders (1 drag vs 2 actions)
- Layout fits on iPad without scrolling (100% of target device support)

**Parameter Binding:**
- Zero parameter drift bugs in bug tracker
- Host automation works correctly in 100% of tested DAWs
- Preset recall accuracy: 100% (all parameters match saved state)

**Performance:**
- No audio dropouts during parameter changes (tested with 32-voice polyphony)
- UI remains responsive during heavy automation (60 FPS maintained)

---

## Next Steps

**Recommendation:**
1. **Review this proposal** and approve/modify phasing
2. **Phase 1 first:** Add waveform selection (highest user value, lowest risk)
3. **Phase 4 in parallel:** Architecture improvements (foundational, benefits all phases)
4. **Phase 2-3 after:** UI polish (depends on Phase 1 working)

**Questions for Clarification:**
1. Should NodeCount parameter be removed or kept as read-only?
2. Should Poke Duration remain a slider or convert to knob?
3. Should wave picker be segmented control (wide) or menu (compact)?
4. Should we add Shift+drag fine-tune mode to knobs?
5. Should presets be versioned for backward compatibility?

---

**Ready to proceed?** Please approve this proposal or request modifications, and I'll begin implementation starting with Phase 1.
