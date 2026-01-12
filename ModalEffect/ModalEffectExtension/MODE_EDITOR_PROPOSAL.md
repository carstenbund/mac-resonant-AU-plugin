# Mode Editor with Radio-Button Selection Proposal

**Author:** Claude
**Date:** January 10, 2026
**Target:** ModalEffectExtension UI Layer
**Status:** Proposal for Review

---

## Executive Summary

This proposal outlines the design and implementation of a **radio-button-based mode editor** for the Modal Attractors UI. Instead of displaying 12 individual sliders (3 parameters × 4 modes), we will create a **shared control panel with mode selection**, allowing real-time editing of one mode at a time while keeping all modes accessible.

**Key Benefits:**
- ✅ Reduces UI clutter from 12 sliders to 3 sliders + mode selector
- ✅ Maintains full access to all 4 mode parameters
- ✅ Enables real-time parameter changes during performance
- ✅ Clear visual feedback of which mode is being edited
- ✅ Better workflow for sound designers (focus on one mode at a time)

---

## 1. Problem Statement

### Current Implementation
**File:** `ModalEffectExtensionMainView.swift` (lines 46-49)

```swift
// Mode 0-3 sections
ModeControlsView(mode: parameterTree.mode0, modeLabel: "MODE 0")
ModeControlsView(mode: parameterTree.mode1, modeLabel: "MODE 1")
ModeControlsView(mode: parameterTree.mode2, modeLabel: "MODE 2")
ModeControlsView(mode: parameterTree.mode3, modeLabel: "MODE 3")
```

**Issues:**
- 4 separate panels × 3 sliders each = 12 sliders total
- Takes up significant vertical space
- Hard to focus on editing one mode
- Difficult to compare modes side-by-side
- Not optimized for real-time performance editing

### User Requirements
From branch `templates`:
> "I like the 4 mode settings in the control panel to be active to change the settings - the mode settings are not connected right now, I think it would be helpful to have a radio-button in front of each voice - if selected this voices settings are linked to the panel controls - so I can do real time changes - when I select a different voice - the previous link is dissolved and newly linked to the newly selected one."

**Translation:**
1. Create a **single shared control panel** with 3 sliders (frequency, damping, weight)
2. Add **4 radio buttons** to select which mode (0-3) is currently being edited
3. When a mode is selected, **dynamically bind** the shared sliders to that mode's parameters
4. When switching modes, **unbind** the previous mode and **rebind** to the new mode
5. Enable **real-time parameter changes** while maintaining selection state

---

## 2. Proposed Solution

### 2.1 Architecture Overview

**New Component:** `ModeEditorView`
- Replaces the 4 separate `ModeControlsView` instances
- Contains:
  - Radio button group for mode selection (0-3)
  - Shared parameter control panel (3 sliders)
  - Visual feedback for selected mode

**State Management:**
- `@State var selectedModeIndex: Int` - Tracks which mode is currently selected
- Computed property to get the current mode's parameters
- Dynamic binding to selected mode's frequency, damping, weight

**UI Layout:**
```
┌─────────────────────────────────────────────┐
│ MODAL RESONATORS                            │
├─────────────────────────────────────────────┤
│  ○ Mode 0   ○ Mode 1   ○ Mode 2   ○ Mode 3 │ ← Radio buttons
│                                             │
│  Frequency  [====○--------] 440.0 Hz       │
│  Damping    [===○---------] 0.25           │
│  Weight     [======○------] 0.50           │
│                                             │
│  [Copy to All Modes]  [Reset to Default]   │ ← Optional actions
└─────────────────────────────────────────────┘
```

---

## 3. Detailed Design

### 3.1 Component Structure

**File:** `UI/Components/ModeEditorView.swift` (NEW)

