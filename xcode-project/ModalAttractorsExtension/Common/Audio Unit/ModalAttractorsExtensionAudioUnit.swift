//
//  ModalAttractorsExtensionAudioUnit.swift
//  ModalAttractorsExtension
//
//  Created by Carsten on 1/7/26.
//

import AVFoundation
import CoreAudioKit
import AudioToolbox
import os

private let log = Logger(subsystem: "com.bund.media.ModalAttractorsExtension", category: "AudioUnit")

// MARK: - FourCharCode Extension for Debugging

extension UInt32 {
    /// Convert FourCharCode to readable string (e.g., 'aumi' from 1635085673)
    var fourCharString: String {
        let bytes: [UInt8] = [
            UInt8((self >> 24) & 0xFF),
            UInt8((self >> 16) & 0xFF),
            UInt8((self >> 8) & 0xFF),
            UInt8(self & 0xFF)
        ]
        return String(bytes: bytes, encoding: .utf8) ?? "????"
    }
}

/// AUv3 Instrument implementation for Modal Attractors synthesis engine
///
/// This class implements the Audio Unit wrapper around the C++ DSP engine,
/// providing sample-accurate MIDI and parameter event processing.
public class ModalAttractorsExtensionAudioUnit: AUAudioUnit, @unchecked Sendable {

    // MARK: - DSP Engine

    /// C++ DSP engine handle (managed via C API)
    private var engine: UnsafeMutablePointer<ModalAttractorsEngine>?

    /// RT-safe render block (built once, does not capture self)
    private var _renderBlock: AUInternalRenderBlock!

    // MARK: - Bus Configuration

    private var outputBus: AUAudioUnitBus?
    private var _outputBusses: AUAudioUnitBusArray!

    private var format: AVAudioFormat

    // MARK: - Parameter Tree

    private var _parameterTree: AUParameterTree?

    // MARK: - Constants

    private let maxPolyphony: UInt32 = 16
    private let defaultSampleRate: Double = 44100

    // MARK: - Initialization

