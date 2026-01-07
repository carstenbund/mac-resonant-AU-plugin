/**
 * @file ModalAttractorsAU.mm
 * @brief AUv3 Audio Unit implementation for Modal Attractors
 *
 * Modern Audio Unit v3 implementation for macOS/iOS.
 * Inherits from AUAudioUnit (not legacy AUInstrumentBase).
 */

#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import "ModalAttractorsDSPKernel.h"
#import "ModalParameters.h"

// ============================================================================
// MARK: - ModalAttractorsAU Interface
// ============================================================================

@interface ModalAttractorsAU : AUAudioUnit

@property (nonatomic) ModalAttractorsDSPKernel *kernel;
@property AUAudioUnitBus *outputBus;
@property AUAudioUnitBusArray *outputBusArray;
@property AUParameterTree *parameterTree;

@end

// ============================================================================
// MARK: - ModalAttractorsAU Implementation
// ============================================================================

@implementation ModalAttractorsAU

/**
 * Initialize the Audio Unit
 */
- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError **)outError {
    self = [super initWithComponentDescription:componentDescription
                                        options:options
                                          error:outError];

    if (self == nil) {
        return nil;
    }

    // Create DSP kernel (C++ object)
    _kernel = new ModalAttractorsDSPKernel();

    // Set default format: stereo, 48kHz
    AVAudioFormat *defaultFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:48000.0
                                                                                   channels:2];

    // Create output bus
    _outputBus = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat error:nil];
    _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                              busType:AUAudioUnitBusTypeOutput
                                                               busses:@[_outputBus]];

    // Create parameter tree
    [self createParameterTree];

    // Set maximum frames to render
    self.maximumFramesToRender = 512;

    return self;
}

/**
 * Cleanup
 */
- (void)dealloc {
    if (_kernel) {
        delete _kernel;
        _kernel = nullptr;
    }
}

// ============================================================================
// MARK: - Parameter Tree Creation
// ============================================================================

- (void)createParameterTree {
    // Create parameters
    AUParameter *masterGain = [AUParameterTree createParameterWithIdentifier:@"masterGain"
                                                                         name:@(kParamName_MasterGain)
                                                                      address:kParam_MasterGain
                                                                          min:kMasterGain_Min
                                                                          max:kMasterGain_Max
                                                                         unit:kAudioUnitParameterUnit_LinearGain
                                                                     unitName:nil
                                                                        flags:kAudioUnitParameterFlag_IsWritable |
                                                                              kAudioUnitParameterFlag_IsReadable
                                                                 valueStrings:nil
                                                          dependentParameters:nil];

    AUParameter *couplingStrength = [AUParameterTree createParameterWithIdentifier:@"couplingStrength"
                                                                               name:@(kParamName_CouplingStrength)
                                                                            address:kParam_CouplingStrength
                                                                                min:kCouplingStrength_Min
                                                                                max:kCouplingStrength_Max
                                                                               unit:kAudioUnitParameterUnit_Generic
                                                                           unitName:nil
                                                                              flags:kAudioUnitParameterFlag_IsWritable |
                                                                                    kAudioUnitParameterFlag_IsReadable
                                                                       valueStrings:nil
                                                                dependentParameters:nil];

    AUParameter *topology = [AUParameterTree createParameterWithIdentifier:@"topology"
                                                                       name:@(kParamName_Topology)
                                                                    address:kParam_Topology
                                                                        min:kTopology_Min
                                                                        max:kTopology_Max
                                                                       unit:kAudioUnitParameterUnit_Indexed
                                                                   unitName:nil
                                                                      flags:kAudioUnitParameterFlag_IsWritable |
                                                                            kAudioUnitParameterFlag_IsReadable
                                                               valueStrings:@[@"Ring", @"Small World", @"Clustered",
                                                                             @"Hub-Spoke", @"Random", @"Complete", @"None"]
                                                        dependentParameters:nil];

    AUParameter *polyphony = [AUParameterTree createParameterWithIdentifier:@"polyphony"
                                                                        name:@(kParamName_Polyphony)
                                                                     address:kParam_Polyphony
                                                                         min:kPolyphony_Min
                                                                         max:kPolyphony_Max
                                                                        unit:kAudioUnitParameterUnit_Generic
                                                                    unitName:@"voices"
                                                                       flags:kAudioUnitParameterFlag_IsWritable |
                                                                             kAudioUnitParameterFlag_IsReadable
                                                                valueStrings:nil
                                                         dependentParameters:nil];

    // Set default values
    masterGain.value = kMasterGain_Default;
    couplingStrength.value = kCouplingStrength_Default;
    topology.value = kTopology_Default;
    polyphony.value = kPolyphony_Default;

    // Create parameter tree
    _parameterTree = [AUParameterTree createTreeWithChildren:@[
        masterGain,
        couplingStrength,
        topology,
        polyphony
    ]];

    // Set up parameter observation
    __weak typeof(self) weakSelf = self;
    _parameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
        typeof(self) strongSelf = weakSelf;
        if (strongSelf && strongSelf.kernel) {
            strongSelf.kernel->setParameter(param.address, value);
        }
    };

    _parameterTree.implementorValueProvider = ^AUValue(AUParameter *param) {
        typeof(self) strongSelf = weakSelf;
        if (strongSelf && strongSelf.kernel) {
            return strongSelf.kernel->getParameter(param.address);
        }
        return 0.0;
    };
}

