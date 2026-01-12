# AUv3 Instrument Implementation Guide

This guide explains how to implement the AU wrapper for Modal Attractors following best practices for AUv3 instruments.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│  AU Wrapper (Objective-C++/Swift)                       │
│  - ModalEffectExtensionAudioUnit.swift              │
│  - AUParameterTree creation                             │
│  - AURenderEvent parsing                                │
└──────────────────┬──────────────────────────────────────┘
                   │ C API (Apple-type-free)
                   ▼
┌─────────────────────────────────────────────────────────┐
│  C API Bridge                                           │
│  - ModalEffectEngine.cpp (C functions)              │
│  - Converts AU events → SynthEvent                      │
└──────────────────┬──────────────────────────────────────┘
                   │ C++ API
                   ▼
┌─────────────────────────────────────────────────────────┐
│  SynthEngine (Pure C++, portable)                       │
│  - Sample-accurate event processing                     │
│  - VoiceAllocator, TopologyEngine                       │
│  - All DSP code (no Apple types)                        │
└─────────────────────────────────────────────────────────┘
```

## Critical Requirements for AUv3 Instruments

### 1. Sample-Accurate Event Processing

**Why it matters:** Fast note passages will smear and timing will feel sloppy if you apply all events at the start of each buffer.

**Pattern:**
```swift
// In your render block:
func internalRenderBlock(...) -> AUInternalRenderBlock {
    return { actionFlags, timestamp, frameCount, outputBusNumber, outputData, realtimeEventListHead, pullInputBlock in

        // 1. Clear event queue
        modal_attractors_engine_begin_events(self.engine)

        // 2. Parse AU events and push to queue
        var event = realtimeEventListHead
        while event != nil {
            switch event!.head.eventType {
            case .MIDI:
                let midiEvent = event!.MIDI
                let status = midiEvent.data[0]
                let data1 = midiEvent.data[1]
                let data2 = midiEvent.data[2]
                let offset = midiEvent.eventSampleTime - timestamp

                if (status & 0xF0) == 0x90 && data2 > 0 {
                    // Note On
                    let velocity = Float(data2) / 127.0
                    modal_attractors_engine_push_note_on(
                        self.engine,
                        Int32(offset),
                        data1,
                        velocity
                    )
                } else if (status & 0xF0) == 0x80 || ((status & 0xF0) == 0x90 && data2 == 0) {
                    // Note Off
                    modal_attractors_engine_push_note_off(
                        self.engine,
                        Int32(offset),
                        data1
                    )
                }

            case .parameter:
                let paramEvent = event!.parameter
                let offset = paramEvent.eventSampleTime - timestamp
                modal_attractors_engine_push_parameter(
                    self.engine,
                    Int32(offset),
                    UInt32(paramEvent.parameterAddress),
                    paramEvent.value
                )

            default:
                break
            }
            event = event!.head.next
        }

        // 3. Render with sample-accurate events
        let outputBufferL = outputData.pointee.mBuffers.mData!.assumingMemoryBound(to: Float.self)
        let outputBufferR = // ... get right channel

        modal_attractors_engine_render(
            self.engine,
            outputBufferL,
            outputBufferR,
            frameCount
        )

        return noErr
    }
}
```

### 2. Real-Time Safety Rules

**NEVER in the render thread:**
- ❌ `malloc` / `new` / `delete`
- ❌ `@synchronized` / locks (unless lock-free)
- ❌ Objective-C method calls (except simple getters)
- ❌ File I/O
- ❌ Logging (use os_signpost for debugging)

**The DSP engine:**
- ✅ Pre-allocates all memory in `prepare()`
- ✅ Uses fixed-size event queue (512 events max)
- ✅ Voice pool is allocated at init, reused
- ✅ No sorting needed (events arrive in time order)

### 3. Bus Configuration

```swift
// In your init or factory:
override func allocateRenderResources() throws {
    try super.allocateRenderResources()

    guard let format = outputBus.format else {
        throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
    }

    // Instrument AU: no input bus, stereo output
    let sampleRate = format.sampleRate
    let maxFrames = maximumFramesToRender
    let channels = format.channelCount

    modal_attractors_engine_prepare(
        engine,
        sampleRate,
        UInt32(maxFrames),
        16  // max polyphony
    )
}
```

### 4. Voice Stealing Strategy

The `VoiceAllocator` implements **oldest-voice stealing**:
- When all voices are busy and a new note arrives
- Find the oldest active voice
- Release it and assign to new note

This is deterministic and real-time safe (no allocation).

### 5. Parameter Automation

**Two ways to handle parameters:**

**Option A: Sample-accurate automation (recommended)**
```swift
// Parameter events arrive as AURenderEvent
// Push them with their sample offset
modal_attractors_engine_push_parameter(engine, offset, paramId, value)
```

**Option B: Immediate update (non-automated)**
```swift
// For UI changes that don't need sample accuracy
modal_attractors_engine_set_parameter(engine, paramId, value)
```

### 6. State Saving/Restoration

```swift
override var fullState: [String : Any]? {
    get {
        var state: [String: Any] = [:]

        // Save parameter values
        state["masterGain"] = modal_attractors_engine_get_parameter(engine, 0)
        state["couplingStrength"] = modal_attractors_engine_get_parameter(engine, 1)
        state["topology"] = modal_attractors_engine_get_parameter(engine, 2)

        // Do NOT save transient voice state

        return state
    }
    set {
        guard let newState = newValue else { return }

        // Restore parameters
        if let gain = newState["masterGain"] as? Float {
            modal_attractors_engine_set_parameter(engine, 0, gain)
        }
        // ... restore other parameters
    }
}
```

### 7. MIDI Implementation Checklist

**Minimum required:**
- ✅ Note On (0x90)
- ✅ Note Off (0x80)
- ✅ Note On with velocity 0 (alternate note off)

**Recommended:**
- ✅ Pitch Bend (0xE0) - already supported
- ⚠️ CC (0xB0) - implement if you map mod wheel, etc.
- ⚠️ All Notes Off (CC 123)
- ⚠️ All Sound Off (CC 120)

**Not needed for v1:**
- Program Change
- Aftertouch
- Sysex

### 8. Control Rate vs. Audio Rate

The engine uses a **control rate** of ~500 Hz for:
- Voice parameter updates
- Topology coupling calculations
- Non-critical modulation

This is more efficient than per-sample updates while being responsive enough for human perception.

```cpp
// In SynthEngine.cpp:
static constexpr uint32_t CONTROL_RATE_SAMPLES = 96; // ~500 Hz at 48kHz

