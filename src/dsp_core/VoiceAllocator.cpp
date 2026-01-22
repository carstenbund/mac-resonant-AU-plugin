/**
 * @file VoiceAllocator.cpp
 * @brief Voice allocation and management implementation
 */

#include "VoiceAllocator.h"
#include <cstring>
#include <cmath>
#include <algorithm>

VoiceAllocator::VoiceAllocator(uint32_t max_polyphony)
    : max_polyphony_(max_polyphony)
    , pitch_bend_(0.0f)
    , sustain_pedal_(false)
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
}

VoiceAllocator::~VoiceAllocator() {
    // Delete all voices
    if (voices_) {
        for (uint32_t i = 0; i < max_polyphony_; i++) {
            delete voices_[i];
        }
        delete[] voices_;
    }
}

void VoiceAllocator::initialize(float sample_rate) {
    sample_rate_ = sample_rate;

    // Initialize all voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        voices_[i]->initialize(sample_rate);
    }

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
        return voice;
    }

    // Find free voice
    ModalVoice* voice = findFreeVoice();
    if (!voice) {
        // No free voices, steal best candidate (release-first + quietest)
        voice = stealBestCandidate();
    }

    if (voice) {
        // Allocate voice
        voice->noteOn(midi_note, velocity);
        voice->setPitchBend(pitch_bend_);

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

void VoiceAllocator::setSustain(bool sustain) {
    sustain_pedal_ = sustain;

    // Apply to all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            voices_[i]->setSustain(sustain);
        }
    }
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

    // Clear output buffers
    memset(outL, 0, num_frames * sizeof(float));
    memset(outR, 0, num_frames * sizeof(float));

    // Temporary buffers for each voice
    float* tempL = new float[num_frames];
    float* tempR = new float[num_frames];

    // Mix all active voices
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (voices_[i]->isActive()) {
            // Render voice
            voices_[i]->renderAudio(tempL, tempR, num_frames);

            // Mix into output
            for (uint32_t j = 0; j < num_frames; j++) {
                outL[j] += tempL[j];
                outR[j] += tempR[j];
            }
        }
    }

    // Clean up
    delete[] tempL;
    delete[] tempR;
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
    // Find first inactive voice
    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (!voices_[i]->isActive()) {
            return voices_[i];
        }
    }
    return nullptr;
}

ModalVoice* VoiceAllocator::stealBestCandidate() {
    ModalVoice* candidate = nullptr;
    float min_level = 1e10f;  // Very large initial value
    uint32_t max_age = 0;
    int best_priority = 999;  // Lower is better

    for (uint32_t i = 0; i < max_polyphony_; i++) {
        if (!voices_[i]->isActive()) continue;

        ModalVoice::State state = voices_[i]->getState();
        bool is_sustained = voices_[i]->isSustained();
        bool is_key_held = (state == ModalVoice::State::Attack ||
                           state == ModalVoice::State::Sustain) && !is_sustained;
        float level = voices_[i]->getAmplitude();
        uint32_t age = voices_[i]->getAge();

        // Determine priority category (lower number = higher priority to steal)
        int priority;
        if (state == ModalVoice::State::Release || state == ModalVoice::State::FadeOut) {
            priority = 0;  // Highest priority: already releasing
        } else if (is_sustained && !is_key_held) {
            priority = 1;  // Medium priority: sustained by pedal only
        } else {
            priority = 2;  // Lowest priority: actively held key
        }

        // Select candidate based on: priority (lower better) → level (lower better) → age (higher better)
        bool is_better = false;
        if (priority < best_priority) {
            is_better = true;
        } else if (priority == best_priority) {
            if (level < min_level - 0.0001f) {  // Use small epsilon for float comparison
                is_better = true;
            } else if (fabsf(level - min_level) < 0.0001f && age > max_age) {
                is_better = true;  // Tie-breaker: older voice
            }
        }

        if (is_better) {
            best_priority = priority;
            min_level = level;
            max_age = age;
            candidate = voices_[i];
        }
    }

    // Trigger short fade-out instead of hard reset (THIS FIXES THE CLICKS!)
    if (candidate) {
        // Force immediate 2ms fadeout - this will crossfade with the new voice's 2ms fade-in
        candidate->forceSteal();
    }

    return candidate;
}
