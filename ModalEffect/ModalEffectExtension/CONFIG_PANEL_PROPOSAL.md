# Modal Attractors Config Panel Proposal

**Author:** Claude
**Date:** January 7, 2026
**Target:** ModalEffectExtension UI Layer
**Status:** Proposal for Review

---

## Executive Summary

This proposal outlines the design and implementation plan for a **SwiftUI-based configuration panel** for the Modal Attractors Audio Unit Extension. The panel will expose network topology, excitation triggers, and drive/gain controls in an intuitive, performant interface that integrates seamlessly with the existing parameter system defined in `Parameters.swift`.

**Key Points:**
- No UI layer currently exists—this will be built from scratch
- All parameters are already defined and wired to AU parameter tree
- Focus on three primary control sections: **Network**, **Triggers**, and **Drive**
- SwiftUI-first design leveraging existing parameter binding infrastructure
- Phased implementation strategy to ensure stability

---

## 1. Current State Analysis

### 1.1 What Exists

#### Parameter System ✅
**Location:** `ModalEffect/ModalEffectExtension/Parameters/`

The extension has a complete parameter tree specification with **19 parameters** organized into 6 groups:

| Group | Parameters | Purpose |
|-------|-----------|---------|
| **global** | `masterGain`, `couplingStrength`, `topology` | Network-level controls |
| **mode0-3** | `frequency`, `damping`, `weight` (×4 modes) | Per-mode oscillator tuning |
| **excitation** | `pokeStrength`, `pokeDuration` | Trigger/impulse controls |
| **voice** | `polyphony`, `personality` | Voice allocation settings |

**Parameter Addresses:**
`ModalEffectExtensionParameterAddresses.h` (lines 13-46) defines enum values `kParam_MasterGain` through `kParam_Personality` (addresses 0-18).

**Parameter Tree Spec:**
`Parameters.swift` (lines 12-208) uses declarative `ParameterTreeSpec` DSL with:
- Type-safe parameter specs with value ranges, units, defaults
- Value strings for indexed params (e.g., topology names)
- Read-only flags (e.g., `polyphony`)

#### DSP Integration ⚠️
**Location:** `ModalEffect/ModalEffectExtension/DSP/ModalEffectExtensionDSPKernel.hpp`

The DSP kernel has parameter getter/setter stubs (lines 42-67) but **implementation is pending**:
```cpp
case kParam_MasterGain:
    // TODO: Implement parameter handling
    break;
```

**Implication:** UI can be built independently; DSP integration happens in parallel.

#### UI Layer ❌
**Expected Location:** `ModalEffect/ModalEffectExtension/UI/ModalEffectExtensionMainView.swift`

**Current State:** Directory and file **do not exist**.

**README Expectation (lines 30-31):**
> `/UI`
> `ModalEffectExtensionMainView.swift` - SwiftUI based main view, add your SwiftUI views and controls here.

**README Parameter Access Pattern (lines 90-113):**
```swift
// Access via dot notation
parameterTree.global.masterGain.value

// Bind to slider
ParameterSlider(param: parameterTree.global.masterGain)
```

### 1.2 What's Missing

1. **No SwiftUI views** for parameter control
2. **No UI directory structure** in extension target
3. **Drive parameter undefined** (only `masterGain` exists)
4. **No custom SwiftUI controls** (e.g., `ParameterSlider`, topology picker)
5. **No view hierarchy** for organizing Network/Triggers/Drive sections

---

## 2. Proposed Config Panel Architecture

### 2.1 File Structure

Create the following in `ModalEffect/ModalEffectExtension/`:

```
ModalEffectExtension/
├── UI/
│   ├── ModalEffectExtensionMainView.swift    # Root view container
│   ├── Components/
│   │   ├── NetworkControlsView.swift             # Network section
│   │   ├── TriggersControlsView.swift            # Triggers section
│   │   ├── DriveControlsView.swift               # Drive/gain section
│   │   └── Widgets/
│   │       ├── ParameterSlider.swift             # Reusable slider
│   │       ├── ParameterKnob.swift               # Rotary knob control
│   │       └── TopologyPicker.swift              # Topology selector
│   └── Utilities/
│       └── UIConstants.swift                     # Colors, spacing, fonts
```

