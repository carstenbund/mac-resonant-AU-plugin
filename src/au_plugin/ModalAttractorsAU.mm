/**
 * @file ModalAttractorsAU.mm
 * @brief Audio Unit plugin implementation for Modal Attractors
 *
 * This file implements the Audio Unit v3 (AUv3) plugin interface,
 * bridging macOS Audio Unit SDK with the C++ DSP engine.
 *
 * Based on Apple's AUInstrument example and AudioToolbox framework.
 */

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>

#include "../au_wrapper/ModalAttractorsAU.h"
#include "../au_wrapper/ModalParameters.h"
#include "../dsp_core/VoiceAllocator.h"
#include "../dsp_core/TopologyEngine.h"

#include <vector>
#include <memory>

// ============================================================================
// Audio Unit Kernel (DSP Processing)
// ============================================================================

/**
 * @brief Kernel class handles realtime audio processing
 *
 * This runs on the realtime audio thread and must be lock-free.
 */
class ModalAttractorsKernel {
public:
    ModalAttractorsKernel() : engine_{nullptr} {
        engine_ = new ModalAttractorsEngine();
    }

    ~ModalAttractorsKernel() {
        if (engine_) {
            modal_attractors_engine_cleanup(engine_);
            delete engine_;
        }
    }

    void initialize(float sample_rate, uint32_t max_frames) {
        sample_rate_ = sample_rate;
        max_frames_ = max_frames;

        // Initialize engine with 16-voice polyphony
        modal_attractors_engine_init(engine_, sample_rate, 16);
    }

    void reset() {
        if (engine_) {
            modal_attractors_engine_cleanup(engine_);
            modal_attractors_engine_init(engine_, sample_rate_, 16);
        }
    }

    // MIDI event handling
    void handleMIDINoteOn(uint8_t note, uint8_t velocity) {
        modal_attractors_engine_note_on(engine_, note, velocity);
    }

    void handleMIDINoteOff(uint8_t note) {
        modal_attractors_engine_note_off(engine_, note);
    }

    // Parameter updates
    void setParameter(uint32_t param_id, float value) {
        modal_attractors_engine_set_parameter(engine_, param_id, value);
    }

    // Audio rendering
    void process(AudioBufferList* outputBufferList, uint32_t frames_to_render) {
        // Get output buffers
        float* outL = (float*)outputBufferList->mBuffers[0].mData;
        float* outR = (float*)outputBufferList->mBuffers[1].mData;

        // Render audio from DSP engine
        modal_attractors_engine_render(engine_, outL, outR, frames_to_render);
    }

private:
    ModalAttractorsEngine* engine_;
    float sample_rate_ = 48000.0f;
    uint32_t max_frames_ = 512;
};

// ============================================================================
// Audio Unit Class (Objective-C++)
// ============================================================================

@interface ModalAttractorsAudioUnit : AUAudioUnit

@property (nonatomic) AUParameterTree* parameterTree;
@property (nonatomic, readonly) AUAudioUnitBus* inputBus;
@property (nonatomic, readonly) AUAudioUnitBus* outputBus;
@property (nonatomic, readonly) AUAudioUnitBusArray* inputBusArray;
@property (nonatomic, readonly) AUAudioUnitBusArray* outputBusArray;

@end

@implementation ModalAttractorsAudioUnit {
    // DSP Kernel
    ModalAttractorsKernel* _kernel;

    // Busses
    AUAudioUnitBus* _inputBus;
    AUAudioUnitBus* _outputBus;
    AUAudioUnitBusArray* _inputBusArray;
    AUAudioUnitBusArray* _outputBusArray;

    // Parameter objects
    AUParameter* _masterGainParam;
    AUParameter* _couplingStrengthParam;
    AUParameter* _topologyParam;
}

// ----------------------------------------------------------------------------
// MARK: - Initialization
// ----------------------------------------------------------------------------

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError **)outError {
    self = [super initWithComponentDescription:componentDescription
                                       options:options
                                         error:outError];

    if (self == nil) {
        return nil;
    }

    // Initialize DSP kernel
    _kernel = new ModalAttractorsKernel();

    // Setup audio busses (stereo output, no input for instrument)
    AVAudioFormat* defaultFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:48000
                                                                                  channels:2];

    _outputBus = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat error:nil];
    _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                              busType:AUAudioUnitBusTypeOutput
                                                               busses:@[_outputBus]];

    // No input bus for instrument plugins
    _inputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                             busType:AUAudioUnitBusTypeInput
                                                              busses:@[]];

    // Create parameters
    [self setupParameters];

    // Initialize kernel with default settings
    _kernel->initialize(48000.0f, 512);

    return self;
}

- (void)dealloc {
    if (_kernel) {
        delete _kernel;
        _kernel = nullptr;
    }
}

// ----------------------------------------------------------------------------
// MARK: - Parameter Setup
// ----------------------------------------------------------------------------