```swift
import SwiftUI

/// Mode editor with radio-button selection
/// Allows editing one mode at a time using a shared control panel
struct ModeEditorView: View {
    @EnvironmentObject var parameterTree: ParameterTree

    /// Currently selected mode index (0-3)
    @State private var selectedModeIndex: Int = 0

    /// Get the currently selected mode's parameters
    private var selectedMode: ModeParameters {
        switch selectedModeIndex {
        case 0: return parameterTree.mode0
        case 1: return parameterTree.mode1
        case 2: return parameterTree.mode2
        case 3: return parameterTree.mode3
        default: return parameterTree.mode0
        }
    }

    var body: some View {
        VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
            // Section header
            Text("MODAL RESONATORS")
                .font(UIConstants.Fonts.sectionTitle)
                .foregroundColor(UIConstants.Colors.textSecondary)

            // Mode selector (radio buttons)
            ModeSelectorView(selectedIndex: $selectedModeIndex)

            Divider()
                .padding(.vertical, UIConstants.Spacing.small)

            // Shared parameter controls
            VStack(alignment: .leading, spacing: UIConstants.Spacing.medium) {
                ParameterSlider(
                    param: selectedMode.frequency,
                    label: "Frequency",
                    showUnit: true,
                    formatString: "%.1f"
                )

                ParameterSlider(
                    param: selectedMode.damping,
                    label: "Damping",
                    showUnit: false,
                    formatString: "%.3f"
                )

                ParameterSlider(
                    param: selectedMode.weight,
                    label: "Weight",
                    showUnit: false,
                    formatString: "%.2f"
                )
            }

            // Optional: Action buttons
            HStack(spacing: UIConstants.Spacing.medium) {
                Button("Copy to All") {
                    copyCurrentModeToAll()
                }
                .buttonStyle(.bordered)
                .font(.caption)

                Spacer()

                Button("Reset") {
                    resetCurrentMode()
                }
                .buttonStyle(.bordered)
                .font(.caption)
            }
            .padding(.top, UIConstants.Spacing.small)
        }
        .padding()
        .background(UIConstants.Colors.sectionBackground)
        .cornerRadius(UIConstants.CornerRadius.medium)
    }

    // MARK: - Actions

    /// Copy current mode's parameters to all other modes
    private func copyCurrentModeToAll() {
        let sourceMode = selectedMode
        let freq = sourceMode.frequency.value
        let damp = sourceMode.damping.value
        let wt = sourceMode.weight.value

        // Copy to all modes
        for i in 0...3 {
            let targetMode: ModeParameters
            switch i {
            case 0: targetMode = parameterTree.mode0
            case 1: targetMode = parameterTree.mode1
            case 2: targetMode = parameterTree.mode2
            case 3: targetMode = parameterTree.mode3
            default: continue
            }

            targetMode.frequency.value = freq
            targetMode.damping.value = damp
            targetMode.weight.value = wt
        }
    }

    /// Reset current mode to default values
    private func resetCurrentMode() {
        // These defaults should match Parameters.swift specs
        // Mode defaults: freq varies by mode, damping=0.1, weight=0.7
        let defaultFrequencies: [Float] = [220.0, 440.0, 880.0, 1760.0]

        selectedMode.frequency.value = defaultFrequencies[selectedModeIndex]
        selectedMode.damping.value = 0.1
        selectedMode.weight.value = 0.7
    }
}

#Preview {
    ModeEditorView()
        .environmentObject(ParameterTree.preview)
        .padding()
        .frame(width: 450)
}
```

### 3.2 Mode Selector Component

**File:** `UI/Components/Widgets/ModeSelectorView.swift` (NEW)