### 2.2 View Hierarchy

```
ModalEffectExtensionMainView
├── VStack
│   ├── NetworkControlsView
│   │   ├── TopologyPicker (global.topology)
│   │   └── ParameterSlider (global.couplingStrength)
│   ├── Divider
│   ├── TriggersControlsView
│   │   ├── ParameterSlider (excitation.pokeStrength)
│   │   └── ParameterSlider (excitation.pokeDuration)
│   │   └── TriggerButton (manual poke)
│   ├── Divider
│   └── DriveControlsView
│       ├── ParameterKnob (global.masterGain)
│       └── ParameterSlider (global.drive) *NEW PARAM*
```

### 2.3 Parameter Binding Strategy

Leverage the existing `ParameterTreeSpec` system:

**Access Pattern:**
```swift
@EnvironmentObject var parameterTree: ParameterTree

// Global parameters
parameterTree.global.topology.value
parameterTree.global.couplingStrength.value
parameterTree.global.masterGain.value

// Excitation parameters
parameterTree.excitation.pokeStrength.value
parameterTree.excitation.pokeDuration.value
```

**Binding to Controls:**
```swift
ParameterSlider(
    param: parameterTree.global.couplingStrength,
    label: "Coupling",
    range: 0.0...1.0
)
```

**Implementation Note:** The `ParameterTree` type and `@EnvironmentObject` injection will need to be implemented as part of the UI layer, following the pattern suggested in the README.

---

## 3. Panel Sections: Detailed Design

### 3.1 Network Controls

**Purpose:** Configure voice coupling topology and strength.

**Parameters:**
- `global.topology` (indexed, 0-6)
  - Value strings: `["Ring", "Small World", "Clustered", "Hub-Spoke", "Random", "Complete", "None"]`
  - Default: `0` (Ring)
- `global.couplingStrength` (0.0-1.0)
  - Default: `0.3`

**UI Components:**

#### TopologyPicker
```swift
struct TopologyPicker: View {
    @Binding var topologyIndex: Int
    let topologies = ["Ring", "Small World", "Clustered",
                      "Hub-Spoke", "Random", "Complete", "None"]

    var body: some View {
        Picker("Topology", selection: $topologyIndex) {
            ForEach(0..<topologies.count, id: \.self) { index in
                Text(topologies[index]).tag(index)
            }
        }
        .pickerStyle(.segmented) // or .menu for compact
    }
}
```

#### Coupling Strength Slider
- Visual: Horizontal slider with numeric value display
- Range: 0% - 100%
- Step: 0.01 (smooth)
- Label: "Coupling Strength"

**Layout:**
```
┌─────────────────────────────────┐
│ NETWORK                         │
├─────────────────────────────────┤
│ Topology: [Ring ▼]             │
│                                 │
│ Coupling  [====○--------] 30%  │
└─────────────────────────────────┘
```

**Behavior:**
- Topology changes update DSP immediately (parameter tree auto-sync)
- Coupling slider has smooth interpolation
- Visual feedback: strength affects UI brightness/color saturation (optional)

### 3.2 Triggers / Excitation Controls

**Purpose:** Configure impulse/poke excitation behavior.

**Parameters:**
- `excitation.pokeStrength` (0.0-1.0)
  - Default: `0.5`
- `excitation.pokeDuration` (1.0-50.0 ms)
  - Default: `10.0`

**UI Components:**

#### Poke Strength Slider
- Visual: Vertical or horizontal slider
- Range: 0% - 100%
- Label: "Strength"

#### Poke Duration Slider
- Visual: Horizontal slider
- Range: 1ms - 50ms
- Label: "Duration"
- Units: Display as "10.0 ms"

#### Manual Trigger Button (Optional)
- Purpose: Manually fire a poke event for testing
- Implementation: Sends MIDI note-on or custom parameter event
- Visual: Large button "POKE" or "TRIGGER"

