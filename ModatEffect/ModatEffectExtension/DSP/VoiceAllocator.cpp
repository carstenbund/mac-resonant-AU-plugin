/**
 * @file VoiceAllocator.cpp
 * @brief Voice allocation and management implementation
 */

#include "VoiceAllocator.h"
#include <cstring>
#include <algorithm>

VoiceAllocator::VoiceAllocator(uint32_t max_polyphony)
    : max_polyphony_(max_polyphony)
    , active_node_count_(max_polyphony)  // Default to full polyphony
    , pitch_bend_(0.0f)
    , personality_(PERSONALITY_RESONATOR)
    , poke_strength_(0.5f)
    , poke_duration_ms_(10.0f)
    , temp_buffer_L_(nullptr)
    , temp_buffer_R_(nullptr)
    , max_buffer_size_(0)
    , sample_rate_(48000.0f)
    , initialized_(false)
{
    // Allocate voice pool
    voices_ = new ModalVoice*[max_polyphony];
    for (uint32_t i = 0; i < max_polyphony; i++) {
        voices_[i] = new ModalVoice(static_cast<uint8_t>(i));
    }

    // Initialize note mapping to -1 (no voice assigned)
    memset(note_to_voice_, -1, sizeof(note_to_voice_));

    // Initialize mode parameters with defaults (from Parameters.swift)
    mode_params_[0] = {1.0f, 1.0f, 1.0f};   // Mode 0
    mode_params_[1] = {2.0f, 1.2f, 0.8f};   // Mode 1
    mode_params_[2] = {3.0f, 1.5f, 0.6f};   // Mode 2
    mode_params_[3] = {4.5f, 2.0f, 0.4f};   // Mode 3
}

VoiceAllocator::~VoiceAllocator() {
    // Delete all voices
    if (voices_) {
        for (uint32_t i = 0; i < max_polyphony_; i++) {
            delete voices_[i];
        }
        delete[] voices_;
    }

    // Free temp buffers
    if (temp_buffer_L_) {
        delete[] temp_buffer_L_;
        temp_buffer_L_ = nullptr;
    }
    if (temp_buffer_R_) {
        delete[] temp_buffer_R_;
        temp_buffer_R_ = nullptr;
    }
}

void VoiceAllocator::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize all voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        voices_[i]->initialize(sample_rate);
    }

    // Allocate temp buffers for rendering (real-time safe)
    // Use 2048 samples as maximum - covers most typical audio buffer sizes
    max_buffer_size_ = 2048;
    temp_buffer_L_ = new float[max_buffer_size_];
    temp_buffer_R_ = new float[max_buffer_size_];

    initialized_ = true;
}

ModalVoice* VoiceAllocator::noteOn(uint8_t midi_note, float velocity) {
    if (!initialized_ || midi_note > 127) return nullptr;

    // Check if this note is already playing
    int8_t existing_voice = note_to_voice_[midi_note];
    if (existing_voice >= 0) {
        // Re-trigger existing voice
        ModalVoice* voice = voices_[existing_voice];
        voice->noteOn(midi_note, velocity);
        voice->setPitchBend(pitch_bend_);
        voice->setPersonality(personality_);

        // Apply mode parameters after noteOn (which calls updateFrequencies with hardcoded values)
        float base_freq = voice->getBaseFrequency();
        for (uint8_t mode_idx = 0; mode_idx < 4; mode_idx++) {
            float mode_freq = base_freq * mode_params_[mode_idx].freq_multiplier;
            voice->setMode(mode_idx, mode_freq, mode_params_[mode_idx].damping, mode_params_[mode_idx].weight);
        }

        return voice;
    }

    // Find free voice
    ModalVoice* voice = findFreeVoice();
    if (!voice) {
        // No free voices, steal oldest
        voice = stealOldestVoice();
    }

    if (voice) {
        // Allocate voice
        voice->noteOn(midi_note, velocity);
        voice->setPitchBend(pitch_bend_);
        voice->setPersonality(personality_);

        // Apply mode parameters after noteOn (which calls updateFrequencies with hardcoded values)
        float base_freq = voice->getBaseFrequency();
        for (uint8_t mode_idx = 0; mode_idx < 4; mode_idx++) {
            float mode_freq = base_freq * mode_params_[mode_idx].freq_multiplier;
            voice->setMode(mode_idx, mode_freq, mode_params_[mode_idx].damping, mode_params_[mode_idx].weight);
        }

        // Update mapping
        for (uint32_t i = 0; i < max_polyphony_; i++) {
            if (voices_[i] == voice) {
                note_to_voice_[midi_note] = static_cast<int8_t>(i);
                break;
            }
        }
    }

    return voice;
}

void VoiceAllocator::noteOff(uint8_t midi_note) {
    if (midi_note > 127) return;

    int8_t voice_idx = note_to_voice_[midi_note];
    if (voice_idx >= 0 && voice_idx < static_cast<int8_t>(max_polyphony_)) {
        voices_[voice_idx]->noteOff();
        note_to_voice_[midi_note] = -1;
    }
}

