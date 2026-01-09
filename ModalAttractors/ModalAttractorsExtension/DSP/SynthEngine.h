/**
 * @file SynthEngine.h
 * @brief Clean interface for the Modal Attractors synthesis engine
 *
 * This provides a real-time safe, Apple-type-free interface between
 * the AU wrapper and the C++ DSP core. Implements the pattern recommended
 * for AUv3 instruments with sample-accurate event handling.
 */

#ifndef SYNTH_ENGINE_H
#define SYNTH_ENGINE_H

#include <cstdint>
#include <cstring>

// Forward declarations - no Apple types leak into DSP
class ModalVoice;
class VoiceAllocator;
class TopologyEngine;

/**
 * @brief Event types for sample-accurate processing
 */
enum class EventType : uint8_t {
    NoteOn,
    NoteOff,
    CC,
    PitchBend,
    Parameter
};

/**
 * @brief Real-time safe event structure
 *
 * All MIDI and parameter events are converted to this format
 * and queued with their sample offset for sample-accurate processing.
 */
struct SynthEvent {
    EventType type;
    int32_t sampleOffset;  // 0 to bufferSize-1

    // Union for different event data
    union {
        struct {
            uint8_t note;
            float velocity;  // 0.0-1.0 normalized
        } noteOn;

        struct {
            uint8_t note;
        } noteOff;

        struct {
            uint8_t cc;
            float value;  // 0.0-1.0 normalized
        } cc;

        struct {
            float value;  // -1.0 to +1.0
        } pitchBend;

        struct {
            uint32_t paramId;
            float value;
        } parameter;
    };
};

/**
 * @brief Fixed-size event queue for real-time safety
 *
 * Pre-allocated queue with no dynamic memory allocation.
 * Size chosen to handle worst-case event density.
 */
class EventQueue {
public:
    static constexpr uint32_t MAX_EVENTS = 512;

    EventQueue() : count_(0) {}

    /**
     * @brief Add event to queue (real-time safe)
     * @param event Event to add
     * @return true if added, false if queue full
     */
    bool push(const SynthEvent& event) {
        if (count_ >= MAX_EVENTS) return false;
        events_[count_++] = event;
        return true;
    }

    /**
     * @brief Get event count
     */
    uint32_t count() const { return count_; }

    /**
     * @brief Get event by index
     */
    const SynthEvent& operator[](uint32_t idx) const {
        return events_[idx];
    }

    /**
     * @brief Clear queue (real-time safe)
     */
    void clear() { count_ = 0; }

private:
    SynthEvent events_[MAX_EVENTS];
    uint32_t count_;
};

/**
 * @brief Main synthesis engine
 *
 * Owns all DSP components and provides a clean interface
 * for the AU wrapper. All methods are real-time safe.
 */
class SynthEngine {
public:
    /**
     * @brief Constructor
     * @param maxPolyphony Maximum number of voices
     */
    SynthEngine(uint32_t maxPolyphony = 16);

    /**
     * @brief Destructor
     */
    ~SynthEngine();

    /**
     * @brief Prepare engine for processing
     * @param sampleRate Sample rate in Hz
     * @param maxFrames Maximum frames per render call
     * @param channels Number of output channels (1 or 2)
     */
    void prepare(double sampleRate, uint32_t maxFrames, uint32_t channels);

    /**
     * @brief Reset engine state (clear all voices)
     */
    void reset();

    /**
     * @brief Process events and render audio (real-time safe)
     * @param events Event queue for this buffer
     * @param outL Left channel output
     * @param outR Right channel output (can be same as outL for mono)
     * @param numFrames Number of frames to render
     */
    void render(const EventQueue& events, float* outL, float* outR, uint32_t numFrames);

    /**
     * @brief Set parameter (real-time safe)
     * @param paramId Parameter ID
     * @param value Parameter value
     */
    void setParameter(uint32_t paramId, float value);

    /**
     * @brief Get parameter (real-time safe)
     * @param paramId Parameter ID
     * @return Current parameter value
     */
    float getParameter(uint32_t paramId) const;

    /**
     * @brief Get maximum polyphony
     */
    uint32_t getMaxPolyphony() const { return maxPolyphony_; }

    /**
     * @brief Get current sample rate
     */
    double getSampleRate() const { return sampleRate_; }

private:
    // DSP components (owned by this class)
    VoiceAllocator* voiceAllocator_;
    TopologyEngine* topologyEngine_;

    // Pre-allocated voice pointer array (no allocation in render!)
    ModalVoice** voicePointers_;

    // Engine state
    uint32_t maxPolyphony_;
    double sampleRate_;
    uint32_t maxFrames_;
    uint32_t channels_;
    bool initialized_;

    // Control rate state (update at ~500 Hz)
    uint32_t controlRateCounter_;
    static constexpr uint32_t CONTROL_RATE_SAMPLES = 96; // ~500 Hz at 48kHz

    // Parameter cache - Global
    float masterGain_;
    float couplingStrength_;
    int topologyType_;
    uint32_t nodeCount_;  // Active node count (1-16), currently stored but not enforced

    // Parameter cache - Per-mode parameters
    float mode0_frequency_;
    float mode0_damping_;
    float mode0_weight_;
    float mode1_frequency_;
    float mode1_damping_;
    float mode1_weight_;
    float mode2_frequency_;
    float mode2_damping_;
    float mode2_weight_;
    float mode3_frequency_;
    float mode3_damping_;
    float mode3_weight_;

    // Parameter cache - Excitation
    float pokeStrength_;
    float pokeDuration_;

    // Parameter cache - Voice
    float polyphony_;
    float personality_;

    // Parameter cache - ADSR Envelope
    float attack_;
    float decay_;
    float sustain_;
    float release_;

    /**
     * @brief Process single event (real-time safe)
     */
    void processEvent(const SynthEvent& event);

    /**
     * @brief Update control-rate parameters
     */
    void updateControlRate();

    /**
     * @brief Render audio slice (real-time safe)
     */
    void renderSlice(float* outL, float* outR, uint32_t startFrame, uint32_t numFrames);
};

#endif // SYNTH_ENGINE_H