**Layout:**
```
┌─────────────────────────────────┐
│ TRIGGERS                        │
├─────────────────────────────────┤
│ Strength  [====○--------] 50%  │
│ Duration  [===○---------] 10ms │
│                                 │
│        [ TRIGGER POKE ]         │
└─────────────────────────────────┘
```

**Behavior:**
- Strength affects amplitude of excitation
- Duration controls impulse decay time
- Trigger button fires immediate poke (if implemented in DSP)

### 3.3 Drive / Output Controls

**Purpose:** Control output gain and (optional) drive/saturation.

**Current Parameters:**
- `global.masterGain` (0.0-1.0 linear gain)
  - Default: `0.7`

**Proposed New Parameter:**
- `global.drive` (0.0-1.0, maps to saturation amount)
  - **Not currently defined—requires adding to parameter tree**

**UI Components:**

#### Master Gain Knob
- Visual: Rotary knob (SVG or custom SwiftUI `Canvas`)
- Range: 0.0 - 1.0 (or -∞ to 0 dB if using log scale)
- Label: "Output"

#### Drive Slider (Proposed)
- Visual: Horizontal slider
- Range: 0% - 100%
- Label: "Drive"
- Effect: Soft clipping/saturation before output

**Layout:**
```
┌─────────────────────────────────┐
│ DRIVE                           │
├─────────────────────────────────┤
│     Output         Drive        │
│      ┌───┐   [====○----] 40%   │
│      │ ◯ │                      │
│      └───┘                      │
│       70%                       │
└─────────────────────────────────┘
```

**Implementation Notes:**

**Drive Parameter (if added):**
1. Add to `ModalEffectExtensionParameterAddresses.h`:
   ```c
   kParam_Drive = 19,
   ```

2. Add to `Parameters.swift` in `global` group:
   ```swift
   ParameterSpec(
       address: .kParam_Drive,
       identifier: "drive",
       name: "Drive",
       units: .generic,
       valueRange: 0.0...1.0,
       defaultValue: 0.0
   )
   ```

3. Implement in `ModalEffectExtensionDSPKernel.hpp`:
   ```cpp
   case kParam_Drive:
       mDrive = value;
       break;
   ```

4. Apply in DSP processing:
   ```cpp
   // Soft clipping: tanh(drive * signal)
   float applyDrive(float sample) {
       return std::tanh(mDrive * 3.0f * sample);
   }
   ```

**Alternative (Without New Parameter):**
- Use only `masterGain` for now
- Label as "Output Level"
- Add drive later in Phase 2

---

## 4. SwiftUI Implementation Details

### 4.1 Main View Structure

**File:** `UI/ModalEffectExtensionMainView.swift`

```swift
import SwiftUI
import AudioToolbox

struct ModalEffectExtensionMainView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(spacing: 20) {
            // Network section
            NetworkControlsView()

            Divider()

            // Triggers section
            TriggersControlsView()

            Divider()

            // Drive section
            DriveControlsView()
        }
        .padding()
        .frame(minWidth: 400, minHeight: 500)
    }
}

#Preview {
    ModalEffectExtensionMainView()
        .environmentObject(ParameterTree.preview)
}
```

### 4.2 Reusable Components

#### ParameterSlider

**File:** `UI/Components/Widgets/ParameterSlider.swift`

```swift
import SwiftUI

struct ParameterSlider: View {
    @Binding var value: Float
    let label: String
    let range: ClosedRange<Float>
    let unit: String?

    var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(label)
                    .font(.caption)
                    .foregroundColor(.secondary)

                Spacer()

                Text(formattedValue)
                    .font(.caption)
                    .foregroundColor(.primary)
            }

            Slider(
                value: $value,
                in: range
            )
        }
    }

    private var formattedValue: String {
        let formatted = String(format: "%.2f", value)
        return unit != nil ? "\(formatted) \(unit!)" : formatted
    }
}
```

**Usage:**
```swift
ParameterSlider(
    value: $parameterTree.global.couplingStrength.value,
    label: "Coupling",
    range: 0.0...1.0,
    unit: nil
)
```

#### ParameterKnob