    @objc override init(componentDescription: AudioComponentDescription,
                       options: AudioComponentInstantiationOptions) throws {

        log.info("🔷 DEBUG: AudioUnit.init() - Initializing with component description")
        NSLog("🔷 AUv3 DEBUG: AudioUnit init - type=\(componentDescription.componentType.fourCharString), subtype=\(componentDescription.componentSubType.fourCharString), mfr=\(componentDescription.componentManufacturer.fourCharString)")

        // Create default stereo format
        self.format = AVAudioFormat(standardFormatWithSampleRate: defaultSampleRate, channels: 2)!

        // Call super
        try super.init(componentDescription: componentDescription, options: options)

        // DIAGNOSTIC: Check if the system recognizes this component has a custom view
        checkCustomViewCapability(componentDescription: componentDescription)

        // Create output bus (instrument has no input bus)
        outputBus = try AUAudioUnitBus(format: self.format)
        outputBus?.maximumChannelCount = 2
        _outputBusses = AUAudioUnitBusArray(audioUnit: self,
                                           busType: .output,
                                           busses: [outputBus!])

        // Allocate and initialize DSP engine
        let enginePtr = UnsafeMutablePointer<ModalAttractorsEngine>.allocate(capacity: 1)
        self.engine = enginePtr
        modal_attractors_engine_init(
            enginePtr,
            defaultSampleRate,
            512, // max frames to render
            maxPolyphony
        )

        // Build RT-safe render block once (no ARC in render thread)
        _renderBlock = Self.makeRenderBlock(enginePtr: enginePtr)

        // Create parameter tree internally
        _parameterTree = ModalAttractorsExtensionParameterSpecs.createAUParameterTree()

        // Set default values from parameter tree
        if let paramTree = _parameterTree {
            for param in paramTree.allParameters {
                modal_attractors_engine_set_parameter(engine, UInt32(param.address), param.value)
            }
            setupParameterCallbacks(paramTree)
        }
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

    public override var parameterTree: AUParameterTree? {
        get { return _parameterTree }
        set { _parameterTree = newValue }
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

    // MARK: - Parameter Tree Callbacks

    private func setupParameterCallbacks(_ paramTree: AUParameterTree) {

        // Called when a parameter changes (from UI or host automation)
        // NOTE: this is not sample-accurate; sample-accurate automation comes via render events.
        paramTree.implementorValueObserver = { [weak self] param, value in
            guard let self = self, let engine = self.engine else { return }
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

            switch param.unit {
            case .linearGain:
                return String(format: "%.2f", value)
            case .milliseconds:
                return String(format: "%.1f ms", value)
            case .indexed:
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

    private func ensureParameterTree() -> AUParameterTree {
        if let paramTree = _parameterTree {
            return paramTree
        }

        let paramTree = ModalAttractorsExtensionParameterSpecs.createAUParameterTree()
        _parameterTree = paramTree

        if let engine = engine {
            for param in paramTree.allParameters {
                modal_attractors_engine_set_parameter(engine, UInt32(param.address), param.value)
            }
        }

        setupParameterCallbacks(paramTree)
        return paramTree
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

    public override var internalRenderBlock: AUInternalRenderBlock {
        _renderBlock
    }

    // MARK: - View Controller (in-process hosting)

    public override func requestViewController(
      completionHandler: @escaping (AUViewController?) -> Void
    ) {
    #if os(macOS)
      log.info("⚡️ DEBUG: requestViewController called - Host using IN-PROCESS mode")
      NSLog("⚡️ AUv3 DEBUG: requestViewController called on AudioUnit (in-process mode)")

      DispatchQueue.main.async { [weak self] in
          guard let self = self else {
              NSLog("⚡️ AUv3 DEBUG: requestViewController - self was nil, returning nil VC")
              completionHandler(nil)
              return
          }

          NSLog("⚡️ AUv3 DEBUG: requestViewController - Creating ModalAttractorsAUViewController")
          let viewController = ModalAttractorsAUViewController()
          viewController.audioUnit = self
          NSLog("⚡️ AUv3 DEBUG: requestViewController - Returning view controller to host")
          completionHandler(viewController)
      }
    #else
      NSLog("⚡️ AUv3 DEBUG: requestViewController called on non-macOS platform - returning nil")
      completionHandler(nil)
    #endif
    }

    // MARK: - Diagnostics

    /// Check if the AudioComponent is recognized as having a custom view
    /// This is critical for hosts to know whether to show generic UI or request custom UI
    private func checkCustomViewCapability(componentDescription: AudioComponentDescription) {
        log.info("🔍 DEBUG: Checking AudioComponent custom view capability...")
        NSLog("🔍 AUv3 DEBUG: Querying AudioComponent for hasCustomView property")

        // Find the AudioComponent matching this description
        var desc = componentDescription
        guard let component = AudioComponentFindNext(nil, &desc) else {
            log.error("🔴 ERROR: Could not find AudioComponent for this description!")
            NSLog("🔴 AUv3 ERROR: AudioComponentFindNext returned nil - component not registered?")
            return
        }

        NSLog("🔍 AUv3 DEBUG: AudioComponent found, checking properties...")

        // Check hasCustomView using AudioComponentCopyConfigurationInfo
        var cfDict: Unmanaged<CFDictionary>?
        let status = AudioComponentCopyConfigurationInfo(component, &cfDict)

        if status == noErr, let dict = cfDict?.takeRetainedValue() as? [String: Any] {
            log.info("🔍 DEBUG: Component configuration dictionary: \(dict)")
            NSLog("🔍 AUv3 DEBUG: Configuration info retrieved successfully")

            // Check for custom view indicator
            if let hasCustomView = dict["hasCustomView"] as? Bool {
                if hasCustomView {
                    log.info("✅ DEBUG: hasCustomView = TRUE - Component reports custom view available!")
                    NSLog("✅ AUv3 DEBUG: *** hasCustomView = TRUE *** - Hosts SHOULD request custom UI")
                } else {
                    log.error("🔴 ERROR: hasCustomView = FALSE - Component does NOT report custom view!")
                    NSLog("🔴 AUv3 ERROR: *** hasCustomView = FALSE *** - This explains generic parameter view!")
                    NSLog("🔴 AUv3 ERROR: Info.plist may be missing NSExtension configuration")
                }
            } else {
                log.warning("⚠️ WARNING: hasCustomView key not found in config dictionary")
                NSLog("⚠️ AUv3 WARNING: hasCustomView property not in configuration info")
                NSLog("⚠️ AUv3 WARNING: Available keys: %@", dict.keys.joined(separator: ", "))
            }

            // Log other useful info
            if let name = dict["name"] as? String {
                NSLog("🔍 AUv3 DEBUG: Component name: %@", name)
            }
            if let manufacturer = dict["manufacturer"] as? String {
                NSLog("🔍 AUv3 DEBUG: Manufacturer: %@", manufacturer)
            }
        } else {
            log.error("🔴 ERROR: AudioComponentCopyConfigurationInfo failed with status: \(status)")
            NSLog("🔴 AUv3 ERROR: Could not get configuration info, status = %d", status)
        }
    }
    
    
    
    /// Build a real-time safe render block.
    /// - Important: Does not capture `self` (avoids ARC traffic on audio thread).
    ///
    
    private static func makeRenderBlock(
        enginePtr: UnsafeMutablePointer<ModalAttractorsEngine>
    ) -> AUInternalRenderBlock {

        let block: AUInternalRenderBlock = { actionFlags,
                                             timestamp,
                                             frameCount,
                                             outputBusNumber,
                                             outputData,
                                             realtimeEventListHead,
                                             pullInputBlock -> AUAudioUnitStatus in

            modal_attractors_engine_begin_events(enginePtr)

            var evtPtr = realtimeEventListHead
            let hostSampleTime = AUEventSampleTime(timestamp.pointee.mSampleTime)

            while let e = evtPtr {
                let ev = e.pointee

                switch ev.head.eventType {
                case .MIDI:
                    let midi = ev.MIDI
                    let offset = Int32(midi.eventSampleTime - hostSampleTime)

                    let d = midi.data
                    let status = d.0
                    let data1  = d.1
                    let data2  = d.2
                    let channel = status & 0x0F  // Extract MIDI channel (0-15)

                    switch status & 0xF0 {
                    case 0x90:
                        if data2 > 0 {
                            modal_attractors_engine_push_note_on(
                                enginePtr, offset, data1, Float(data2) * (1.0 / 127.0), channel
                            )
                        } else {
                            modal_attractors_engine_push_note_off(enginePtr, offset, data1)
                        }
                    case 0x80:
                        modal_attractors_engine_push_note_off(enginePtr, offset, data1)
                    case 0xE0:
                        let bend14 = (Int(data2) << 7) | Int(data1)
                        let bend   = (Float(bend14) - 8192.0) * (1.0 / 8192.0)
                        modal_attractors_engine_push_pitch_bend(enginePtr, offset, bend)
                    default:
                        break
                    }

                case .parameter:
                    let p = ev.parameter
                    let offset = Int32(p.eventSampleTime - hostSampleTime)
                    modal_attractors_engine_push_parameter(
                        enginePtr, offset, UInt32(p.parameterAddress), p.value
                    )

                case .parameterRamp:
                    let r = ev.parameter
                    let offset = Int32(r.eventSampleTime - hostSampleTime)
                    modal_attractors_engine_push_parameter(
                        enginePtr, offset, UInt32(r.parameterAddress), r.value
                    )

                case .MIDISysEx:
                    // SysEx not currently handled
                    break

                case .midiEventList:
                    // MIDI 2.0 event list not currently handled
                    break

                @unknown default:
                    break
                }

                if let next = ev.head.next {
                    evtPtr = UnsafePointer<AURenderEvent>(next)
                } else {
                    evtPtr = nil
                }
            }

            let buffers = UnsafeMutableAudioBufferListPointer(outputData)
            guard buffers.count >= 1 else { return kAudioUnitErr_InvalidProperty }

            if buffers.count >= 2 {
                guard
                    let outL = buffers[0].mData?.assumingMemoryBound(to: Float.self),
                    let outR = buffers[1].mData?.assumingMemoryBound(to: Float.self)
                else { return kAudioUnitErr_InvalidProperty }

                modal_attractors_engine_render(enginePtr, outL, outR, frameCount)
            } else {
                guard
                    let out = buffers[0].mData?.assumingMemoryBound(to: Float.self)
                else { return kAudioUnitErr_InvalidProperty }

                modal_attractors_engine_render(enginePtr, out, out, frameCount)
            }

            return noErr
        }

        return block
    }



    // MARK: - State Management

    public override var fullState: [String : Any]? {
        get {
            guard let engine = engine else { return nil }

            var state: [String: Any] = [:]

            // REQUIRED: Add component identification for AUv3 validation
            // These must match the Info.plist AudioComponents entry
            // Note: type/subtype/manufacturer must be FourCharCode as Int, not String
            state["type"] = Self.fourCharCode("aumi")        // 1635085673
            state["subtype"] = Self.fourCharCode("Test")     // 1413829748
            state["manufacturer"] = Self.fourCharCode("Test") // 1413829748
            state["version"] = 67072

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

    // Helper function to convert 4-character string to FourCharCode integer
    private static func fourCharCode(_ string: String) -> Int {
        let chars = string.utf8
        guard chars.count == 4 else { return 0 }
        var result: Int = 0
        for char in chars {
            result = (result << 8) | Int(char)
        }
        return result
    }

    // NOTE: UI Integration is handled by ModalAttractorsAUViewController
    // which is the principal class and conforms to AUAudioUnitFactory.
    // The system automatically uses the principal class for view controller requests.
}