```swift
import SwiftUI

/// Radio-button style mode selector
struct ModeSelectorView: View {
    @Binding var selectedIndex: Int

    private let modeLabels = ["Mode 0", "Mode 1", "Mode 2", "Mode 3"]
    private let modeColors: [Color] = [
        .blue, .green, .orange, .purple
    ]

    var body: some View {
        HStack(spacing: UIConstants.Spacing.medium) {
            ForEach(0..<4) { index in
                ModeRadioButton(
                    label: modeLabels[index],
                    isSelected: selectedIndex == index,
                    color: modeColors[index],
                    action: {
                        withAnimation(.easeInOut(duration: 0.2)) {
                            selectedIndex = index
                        }
                    }
                )
            }
        }
    }
}

/// Individual radio button for mode selection
struct ModeRadioButton: View {
    let label: String
    let isSelected: Bool
    let color: Color
    let action: () -> Void

    var body: some View {
        Button(action: action) {
            HStack(spacing: 6) {
                // Radio button circle
                ZStack {
                    Circle()
                        .strokeBorder(
                            isSelected ? color : Color.gray.opacity(0.5),
                            lineWidth: 2
                        )
                        .frame(width: 16, height: 16)

                    if isSelected {
                        Circle()
                            .fill(color)
                            .frame(width: 10, height: 10)
                    }
                }

                // Label
                Text(label)
                    .font(.callout)
                    .fontWeight(isSelected ? .semibold : .regular)
                    .foregroundColor(isSelected ? color : .secondary)
            }
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(
                RoundedRectangle(cornerRadius: 8)
                    .fill(isSelected ? color.opacity(0.15) : Color.clear)
            )
        }
        .buttonStyle(.plain)
    }
}

#Preview {
    VStack(spacing: 20) {
        ModeSelectorView(selectedIndex: .constant(0))
        ModeSelectorView(selectedIndex: .constant(1))
        ModeSelectorView(selectedIndex: .constant(2))
    }
    .padding()
    .frame(width: 450)
}
```

### 3.3 Integration with Main View

**File:** `ModalEffectExtensionMainView.swift` (MODIFIED)

Replace lines 45-49 with:

```swift
// Mode editor with radio-button selection
ModeEditorView()
```

**Before:**
```swift
// Mode 0-3 sections
ModeControlsView(mode: parameterTree.mode0, modeLabel: "MODE 0")
ModeControlsView(mode: parameterTree.mode1, modeLabel: "MODE 1")
ModeControlsView(mode: parameterTree.mode2, modeLabel: "MODE 2")
ModeControlsView(mode: parameterTree.mode3, modeLabel: "MODE 3")
```

**After:**
```swift
// Mode editor with radio-button selection
ModeEditorView()
```

---

## 4. Technical Implementation Details

### 4.1 Parameter Binding

**Key Mechanism:** Computed property for dynamic binding

```swift
private var selectedMode: ModeParameters {
    switch selectedModeIndex {
    case 0: return parameterTree.mode0
    case 1: return parameterTree.mode1
    case 2: return parameterTree.mode2
    case 3: return parameterTree.mode3
    default: return parameterTree.mode0
    }
}
```

**How it works:**
1. `selectedModeIndex` is a `@State` variable that changes when user clicks a radio button
2. When `selectedModeIndex` changes, SwiftUI re-evaluates `selectedMode`
3. The sliders are bound to `selectedMode.frequency`, `selectedMode.damping`, `selectedMode.weight`
4. This creates a **dynamic binding** that automatically redirects to the newly selected mode

**Thread Safety:**
- All parameter updates go through `ParameterWrapper.value` setter
- This uses `AUParameter.value` which is thread-safe (AU framework handles synchronization)
- No direct DSP access from UI thread

### 4.2 State Management

**Selection State:**
- `@State private var selectedModeIndex: Int = 0`
- Starts with Mode 0 selected by default
- Persists during the session (could be saved to UserDefaults if needed)

**Parameter State:**
- Managed by `ParameterTree` and `ModeParameters` classes
- Uses `@Published` properties for SwiftUI reactivity
- Updates propagate automatically to DSP via AU parameter tree

**Animation:**
```swift
withAnimation(.easeInOut(duration: 0.2)) {
    selectedIndex = index
}
```
- Smooth visual transition when switching modes
- Helps user track which mode is selected

### 4.3 Accessibility