**File:** `UI/Components/Widgets/ParameterKnob.swift`

```swift
import SwiftUI

struct ParameterKnob: View {
    @Binding var value: Float
    let label: String
    let range: ClosedRange<Float>

    @State private var isDragging = false
    @State private var dragStart: CGPoint = .zero
    @State private var valueStart: Float = 0.0

    var body: some View {
        VStack(spacing: 8) {
            // Rotary knob visual
            ZStack {
                Circle()
                    .fill(Color.gray.opacity(0.3))
                    .frame(width: 60, height: 60)

                Circle()
                    .trim(from: 0, to: normalizedValue)
                    .stroke(Color.blue, lineWidth: 4)
                    .frame(width: 60, height: 60)
                    .rotationEffect(.degrees(-90))

                // Pointer indicator
                Rectangle()
                    .fill(Color.white)
                    .frame(width: 2, height: 20)
                    .offset(y: -15)
                    .rotationEffect(.degrees(normalizedValue * 270 - 135))
            }
            .gesture(
                DragGesture()
                    .onChanged { gesture in
                        if !isDragging {
                            isDragging = true
                            dragStart = gesture.location
                            valueStart = value
                        }

                        let delta = Float(gesture.location.y - dragStart.y)
                        let sensitivity: Float = 0.01
                        let newValue = valueStart - (delta * sensitivity)
                        value = min(max(newValue, range.lowerBound), range.upperBound)
                    }
                    .onEnded { _ in
                        isDragging = false
                    }
            )

            Text(label)
                .font(.caption)

            Text(String(format: "%.0f%%", value * 100))
                .font(.caption2)
                .foregroundColor(.secondary)
        }
    }

    private var normalizedValue: Double {
        Double((value - range.lowerBound) / (range.upperBound - range.lowerBound))
    }
}
```

### 4.3 Section Views

#### NetworkControlsView

**File:** `UI/Components/NetworkControlsView.swift`

```swift
import SwiftUI

struct NetworkControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("NETWORK")
                .font(.headline)
                .foregroundColor(.secondary)

            TopologyPicker(
                topologyIndex: Binding(
                    get: { Int(parameterTree.global.topology.value) },
                    set: { parameterTree.global.topology.value = Float($0) }
                )
            )

            ParameterSlider(
                value: $parameterTree.global.couplingStrength.value,
                label: "Coupling Strength",
                range: 0.0...1.0,
                unit: nil
            )
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)
    }
}
```

#### TriggersControlsView

**File:** `UI/Components/TriggersControlsView.swift`

```swift
import SwiftUI

struct TriggersControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("TRIGGERS")
                .font(.headline)
                .foregroundColor(.secondary)

            ParameterSlider(
                value: $parameterTree.excitation.pokeStrength.value,
                label: "Strength",
                range: 0.0...1.0,
                unit: nil
            )

            ParameterSlider(
                value: $parameterTree.excitation.pokeDuration.value,
                label: "Duration",
                range: 1.0...50.0,
                unit: "ms"
            )

            // Optional: Manual trigger button
            Button("TRIGGER POKE") {
                // TODO: Implement manual poke trigger
                // Could send MIDI event or set a parameter
            }
            .buttonStyle(.bordered)
            .frame(maxWidth: .infinity)
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)
    }
}
```

#### DriveControlsView

**File:** `UI/Components/DriveControlsView.swift`

```swift
import SwiftUI

struct DriveControlsView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            Text("OUTPUT")
                .font(.headline)
                .foregroundColor(.secondary)

            HStack(spacing: 40) {
                // Master gain knob
                ParameterKnob(
                    value: $parameterTree.global.masterGain.value,
                    label: "Gain",
                    range: 0.0...1.0
                )

                // Drive slider (if parameter added)
                // Uncomment when drive parameter is implemented:
                /*
                VStack {
                    ParameterSlider(
                        value: $parameterTree.global.drive.value,
                        label: "Drive",
                        range: 0.0...1.0,
                        unit: nil
                    )
                }
                */
            }
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)
    }
}
```

### 4.4 Parameter Tree Integration