void renderSlice(...) {
    controlRateCounter_ += numFrames;
    if (controlRateCounter_ >= CONTROL_RATE_SAMPLES) {
        updateControlRate();  // Update coupling, voice state
        controlRateCounter_ = 0;
    }
    // ... render audio
}
```

## Testing Checklist

### Validation
- [ ] Run `auval -v aumu Modl MyID` (replace with your type/subtype/manufacturer)
- [ ] Should pass all validation tests
- [ ] Check for timing accuracy with fast MIDI sequences

### Performance
- [ ] CPU usage < 10% at 48kHz, 256 buffer size, 16 voices
- [ ] No glitches under heavy polyphony
- [ ] No memory allocations in Activity Monitor during playback

### Compatibility
- [ ] Test in Logic Pro
- [ ] Test in GarageBand
- [ ] Test in Ableton Live
- [ ] Test parameter automation
- [ ] Test save/restore state

## Common Pitfalls Fixed

### ❌ Before (BAD):
```cpp
// MEMORY ALLOCATION IN RENDER!
void render(...) {
    ModalVoice** voices = new ModalVoice*[max_polyphony];  // BAD!
    // ...
    delete[] voices;  // BAD!
}
```

### ✅ After (GOOD):
```cpp
// Pre-allocated in constructor
SynthEngine::SynthEngine(uint32_t maxPolyphony) {
    voicePointers_ = new ModalVoice*[maxPolyphony];  // Once at init
}

void SynthEngine::renderSlice(...) {
    // Use pre-allocated array - no allocation!
    for (uint32_t i = 0; i < maxPolyphony_; i++) {
        voicePointers_[i] = voiceAllocator_->getVoice(i);
    }
}
```

## References

- [Audio Unit Programming Guide](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide)
- [Core Audio Best Practices](https://developer.apple.com/documentation/audiotoolbox/core_audio_essentials)
- WWDC Sessions on Audio Units (search "Audio Unit" on Apple Developer)

## Next Steps

1. Implement the Swift AU wrapper using this guide
2. Test with `auval`
3. Profile with Instruments (look for allocations in render)
4. Test in various DAWs
5. Add UI (parameters are already hooked up)