**VoiceOver Support:**
```swift
ModeRadioButton(...)
    .accessibilityLabel("\(label), \(isSelected ? "selected" : "not selected")")
    .accessibilityAddTraits(isSelected ? [.isButton, .isSelected] : .isButton)
```

**Keyboard Navigation:**
- Radio buttons are focusable
- Can use arrow keys to change selection
- Tab key moves between radio group and sliders

---

## 5. Advanced Features

### 5.1 Copy to All Modes

**Use Case:** User finds a great sound on Mode 0 and wants to apply it to all modes as a starting point.

**Implementation:**
```swift
private func copyCurrentModeToAll() {
    let sourceMode = selectedMode
    let freq = sourceMode.frequency.value
    let damp = sourceMode.damping.value
    let wt = sourceMode.weight.value

    // Copy to all modes
    parameterTree.mode0.frequency.value = freq
    parameterTree.mode1.frequency.value = freq
    parameterTree.mode2.frequency.value = freq
    parameterTree.mode3.frequency.value = freq
    // ... same for damping and weight
}
```

**UI:**
- Button labeled "Copy to All"
- Located below the sliders
- Optional confirmation dialog to prevent accidents

### 5.2 Reset to Default

**Use Case:** User wants to start fresh with a mode's default values.

**Implementation:**
```swift
private func resetCurrentMode() {
    let defaultFrequencies: [Float] = [220.0, 440.0, 880.0, 1760.0]

    selectedMode.frequency.value = defaultFrequencies[selectedModeIndex]
    selectedMode.damping.value = 0.1  // From Parameters.swift
    selectedMode.weight.value = 0.7   // From Parameters.swift
}
```

**Note:** Default values should match `Parameters.swift` specifications.

### 5.3 Visual Feedback Enhancements

**Option 1: Color-coded backgrounds**
```swift
.background(
    RoundedRectangle(cornerRadius: 8)
        .fill(modeColors[selectedModeIndex].opacity(0.1))
)
```

**Option 2: Mode indicator in slider labels**
```swift
Text("Frequency (Mode \(selectedModeIndex))")
```

**Option 3: Animated transitions**
```swift
.transition(.asymmetric(
    insertion: .move(edge: .trailing).combined(with: .opacity),
    removal: .move(edge: .leading).combined(with: .opacity)
))
```

---

## 6. Alternative Designs Considered

### 6.1 Radio Buttons vs Picker

**Radio Buttons (Proposed):**
- ✅ More visual
- ✅ Single click to change mode
- ✅ All options visible at once
- ❌ Takes more horizontal space

**Picker (Alternative):**
```swift
Picker("Mode", selection: $selectedModeIndex) {
    Text("Mode 0").tag(0)
    Text("Mode 1").tag(1)
    Text("Mode 2").tag(2)
    Text("Mode 3").tag(3)
}
.pickerStyle(.segmented)
```

- ✅ More compact
- ✅ Native macOS look
- ❌ Less customizable
- ❌ Harder to add color-coding

**Decision:** Radio buttons for better visual feedback and customization.

### 6.2 Horizontal vs Vertical Radio Layout

**Horizontal (Proposed):**
```
○ Mode 0   ○ Mode 1   ○ Mode 2   ○ Mode 3
```
- ✅ Fits better in wide panels
- ✅ Matches typical radio button patterns
- ❌ May not fit on narrow windows

**Vertical (Alternative):**
```
○ Mode 0
○ Mode 1
○ Mode 2
○ Mode 3
```
- ✅ Works on narrow windows
- ❌ Takes more vertical space
- ❌ Less common pattern

**Decision:** Horizontal with responsive fallback to vertical on narrow windows.

### 6.3 Inline Editing vs Modal Dialog

**Inline Editing (Proposed):**
- ✅ Immediate visual feedback
- ✅ Better for real-time performance
- ✅ Simpler user flow

**Modal Dialog (Alternative):**
- ❌ Requires extra clicks
- ❌ Breaks performance workflow
- ❌ More complex state management

**Decision:** Inline editing for real-time usability.

---

## 7. Implementation Roadmap