**Required Infrastructure:**

The `ParameterTree` type needs to be created to bridge SwiftUI and `AUParameterTree`. This is **not currently defined** but follows the pattern suggested in the README.

**Proposed Implementation:**

**File:** `UI/Utilities/ParameterTree.swift`

```swift
import Foundation
import AudioToolbox
import Combine

@MainActor
class ParameterTree: ObservableObject {
    let global: GlobalParameters
    let excitation: ExcitationParameters
    // Add other parameter groups as needed

    init(auParameterTree: AUParameterTree) {
        self.global = GlobalParameters(auParameterTree: auParameterTree)
        self.excitation = ExcitationParameters(auParameterTree: auParameterTree)
    }

    // For previews
    static var preview: ParameterTree {
        // Create mock AUParameterTree
        let spec = ModalEffectExtensionParameterSpecs
        let tree = spec.createAUParameterTree()
        return ParameterTree(auParameterTree: tree)
    }
}

@MainActor
class GlobalParameters: ObservableObject {
    @Published var masterGain: ParameterWrapper
    @Published var couplingStrength: ParameterWrapper
    @Published var topology: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        // Fetch parameters from tree by identifier
        let gain = auParameterTree.parameter(withID: "global.masterGain")!
        let coupling = auParameterTree.parameter(withID: "global.couplingStrength")!
        let topo = auParameterTree.parameter(withID: "global.topology")!

        self.masterGain = ParameterWrapper(parameter: gain)
        self.couplingStrength = ParameterWrapper(parameter: coupling)
        self.topology = ParameterWrapper(parameter: topo)
    }
}

@MainActor
class ExcitationParameters: ObservableObject {
    @Published var pokeStrength: ParameterWrapper
    @Published var pokeDuration: ParameterWrapper

    init(auParameterTree: AUParameterTree) {
        let strength = auParameterTree.parameter(withID: "excitation.pokeStrength")!
        let duration = auParameterTree.parameter(withID: "excitation.pokeDuration")!

        self.pokeStrength = ParameterWrapper(parameter: strength)
        self.pokeDuration = ParameterWrapper(parameter: duration)
    }
}

@MainActor
class ParameterWrapper: ObservableObject {
    private let parameter: AUParameter

    @Published var value: Float {
        didSet {
            parameter.value = value
        }
    }

    init(parameter: AUParameter) {
        self.parameter = parameter
        self.value = parameter.value

        // Observe parameter changes from DSP/host
        parameter.implementorValueObserver = { [weak self] value in
            DispatchQueue.main.async {
                self?.value = value
            }
        }
    }
}
```

**Integration in AudioUnit:**

The `ModalEffectExtensionAudioUnit.swift` would inject `ParameterTree` into the SwiftUI view hierarchy:

```swift
public var viewController: AUViewController {
    let paramTree = ParameterTree(auParameterTree: self.parameterTree)
    let view = ModalEffectExtensionMainView()
        .environmentObject(paramTree)

    return AUHostingController(rootView: view)
}
```

---

## 5. Implementation Roadmap

### Phase 1: Foundation (Week 1)
**Goal:** Basic UI structure with read-only parameter display

**Tasks:**
1. ✅ Create `/UI` directory structure
2. ✅ Implement `ParameterTree` bridging layer
3. ✅ Build `ModalEffectExtensionMainView` skeleton
4. ✅ Create basic section views (Network, Triggers, Drive)
5. ✅ Implement `ParameterSlider` widget
6. ✅ Test parameter reading from SwiftUI (read-only)

**Deliverable:** UI displays current parameter values from AU parameter tree.

### Phase 2: Interactive Controls (Week 2)
**Goal:** Bidirectional parameter binding

**Tasks:**
1. ✅ Implement parameter write-back from UI to AU
2. ✅ Add `TopologyPicker` component
3. ✅ Add `ParameterKnob` component
4. ✅ Test parameter changes affect DSP (once DSP is hooked up)
5. ✅ Add input validation and range clamping

**Deliverable:** All controls are fully interactive and sync with AU state.