// ============================================================================
// MARK: - AUAudioUnit Overrides
// ============================================================================

- (AUAudioUnitBusArray *)outputBusses {
    return _outputBusArray;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError {
    if (![super allocateRenderResourcesAndReturnError:outError]) {
        return NO;
    }

    // Initialize DSP kernel with current format
    AVAudioFormat *format = _outputBus.format;
    _kernel->init(format.sampleRate, format.channelCount);

    return YES;
}

- (void)deallocateRenderResources {
    _kernel->cleanup();
    [super deallocateRenderResources];
}

- (AUInternalRenderBlock)internalRenderBlock {
    // Capture kernel pointer for use in render block
    ModalAttractorsDSPKernel *kernel = _kernel;

    return ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
                             const AudioTimeStamp *timestamp,
                             AVAudioFrameCount frameCount,
                             NSInteger outputBusNumber,
                             AudioBufferList *outputData,
                             const AURenderEvent *realtimeEventListHead,
                             AURenderPullInputBlock pullInputBlock) {

        // Process MIDI events
        const AURenderEvent *event = realtimeEventListHead;
        while (event != nullptr) {
            if (event->head.eventType == AURenderEventMIDI) {
                const AUMIDIEvent *midiEvent = &event->MIDI;
                kernel->handleMIDI(midiEvent->data[0],
                                  midiEvent->data[1],
                                  midiEvent->data[2]);
            } else if (event->head.eventType == AURenderEventParameter ||
                      event->head.eventType == AURenderEventParameterRamp) {
                const AUParameterEvent *paramEvent = &event->parameter;
                kernel->setParameter(paramEvent->parameterAddress,
                                   paramEvent->value);
            }
            event = event->head.next;
        }

        // Render audio
        float *outL = (float *)outputData->mBuffers[0].mData;
        float *outR = (outputData->mNumberBuffers > 1) ?
                      (float *)outputData->mBuffers[1].mData : outL;

        kernel->render(outL, outR, frameCount);

        return noErr;
    };
}

// ============================================================================
// MARK: - MIDI Support
// ============================================================================

- (NSArray<NSNumber *> *)MIDIOutputNames {
    return @[];
}

@end

// ============================================================================
// MARK: - Audio Component Registration
// ============================================================================

/**
 * Factory function to create AU instances
 * Called by AudioComponentInstanceNew()
 */
extern "C" {
    AUAudioUnit *ModalAttractorsAUFactory(const AudioComponentDescription *desc) {
        ModalAttractorsAU *au = [[ModalAttractorsAU alloc] initWithComponentDescription:*desc
                                                                                 options:0
                                                                                   error:nil];
        return au;
    }
}