### Phase 1: Core Functionality (Week 1)

**Tasks:**
1. ✅ Create `ModeSelectorView.swift` with radio buttons
2. ✅ Create `ModeEditorView.swift` with shared control panel
3. ✅ Implement dynamic binding to selected mode
4. ✅ Update `ModalEffectExtensionMainView.swift` to use new component
5. ✅ Test mode switching and parameter updates

**Deliverable:** Basic mode editor with radio-button selection working.

### Phase 2: Visual Enhancements (Week 2)

**Tasks:**
1. ✅ Add color-coding for modes
2. ✅ Add selection animations
3. ✅ Improve layout and spacing
4. ✅ Add visual feedback for selected mode
5. ✅ Test on different window sizes

**Deliverable:** Polished mode editor with smooth animations.

### Phase 3: Advanced Features (Week 3)

**Tasks:**
1. ✅ Implement "Copy to All" button
2. ✅ Implement "Reset" button
3. ✅ Add confirmation dialogs (optional)
4. ✅ Add tooltips explaining features
5. ✅ Test with real AU hosts (Logic Pro, Ableton)

**Deliverable:** Production-ready mode editor with advanced features.

### Phase 4: Optimization & Polish (Week 4)

**Tasks:**
1. ✅ Add accessibility support (VoiceOver)
2. ✅ Add keyboard navigation
3. ✅ Test performance (60 FPS UI updates)
4. ✅ Add unit tests for state management
5. ✅ Update documentation

**Deliverable:** Fully polished, accessible, and performant mode editor.

---

## 8. Testing Strategy

### 8.1 Unit Tests

**State Management:**
```swift
func testModeSelection() {
    let view = ModeEditorView()
    view.selectedModeIndex = 2
    XCTAssertEqual(view.selectedMode.modeNumber, 2)
}
```

**Parameter Binding:**
```swift
func testParameterBinding() {
    // Change mode 0 frequency
    parameterTree.mode0.frequency.value = 440.0

    // Select mode 0
    view.selectedModeIndex = 0

    // Verify binding
    XCTAssertEqual(view.selectedMode.frequency.value, 440.0)
}
```

### 8.2 Integration Tests

**Mode Switching:**
1. Select Mode 0
2. Change frequency to 440 Hz
3. Select Mode 1
4. Verify Mode 1's frequency is unchanged
5. Select Mode 0 again
6. Verify frequency is still 440 Hz (persistence)

**Copy to All:**
1. Select Mode 2
2. Set frequency=1000 Hz, damping=0.5, weight=0.8
3. Click "Copy to All"
4. Verify all 4 modes have the same values

### 8.3 Manual Testing

**Real-time Performance:**
1. Open in Logic Pro X
2. Play a MIDI note
3. Switch modes while note is playing
4. Adjust parameters
5. Verify smooth audio (no clicks, dropouts)

**UI Responsiveness:**
1. Rapidly click different mode radio buttons
2. Verify no lag or stuttering
3. Verify correct mode is always selected
4. Verify sliders update immediately

---

## 9. Success Criteria

**MVP (Phase 1):**
- ✅ Radio-button mode selection works
- ✅ Shared control panel updates when mode changes
- ✅ Parameter changes affect correct mode
- ✅ No crashes or hangs

**Production Ready (Phase 4):**
- ✅ Smooth animations (<16ms frame time)
- ✅ Accessible (VoiceOver, keyboard nav)
- ✅ Works in all major DAWs (Logic, Ableton, Reaper)
- ✅ No audio glitches during mode switching
- ✅ UI matches design mockups

---

## 10. Open Questions

### Q1: Should mode selection persist across sessions?
**Options:**
- Save to UserDefaults
- Always start with Mode 0
- Save in AU state blob

**Recommendation:** Always start with Mode 0 for simplicity.

### Q2: Should we add a "Compare" mode?
**Feature:** Select two modes and A/B compare them.
**Recommendation:** Defer to future version (Phase 5).