### Phase 3: Drive Parameter (Week 3)
**Goal:** Add optional drive/saturation control

**Tasks:**
1. ✅ Add `kParam_Drive` to parameter addresses enum
2. ✅ Add drive spec to `Parameters.swift`
3. ✅ Implement DSP drive/saturation algorithm
4. ✅ Add drive slider to `DriveControlsView`
5. ✅ Test saturation effect on output

**Deliverable:** Drive parameter fully functional in UI and DSP.

### Phase 4: Polish & UX (Week 4)
**Goal:** Professional UI polish

**Tasks:**
1. ✅ Add color scheme and theming (`UIConstants.swift`)
2. ✅ Improve layout responsiveness (test various window sizes)
3. ✅ Add parameter value formatting (e.g., "30%" vs "0.30")
4. ✅ Add tooltips/help text for parameters
5. ✅ Implement light/dark mode support
6. ✅ Add animations for parameter changes

**Deliverable:** Production-ready config panel UI.

### Phase 5: Advanced Features (Future)
**Optional enhancements:**
- Topology visualization (graph display)
- Per-mode controls expandable panels
- Preset management UI
- MIDI learn for parameters
- Undo/redo support

---

## 6. Technical Considerations

### 6.1 Thread Safety

**Critical:** SwiftUI runs on main thread, DSP on real-time thread.

**Strategy:**
- Use `AUParameter.value` setter (thread-safe by AU framework)
- Never access DSP state directly from UI
- Use `implementorValueObserver` for DSP → UI updates

**Implementation:**
```swift
// ✅ SAFE: Uses AU parameter system
parameter.value = newValue

// ❌ UNSAFE: Direct DSP access
dspKernel.setGain(newValue)
```

### 6.2 Performance

**Goals:**
- UI updates at 60 FPS
- No parameter update lag (<10ms)
- Minimal CPU overhead (<1% on modern Macs)

**Strategies:**
- Use `@Published` sparingly (only for displayed values)
- Batch parameter updates where possible
- Debounce rapid slider changes (e.g., 16ms / 60 FPS)

### 6.3 Accessibility

**Requirements (macOS AU best practices):**
- VoiceOver support for all controls
- Keyboard navigation (tab order)
- High contrast mode support
- Minimum touch target sizes (44×44 pts)

**SwiftUI Support:**
```swift
ParameterSlider(...)
    .accessibilityLabel("Coupling Strength")
    .accessibilityValue("\(Int(value * 100)) percent")
```

### 6.4 Xcode Integration

**Build Requirements:**
- Add SwiftUI files to extension target (not app target)
- Link `SwiftUI.framework`
- Set minimum deployment target (macOS 12.0+)

**Project Settings:**
```
PRODUCT_NAME = ModalEffectExtension
SWIFT_VERSION = 5.9
MACOSX_DEPLOYMENT_TARGET = 12.0
FRAMEWORKS = AudioToolbox, SwiftUI, CoreMIDI
```

---

## 7. Alternative Approaches Considered

### 7.1 AppKit vs SwiftUI

**Decision:** SwiftUI

**Rationale:**
- ✅ Faster development
- ✅ Declarative syntax matches parameter tree structure
- ✅ Built-in state management (`@Published`, `@Binding`)
- ✅ Modern Apple platform standard
- ❌ Limited to macOS 12.0+ (acceptable for new AU)

**AppKit Alternative:**
- Would require manual `NSSlider` → parameter binding
- More boilerplate code
- Better for macOS 10.x support (not needed)

### 7.2 Single-View vs Multi-Tab

**Decision:** Single-view with sections

**Rationale:**
- ✅ All controls visible at once (better for small param set)
- ✅ Simpler navigation
- ❌ May need tabs if adding per-mode controls later

**Multi-Tab Alternative:**
- Tab 1: Network/Global
- Tab 2: Modes (expandable per-mode controls)
- Tab 3: Advanced
- Deferred to Phase 5 if needed

### 7.3 Drive Implementation

**Option A:** Add new `drive` parameter (proposed)
- ✅ Explicit control separation
- ✅ Clearer user intent
- ❌ Requires parameter tree modification

