//
//  ModalAttractorsExtensionAudioUnit.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

import AVFoundation
import CoreAudioKit

/// AUv3 Instrument implementation for Modal Attractors synthesis engine
///
/// This class implements the Audio Unit wrapper around the C++ DSP engine,
/// providing sample-accurate MIDI and parameter event processing.
public class ModalAttractorsExtensionAudioUnit: AUAudioUnit, @unchecked Sendable {

    // MARK: - DSP Engine

    /// C++ DSP engine handle (managed via C API)
    private var engine: UnsafeMutablePointer<ModalAttractorsEngine>?

    // MARK: - Bus Configuration

    private var outputBus: AUAudioUnitBus?
    private var _outputBusses: AUAudioUnitBusArray!

    private var format: AVAudioFormat

    // MARK: - Constants

    private let maxPolyphony: UInt32 = 16
    private let defaultSampleRate: Double = 44100

    // MARK: - Initialization

    @objc override init(componentDescription: AudioComponentDescription,
                       options: AudioComponentInstantiationOptions) throws {

        // Create default stereo format
        self.format = AVAudioFormat(standardFormatWithSampleRate: defaultSampleRate, channels: 2)!

        // Call super
        try super.init(componentDescription: componentDescription, options: options)

        // Create output bus (instrument has no input bus)
        outputBus = try AUAudioUnitBus(format: self.format)
        outputBus?.maximumChannelCount = 2
        _outputBusses = AUAudioUnitBusArray(audioUnit: self,
                                           busType: .output,
                                           busses: [outputBus!])

        // Allocate and initialize DSP engine
        engine = UnsafeMutablePointer<ModalAttractorsEngine>.allocate(capacity: 1)
        modal_attractors_engine_init(
            engine,
            defaultSampleRate,
            512, // max frames to render
            maxPolyphony
        )
    }

    deinit {
        // Clean up DSP engine
        if let engine = engine {
            modal_attractors_engine_cleanup(engine)
            engine.deallocate()
        }
    }

    // MARK: - AUAudioUnit Overrides

    public override var outputBusses: AUAudioUnitBusArray {
        return _outputBusses
    }

    public override var maximumFramesToRender: AUAudioFrameCount {
        get {
            return super.maximumFramesToRender
        }
        set {
            super.maximumFramesToRender = newValue

            // Update engine with new max frames
            if let engine = engine, let bus = outputBus {
                modal_attractors_engine_prepare(
                    engine,
                    bus.format.sampleRate,
                    UInt32(newValue)
                )
            }
        }
    }

    // MARK: - Parameter Tree

    public func setupParameterTree(_ parameterTree: AUParameterTree) {
        self.parameterTree = parameterTree

        // Set default values from parameter tree
        guard let engine = engine else { return }

        for param in parameterTree.allParameters {
            modal_attractors_engine_set_parameter(engine, UInt32(param.address), param.value)
        }

        setupParameterCallbacks()
    }

    private func setupParameterCallbacks() {
        guard let paramTree = parameterTree else { return }

        // Called when a parameter changes (from UI or host automation)
        paramTree.implementorValueObserver = { [weak self] param, value in
            guard let self = self, let engine = self.engine else { return }

            // Update engine parameter immediately (non-sample-accurate)
            // For sample-accurate updates, the render block will handle automation events
            modal_attractors_engine_set_parameter(engine, UInt32(param.address), value)
        }

        // Called when the value needs to be read
        paramTree.implementorValueProvider = { [weak self] param in
            guard let self = self, let engine = self.engine else { return 0 }
            return modal_attractors_engine_get_parameter(engine, UInt32(param.address))
        }

        // String representation of parameter values
        paramTree.implementorStringFromValueCallback = { param, valuePtr in
            guard let value = valuePtr?.pointee else { return "-" }

            // Format based on parameter type
            switch param.unit {
            case .linearGain:
                return String(format: "%.2f", value)
            case .milliseconds:
                return String(format: "%.1f ms", value)
            case .indexed:
                // Use value strings if available
                if let valueStrings = param.valueStrings,
                   Int(value) < valueStrings.count {
                    return valueStrings[Int(value)]
                }
                return String(format: "%.0f", value)
            default:
                return String(format: "%.2f", value)
            }
        }
    }

    // MARK: - Resource Management

    public override func allocateRenderResources() throws {
        guard let bus = outputBus else {
            throw NSError(domain: NSOSStatusErrorDomain,
                         code: Int(kAudioUnitErr_FormatNotSupported))
        }

        // Update format
        format = bus.format

        // Prepare engine with current format
        if let engine = engine {
            modal_attractors_engine_prepare(
                engine,
                format.sampleRate,
                UInt32(maximumFramesToRender)
            )
        }

        try super.allocateRenderResources()
    }

    public override func deallocateRenderResources() {
        // Reset engine state
        if let engine = engine {
            modal_attractors_engine_reset(engine)
        }

        super.deallocateRenderResources()
    }

    // MARK: - Rendering