void VoiceAllocator::allNotesOff() {
    // Release all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->noteOff();
        }
    }

    // Clear note mapping
    memset(note_to_voice_, -1, sizeof(note_to_voice_));
}

void VoiceAllocator::setPitchBend(float bend_amount) {
    pitch_bend_ = bend_amount;

    // Apply to all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->setPitchBend(bend_amount);
        }
    }
}

void VoiceAllocator::setPersonality(node_personality_t personality) {
    personality_ = personality;

    // Apply to all voices (both active and inactive)
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        voices_[i]->setPersonality(personality);
    }
}

void VoiceAllocator::setMode(uint8_t mode_idx, float freq_multiplier, float damping, float weight) {
    if (mode_idx >= 4) return;  // Only 4 modes (0-3)

    // Store parameters
    mode_params_[mode_idx].freq_multiplier = freq_multiplier;
    mode_params_[mode_idx].damping = damping;
    mode_params_[mode_idx].weight = weight;

    // Apply to all voices (both active and inactive)
    // Note: Mode parameters use frequency multipliers, but ModalVoice::setMode expects Hz
    // We'll apply the multiplier in noteOn() when we know the base frequency
    // For now, we need to update active voices that already have a base frequency
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            // Get the base frequency from the voice's current MIDI note
            float base_freq = voices_[i]->getBaseFrequency();
            if (base_freq > 0.0f) {
                float mode_freq = base_freq * freq_multiplier;
                voices_[i]->setMode(mode_idx, mode_freq, damping, weight);
            }
        }
    }
}

void VoiceAllocator::setPokeStrength(float strength) {
    poke_strength_ = strength;
    // Poke strength is applied at note-on time
}

void VoiceAllocator::setPokeDuration(float duration_ms) {
    poke_duration_ms_ = duration_ms;
    // Poke duration is applied at note-on time
}

void VoiceAllocator::setNodeCount(uint32_t node_count) {
    // Clamp to valid range
    if (node_count < 1) node_count = 1;
    if (node_count > max_polyphony_) node_count = max_polyphony_;

    // If reducing node count, release voices above the new limit
    if (node_count < active_node_count_) {
        for (uint32_t i = node_count; i < max_polyphony_; i++) {
            if (voices_[i]->isActive()) {
                voices_[i]->reset();

                // Clear note mapping for this voice
                for (int note = 0; note < 128; note++) {
                    if (note_to_voice_[note] == static_cast<int8_t>(i)) {
                        note_to_voice_[note] = -1;
                    }
                }
            }
        }
    }

    active_node_count_ = node_count;
}

void VoiceAllocator::updateVoices() {
    if (!initialized_) return;

    // Update all active voices at control rate
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->updateModal();
        }
    }
}

void VoiceAllocator::renderAudio(float* outL, float* outR, uint32_t num_frames) {
    if (!initialized_) {
        // Return silence
        memset(outL, 0, num_frames * sizeof(float));
        memset(outR, 0, num_frames * sizeof(float));
        return;
    }

    // Safety check: ensure we don't exceed pre-allocated buffer size
    if (num_frames > max_buffer_size_) {
        // This should never happen in practice, but log and clamp if it does
        num_frames = max_buffer_size_;
    }

    // Clear output buffers
    memset(outL, 0, num_frames * sizeof(float));
    memset(outR, 0, num_frames * sizeof(float));

    // Mix all active voices using pre-allocated temp buffers (real-time safe)
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            // Render voice into temp buffers
            voices_[i]->renderAudio(temp_buffer_L_, temp_buffer_R_, num_frames);

            // Mix into output
            for (uint32_t j = 0; j < num_frames; j++) {
                outL[j] += temp_buffer_L_[j];
                outR[j] += temp_buffer_R_[j];
            }
        }
    }
}

ModalVoice* VoiceAllocator::getVoice(uint32_t voice_idx) {
    if (voice_idx >= max_polyphony_) return nullptr;
    return voices_[voice_idx];
}

uint32_t VoiceAllocator::getActiveVoiceCount() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            count++;
        }
    }
    return count;
}

ModalVoice* VoiceAllocator::findFreeVoice() {
    // Find first inactive voice within active node count limit
    for (uint32_t i = 0; i < active_node_count_; i++) {
        if (!voices_[i]->isActive()) {
            return voices_[i];
        }
    }
    return nullptr;
}

ModalVoice* VoiceAllocator::stealOldestVoice() {
    // Find oldest active voice within active node count limit
    ModalVoice* oldest = nullptr;
    uint32_t max_age = 0;

    for (uint32_t i = 0; i < active_node_count_; i++) {
        if (voices_[i]->isActive()) {
            uint32_t age = voices_[i]->getAge();
            if (age > max_age) {
                max_age = age;
                oldest = voices_[i];
            }
        }
    }

    // Force release the oldest voice
    if (oldest) {
        oldest->reset();
    }

    return oldest;
}