**Option B:** Use `masterGain` with UI-side mapping
- ✅ No parameter changes needed
- ❌ Confusing semantics (gain ≠ drive)
- ❌ Less flexible

**Decision:** Propose Option A, implement Option B as MVP if needed.

---

## 8. Open Questions

1. **Drive Parameter Priority:**
   Should the drive parameter be added in Phase 1 (MVP) or Phase 3 (enhancement)?
   **Recommendation:** Phase 3 (use masterGain only for MVP).

2. **Manual Trigger Button:**
   How should the "Trigger Poke" button send events to DSP?
   **Options:**
   - Send MIDI note-on event
   - Add a momentary parameter (e.g., `kParam_ManualTrigger`)
   - Direct DSP method call (non-AU-compliant)

   **Recommendation:** Add momentary parameter in Phase 2.

3. **Per-Mode Controls:**
   Should the panel expose all 4 modes' frequency/damping/weight controls?
   **Recommendation:** Not in MVP; add expandable sections in Phase 5 if users request.

4. **Preset Integration:**
   Should the panel include a preset browser/selector?
   **Recommendation:** Defer to Phase 5; use DAW's built-in preset system initially.

---

## 9. Success Criteria

**MVP Definition (Phase 1-2):**
- ✅ Network controls (topology + coupling) functional
- ✅ Triggers controls (strength + duration) functional
- ✅ Master gain control functional
- ✅ All parameters sync with AU parameter tree
- ✅ UI loads in Logic Pro X without crashes
- ✅ Parameter changes audible in DSP output

**Production Ready (Phase 4):**
- ✅ All controls polished and responsive
- ✅ Accessible (VoiceOver, keyboard nav)
- ✅ Light/dark mode support
- ✅ No performance regressions (<1% CPU overhead)
- ✅ Passes AU validation (`auval -v aumu`)

---

## 10. Next Steps

### Immediate Actions (This Week)

1. **Approve Proposal:**
   Review this document and approve architecture.

2. **Create UI Directory:**
   ```bash
   mkdir -p ModalEffect/ModalEffectExtension/UI/{Components/Widgets,Utilities}
   ```

3. **Implement Phase 1 Foundation:**
   - `ParameterTree.swift` (bridging layer)
   - `ModalEffectExtensionMainView.swift` (root view)
   - Basic section views (Network, Triggers, Drive)

4. **Add to Xcode Project:**
   - Add new files to extension target
   - Link SwiftUI framework
   - Test in Logic Pro X

### Future Milestones

- **Week 2:** Phase 2 (interactive controls) complete
- **Week 3:** Phase 3 (drive parameter) complete
- **Week 4:** Phase 4 (polish) complete
- **Week 5+:** Phase 5 (advanced features) as needed

---

## Appendix A: Parameter Reference

Complete mapping of existing parameters to UI controls:

| Parameter | Group | Address | UI Control | Section |
|-----------|-------|---------|------------|---------|
| `masterGain` | global | 0 | Rotary knob | Drive |
| `couplingStrength` | global | 1 | Horizontal slider | Network |
| `topology` | global | 2 | Segmented picker | Network |
| `mode0Frequency` | mode0 | 3 | *(Phase 5)* | Modes |
| `mode0Damping` | mode0 | 4 | *(Phase 5)* | Modes |
| `mode0Weight` | mode0 | 5 | *(Phase 5)* | Modes |
| `mode1Frequency` | mode1 | 6 | *(Phase 5)* | Modes |
| `mode1Damping` | mode1 | 7 | *(Phase 5)* | Modes |
| `mode1Weight` | mode1 | 8 | *(Phase 5)* | Modes |
| `mode2Frequency` | mode2 | 9 | *(Phase 5)* | Modes |
| `mode2Damping` | mode2 | 10 | *(Phase 5)* | Modes |
| `mode2Weight` | mode2 | 11 | *(Phase 5)* | Modes |
| `mode3Frequency` | mode3 | 12 | *(Phase 5)* | Modes |
| `mode3Damping` | mode3 | 13 | *(Phase 5)* | Modes |
| `mode3Weight` | mode3 | 14 | *(Phase 5)* | Modes |
| `pokeStrength` | excitation | 15 | Horizontal slider | Triggers |
| `pokeDuration` | excitation | 16 | Horizontal slider | Triggers |
| `polyphony` | voice | 17 | *(Read-only)* | *(Info panel)* |
| `personality` | voice | 18 | *(Phase 5)* | Voice |
| `drive` | global | *(19, proposed)* | Horizontal slider | Drive |