- (void)setupParameters {
    // Create parameter tree
    NSMutableArray<AUParameterNode*>* parameters = [NSMutableArray array];

    // Master Gain (0.0 - 1.0)
    _masterGainParam = [AUParameterTree createParameterWithIdentifier:@"masterGain"
                                                                  name:@"Master Gain"
                                                               address:kParam_MasterGain
                                                                   min:0.0f
                                                                   max:1.0f
                                                                  unit:kAudioUnitParameterUnit_LinearGain
                                                              unitName:nil
                                                                 flags:kAudioUnitParameterFlag_IsReadable |
                                                                       kAudioUnitParameterFlag_IsWritable
                                                          valueStrings:nil
                                                   dependentParameters:nil];
    _masterGainParam.value = 0.7f;
    [parameters addObject:_masterGainParam];

    // Coupling Strength (0.0 - 1.0)
    _couplingStrengthParam = [AUParameterTree createParameterWithIdentifier:@"couplingStrength"
                                                                       name:@"Coupling Strength"
                                                                    address:kParam_CouplingStrength
                                                                        min:0.0f
                                                                        max:1.0f
                                                                       unit:kAudioUnitParameterUnit_Generic
                                                                   unitName:nil
                                                                      flags:kAudioUnitParameterFlag_IsReadable |
                                                                            kAudioUnitParameterFlag_IsWritable
                                                               valueStrings:nil
                                                        dependentParameters:nil];
    _couplingStrengthParam.value = 0.3f;
    [parameters addObject:_couplingStrengthParam];

    // Topology Type (0 - 6)
    NSArray* topologyNames = @[@"Ring", @"Small World", @"Clustered", @"Hub-Spoke", @"Random", @"Complete", @"None"];
    _topologyParam = [AUParameterTree createParameterWithIdentifier:@"topology"
                                                               name:@"Topology"
                                                            address:kParam_Topology
                                                                min:0.0f
                                                                max:6.0f
                                                               unit:kAudioUnitParameterUnit_Indexed
                                                           unitName:nil
                                                              flags:kAudioUnitParameterFlag_IsReadable |
                                                                    kAudioUnitParameterFlag_IsWritable
                                                       valueStrings:topologyNames
                                                dependentParameters:nil];
    _topologyParam.value = 0.0f; // Ring by default
    [parameters addObject:_topologyParam];

    // TODO: Add mode parameters (frequency, damping, weight) for each of 4 modes

    // Create parameter tree
    _parameterTree = [AUParameterTree createTreeWithChildren:parameters];

    // Setup parameter observers
    __weak typeof(self) weakSelf = self;
    _parameterTree.implementorValueObserver = ^(AUParameter* param, AUValue value) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (strongSelf) {
            strongSelf->_kernel->setParameter(param.address, value);
        }
    };

    _parameterTree.implementorValueProvider = ^AUValue(AUParameter* param) {
        return param.value;
    };
}

// ----------------------------------------------------------------------------
// MARK: - AUAudioUnit Overrides
// ----------------------------------------------------------------------------

- (AUAudioUnitBusArray*)inputBusses {
    return _inputBusArray;
}

- (AUAudioUnitBusArray*)outputBusses {
    return _outputBusArray;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError {
    if (![super allocateRenderResourcesAndReturnError:outError]) {
        return NO;
    }

    // Initialize kernel with actual sample rate and max frames
    AVAudioFormat* outputFormat = _outputBus.format;
    _kernel->initialize(outputFormat.sampleRate, self.maximumFramesToRender);

    return YES;
}

- (void)deallocateRenderResources {
    [super deallocateRenderResources];
    _kernel->reset();
}

- (AUInternalRenderBlock)internalRenderBlock {
    // Capture kernel pointer in block
    ModalAttractorsKernel* kernel = _kernel;

    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags* actionFlags,
                              const AudioTimeStamp* timestamp,
                              AVAudioFrameCount frameCount,
                              NSInteger outputBusNumber,
                              AudioBufferList* outputData,
                              const AURenderEvent* realtimeEventListHead,
                              AURenderPullInputBlock pullInputBlock) {

        // Process render events (MIDI, parameters)
        const AURenderEvent* event = realtimeEventListHead;
        while (event != nullptr) {
            switch (event->head.eventType) {
                case AURenderEventMIDI: {
                    const AUMIDIEvent& midiEvent = event->MIDI;
                    uint8_t status = midiEvent.data[0] & 0xF0;
                    uint8_t channel = midiEvent.data[0] & 0x0F;
                    uint8_t data1 = midiEvent.data[1];
                    uint8_t data2 = midiEvent.data[2];

                    if (status == 0x90 && data2 > 0) {
                        // Note On
                        kernel->handleMIDINoteOn(data1, data2);
                    } else if (status == 0x80 || (status == 0x90 && data2 == 0)) {
                        // Note Off
                        kernel->handleMIDINoteOff(data1);
                    }
                    break;
                }

                case AURenderEventParameter:
                case AURenderEventParameterRamp: {
                    const AUParameterEvent& paramEvent = event->parameter;
                    kernel->setParameter(paramEvent.parameterAddress, paramEvent.value);
                    break;
                }

                default:
                    break;
            }
            event = event->head.next;
        }

        // Render audio
        kernel->process(outputData, frameCount);

        return noErr;
    };
}

// MIDI Support
- (BOOL)shouldBypassEffect {
    return NO;
}

- (NSArray<NSString*>*)MIDIOutputNames {
    return @[];
}

@end

// ============================================================================
// MARK: - Factory Function
// ============================================================================

/**
 * @brief Factory function for Audio Unit instantiation
 *
 * This is the entry point called by the Audio Unit host (Logic Pro X, etc.)
 */
extern "C" {

OSStatus ModalAttractorsAUFactory(const AudioComponentDescription* inDesc) {
    return AUAudioUnit.registerSubclass(ModalAttractorsAudioUnit.class,
                                        *inDesc,
                                        @"ModalAttractors",
                                        1);
}

} // extern "C"