### Q3: Should "Copy to All" require confirmation?
**Options:**
- Always confirm (prevent accidents)
- Never confirm (faster workflow)
- Make it a preference

**Recommendation:** No confirmation for MVP, add preference later if users request it.

### Q4: Should we deprecate the old ModeControlsView?
**Options:**
- Delete it entirely
- Keep it for reference
- Make it a toggle option

**Recommendation:** Keep it commented out in the codebase for reference, in case users prefer the old layout.

---

## 11. Migration Path

### Step 1: Create New Components
1. Add `ModeSelectorView.swift`
2. Add `ModeEditorView.swift`
3. Test in isolation with previews

### Step 2: Update Main View
1. Comment out old mode controls:
   ```swift
   // Old mode controls (deprecated)
   // ModeControlsView(mode: parameterTree.mode0, modeLabel: "MODE 0")
   // ModeControlsView(mode: parameterTree.mode1, modeLabel: "MODE 1")
   // ModeControlsView(mode: parameterTree.mode2, modeLabel: "MODE 2")
   // ModeControlsView(mode: parameterTree.mode3, modeLabel: "MODE 3")
   ```

2. Add new mode editor:
   ```swift
   // New mode editor with radio-button selection
   ModeEditorView()
   ```

### Step 3: Test & Validate
1. Build and run in Xcode
2. Test in AU host (Logic Pro X)
3. Verify parameter changes work correctly
4. Get user feedback

### Step 4: Cleanup
1. If approved, delete old `ModeControlsView.swift`
2. Update documentation
3. Commit changes

---

## 12. Visual Mockup

```
┌───────────────────────────────────────────────────────┐
│  MODAL RESONATORS                                     │
├───────────────────────────────────────────────────────┤
│                                                       │
│  ● Mode 0    ○ Mode 1    ○ Mode 2    ○ Mode 3       │
│  ▔▔▔▔▔▔▔                                             │
│                                                       │
│  ─────────────────────────────────────────────────── │
│                                                       │
│  Frequency                                            │
│  [====════○──────────────────────────] 440.0 Hz      │
│                                                       │
│  Damping                                              │
│  [===○───────────────────────────────] 0.100         │
│                                                       │
│  Weight                                               │
│  [======○────────────────────────────] 0.500         │
│                                                       │
│  [Copy to All]                           [Reset]     │
│                                                       │
└───────────────────────────────────────────────────────┘
```

**Color Legend:**
- Mode 0: Blue
- Mode 1: Green
- Mode 2: Orange
- Mode 3: Purple

**When Mode 1 is selected:**
```
┌───────────────────────────────────────────────────────┐
│  MODAL RESONATORS                                     │
├───────────────────────────────────────────────────────┤
│                                                       │
│  ○ Mode 0    ● Mode 1    ○ Mode 2    ○ Mode 3       │
│               ▔▔▔▔▔▔▔                                │
│                                                       │
│  ─────────────────────────────────────────────────── │
│                                                       │
│  Frequency                                            │
│  [══════════════○────────────────────] 880.0 Hz      │
│  ↑ Now editing Mode 1's parameters                   │
│                                                       │
└───────────────────────────────────────────────────────┘
```

---

## 13. Conclusion

This proposal provides a complete design for implementing a radio-button-based mode editor that addresses the user's requirements for real-time mode parameter editing. The design:

- ✅ Reduces UI clutter (12 sliders → 3 sliders + selector)
- ✅ Enables real-time parameter changes
- ✅ Provides clear visual feedback
- ✅ Maintains full access to all 4 modes
- ✅ Follows Apple UI best practices
- ✅ Is performant and thread-safe

**Next Steps:**
1. Review and approve this proposal
2. Begin Phase 1 implementation
3. Test with users and gather feedback
4. Iterate based on feedback

---

**Document Version:** 1.0
**Last Updated:** January 10, 2026
**Author:** Claude (AI Assistant)
**Reviewer:** Carsten Bund (Project Lead)