---

## Appendix B: Code Snippets

### B.1 Complete ModalEffectExtensionMainView.swift

See Section 4.1 above.

### B.2 Adding Drive Parameter

**Step 1:** Update `ModalEffectExtensionParameterAddresses.h`
```c
typedef NS_ENUM(AUParameterAddress, ModalEffectExtensionParameterAddress) {
    // ... existing parameters ...
    kParam_Personality = 18,
    kParam_Drive = 19  // NEW
};
```

**Step 2:** Update `Parameters.swift`
```swift
ParameterGroupSpec(identifier: "global", name: "Global") {
    // ... existing parameters ...
    ParameterSpec(
        address: .kParam_Drive,
        identifier: "drive",
        name: "Drive",
        units: .generic,
        valueRange: 0.0...1.0,
        defaultValue: 0.0
    )
}
```

**Step 3:** Update `ModalEffectExtensionDSPKernel.hpp`
```cpp
// In setParameter:
case kParam_Drive:
    mDrive = value;
    break;

// In getParameter:
case kParam_Drive:
    return mDrive;

// Member variable:
float mDrive = 0.0f;

// In process():
float applyDrive(float sample) {
    return std::tanh(mDrive * 3.0f * sample);
}
```

---

## Appendix C: Visual Mockup

```
┌───────────────────────────────────────────────┐
│  Modal Attractors Extension                  │
├───────────────────────────────────────────────┤
│                                               │
│  ┌─────────────────────────────────────────┐ │
│  │ NETWORK                                 │ │
│  ├─────────────────────────────────────────┤ │
│  │ Topology: [Ring ▼]                     │ │
│  │                                         │ │
│  │ Coupling  [====○--------] 30%          │ │
│  └─────────────────────────────────────────┘ │
│                                               │
│  ┌─────────────────────────────────────────┐ │
│  │ TRIGGERS                                │ │
│  ├─────────────────────────────────────────┤ │
│  │ Strength  [=====○-------] 50%          │ │
│  │ Duration  [===○---------] 10.0 ms      │ │
│  │                                         │ │
│  │        [ TRIGGER POKE ]                 │ │
│  └─────────────────────────────────────────┘ │
│                                               │
│  ┌─────────────────────────────────────────┐ │
│  │ OUTPUT                                  │ │
│  ├─────────────────────────────────────────┤ │
│  │    Gain          Drive                  │ │
│  │    ┌───┐    [====○----] 0%             │ │
│  │    │ ◯ │                                │ │
│  │    └───┘                                │ │
│  │     70%                                 │ │
│  └─────────────────────────────────────────┘ │
│                                               │
└───────────────────────────────────────────────┘
```

---

## Conclusion

This proposal provides a complete roadmap for implementing a SwiftUI-based configuration panel for the Modal Attractors AU Extension. The design leverages the existing parameter system, follows Apple's AU best practices, and provides a clear phased implementation strategy.

**Key Takeaways:**
- All foundational infrastructure (parameter tree) exists
- UI layer needs to be created from scratch in extension target
- Network and Triggers controls map directly to existing parameters
- Drive parameter is optional enhancement (can defer to Phase 3)
- Phased approach ensures stable, testable progress

**Approval Checklist:**
- [ ] Architecture approved
- [ ] Drive parameter decision (Phase 1 vs Phase 3)
- [ ] Manual trigger button approach (momentary parameter vs MIDI)
- [ ] Phase 1 implementation authorized

---

**Document Version:** 1.0
**Last Updated:** January 7, 2026
**Authors:** Claude (AI), Carsten Bund (Project Lead)