    /// Sample-accurate render block implementation
    ///
    /// This implements the pattern recommended for AUv3 instruments:
    /// 1. Parse AURenderEvent list
    /// 2. Push events to queue with sample offsets
    /// 3. Render audio with sample-accurate event processing
    public override var internalRenderBlock: AUInternalRenderBlock {
        return { [weak self] (
            actionFlags,
            timestamp,
            frameCount,
            outputBusNumber,
            outputData,
            realtimeEventListHead,
            pullInputBlock
        ) in
            guard let self = self, let engine = self.engine else {
                // Return silence if not initialized
                return kAudioUnitErr_Uninitialized
            }

            // Clear event queue for this render frame
            modal_attractors_engine_begin_events(engine)

            // Parse AU events and push to queue with sample offsets
            var event = realtimeEventListHead
            while let currentEvent = event?.pointee {

                switch currentEvent.head.eventType {

                // MARK: MIDI Events
                case .MIDI:
                    let midiEvent = currentEvent.MIDI
                    let offset = Int32(midiEvent.eventSampleTime - timestamp.pointee.mSampleTime)

                    let data = midiEvent.data
                    let status = data.0
                    let data1 = data.1
                    let data2 = data.2

                    let messageType = status & 0xF0

                    switch messageType {
                    case 0x90: // Note On
                        if data2 > 0 {
                            // Note On with velocity
                            let velocity = Float(data2) / 127.0
                            modal_attractors_engine_push_note_on(
                                engine,
                                offset,
                                data1,
                                velocity
                            )
                        } else {
                            // Note On with velocity 0 = Note Off
                            modal_attractors_engine_push_note_off(engine, offset, data1)
                        }

                    case 0x80: // Note Off
                        modal_attractors_engine_push_note_off(engine, offset, data1)

                    case 0xE0: // Pitch Bend
                        // Combine data1 (LSB) and data2 (MSB) into 14-bit value
                        let bendValue14bit = (Int(data2) << 7) | Int(data1)
                        // Convert to -1.0...+1.0 range (8192 is center)
                        let bendNormalized = (Float(bendValue14bit) - 8192.0) / 8192.0
                        modal_attractors_engine_push_pitch_bend(engine, offset, bendNormalized)

                    case 0xB0: // Control Change
                        // Could map CC to parameters here if needed
                        break

                    default:
                        break
                    }

                // MARK: Parameter Events
                case .parameter:
                    let paramEvent = currentEvent.parameter
                    let offset = Int32(paramEvent.eventSampleTime - timestamp.pointee.mSampleTime)

                    // Push parameter change with sample offset for sample-accurate automation
                    modal_attractors_engine_push_parameter(
                        engine,
                        offset,
                        UInt32(paramEvent.parameterAddress),
                        paramEvent.value
                    )

                case .parameterRamp:
                    let rampEvent = currentEvent.parameterRamp
                    let offset = Int32(rampEvent.eventSampleTime - timestamp.pointee.mSampleTime)

                    // For now, just apply the start value
                    // A more sophisticated implementation would interpolate over the ramp duration
                    modal_attractors_engine_push_parameter(
                        engine,
                        offset,
                        UInt32(rampEvent.parameterAddress),
                        rampEvent.startValue
                    )

                case .MIDIEventList:
                    // Handle new MIDI 2.0 event list format if needed
                    break

                @unknown default:
                    break
                }

                event = currentEvent.head.next?.pointee
            }

            // Get output buffer pointers
            guard let outputBufferList = UnsafeMutableAudioBufferListPointer(outputData) else {
                return kAudioUnitErr_InvalidProperty
            }

            // Render audio with sample-accurate events
            if outputBufferList.count >= 2 {
                // Stereo output
                let outL = outputBufferList[0].mData?.assumingMemoryBound(to: Float.self)
                let outR = outputBufferList[1].mData?.assumingMemoryBound(to: Float.self)

                modal_attractors_engine_render(engine, outL, outR, frameCount)
            } else if outputBufferList.count == 1 {
                // Mono output (use same buffer for both channels)
                let outL = outputBufferList[0].mData?.assumingMemoryBound(to: Float.self)
                modal_attractors_engine_render(engine, outL, outL, frameCount)
            }

            return noErr
        }
    }

    // MARK: - State Management

    public override var fullState: [String : Any]? {
        get {
            guard let engine = engine else { return nil }

            var state: [String: Any] = [:]

            // Save all parameter values
            if let paramTree = parameterTree {
                for param in paramTree.allParameters {
                    let value = modal_attractors_engine_get_parameter(
                        engine,
                        UInt32(param.address)
                    )
                    state[param.identifier] = value
                }
            }

            return state
        }
        set {
            guard let engine = engine, let newState = newValue else { return }

            // Restore parameter values
            if let paramTree = parameterTree {
                for param in paramTree.allParameters {
                    if let value = newState[param.identifier] as? Float {
                        modal_attractors_engine_set_parameter(
                            engine,
                            UInt32(param.address),
                            value
                        )
                        param.value = value
                    }
                }
            }
        }
    }
}
