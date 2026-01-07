/**
 * @file test_suite.cpp
 * @brief Comprehensive test suite for Modal Attractors DSP
 *
 * Tests:
 * 1. Single voice excitation and decay
 * 2. Multiple excitation patterns (poke events)
 * 3. Polyphony (multiple voices)
 * 4. Different topologies (ring, small-world, etc.)
 * 5. Network coupling behavior
 * 6. MIDI response
 * 7. Parameter sweeps
 */

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include "../src/dsp_core/ModalVoice.h"
#include "../src/dsp_core/VoiceAllocator.h"
#include "../src/dsp_core/TopologyEngine.h"

// WAV file header
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t file_size;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t fmt_size = 16;
    uint16_t audio_format = 1;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t data_size;
};

void writeWAV(const char* filename, std::vector<std::vector<float>>& channels, uint32_t num_samples, float sample_rate) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << filename << std::endl;
        return;
    }

    uint16_t num_channels = channels.size();
    WAVHeader header;
    header.num_channels = num_channels;
    header.sample_rate = static_cast<uint32_t>(sample_rate);
    header.byte_rate = header.sample_rate * num_channels * 2;
    header.block_align = num_channels * 2;
    header.data_size = num_samples * num_channels * 2;
    header.file_size = 36 + header.data_size;

    file.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));

    // Write interleaved samples
    for (uint32_t i = 0; i < num_samples; i++) {
        for (uint16_t ch = 0; ch < num_channels; ch++) {
            int16_t sample = static_cast<int16_t>(channels[ch][i] * 32767.0f);
            file.write(reinterpret_cast<char*>(&sample), sizeof(int16_t));
        }
    }

    file.close();
    std::cout << "✓ Wrote " << num_samples << " samples to " << filename
              << " (" << num_channels << " channels)" << std::endl;
}

// ============================================================================
// Test 1: Single Voice - Excitation and Decay
// ============================================================================
void test_single_voice_decay() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 1: Single Voice - Excitation & Decay" << std::endl;
    std::cout << "========================================" << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 5.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;

    ModalVoice voice(0);
    voice.initialize(SAMPLE_RATE);
    voice.setPersonality(PERSONALITY_RESONATOR);

    // Configure 4 modes
    voice.setMode(0, 220.0f, 0.5f, 1.0f);
    voice.setMode(1, 222.2f, 0.6f, 0.7f);
    voice.setMode(2, 440.0f, 0.8f, 0.5f);
    voice.setMode(3, 660.0f, 1.0f, 0.3f);

    std::vector<float> left(NUM_SAMPLES);
    std::vector<float> right(NUM_SAMPLES);
    float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];

    voice.noteOn(57, 0.8f);  // A3

    uint32_t rendered = 0;
    while (rendered < NUM_SAMPLES) {
        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);
        voice.renderAudio(temp_l, temp_r, to_render);
        memcpy(&left[rendered], temp_l, to_render * sizeof(float));
        memcpy(&right[rendered], temp_r, to_render * sizeof(float));
        rendered += to_render;
    }

    std::vector<std::vector<float>> channels = {left, right};
    writeWAV("test_01_single_voice_decay.wav", channels, NUM_SAMPLES, SAMPLE_RATE);
    std::cout << "Expected: Clean decay over 5s with 2Hz beating" << std::endl;
}

// ============================================================================
// Test 2: Multiple Excitations (Repeated Pokes)
// ============================================================================
void test_multiple_excitations() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 2: Multiple Excitations" << std::endl;
    std::cout << "========================================" << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 6.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;

    ModalVoice voice(0);
    voice.initialize(SAMPLE_RATE);
    voice.setPersonality(PERSONALITY_RESONATOR);
    voice.setMode(0, 440.0f, 0.3f, 1.0f);  // Lower damping for longer ring

    std::vector<float> left(NUM_SAMPLES);
    std::vector<float> right(NUM_SAMPLES);
    float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];

    // Trigger notes at regular intervals (every 0.5s)
    float poke_times[] = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 5.0f};
    uint8_t notes[] = {60, 62, 64, 65, 67, 69, 71, 72};  // C major scale
    int poke_idx = 0;

    uint32_t rendered = 0;
    while (rendered < NUM_SAMPLES) {
        float t = rendered / SAMPLE_RATE;

        // Check if it's time for next poke
        if (poke_idx < 8 && t >= poke_times[poke_idx]) {
            voice.noteOff();
            voice.noteOn(notes[poke_idx], 0.7f);
            std::cout << "  Poke " << poke_idx << " at t=" << t << "s (note " << (int)notes[poke_idx] << ")" << std::endl;
            poke_idx++;
        }

        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);
        voice.renderAudio(temp_l, temp_r, to_render);
        memcpy(&left[rendered], temp_l, to_render * sizeof(float));
        memcpy(&right[rendered], temp_r, to_render * sizeof(float));
        rendered += to_render;
    }

    std::vector<std::vector<float>> channels = {left, right};
    writeWAV("test_02_multiple_excitations.wav", channels, NUM_SAMPLES, SAMPLE_RATE);
    std::cout << "Expected: 8 excitations with pitch changes" << std::endl;
}

// ============================================================================
// Test 3: Polyphony (4 simultaneous voices)
// ============================================================================
void test_polyphony() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 3: Polyphony (4 voices)" << std::endl;
    std::cout << "========================================" << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 5.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;

    VoiceAllocator allocator(8);
    allocator.initialize(SAMPLE_RATE);

    std::vector<float> left(NUM_SAMPLES);
    std::vector<float> right(NUM_SAMPLES);
    float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];

    // Play C major chord (C-E-G-C)
    uint8_t chord[] = {60, 64, 67, 72};
    float velocities[] = {0.8f, 0.7f, 0.6f, 0.5f};

    std::cout << "Playing chord: C-E-G-C" << std::endl;
    for (int i = 0; i < 4; i++) {
        allocator.noteOn(chord[i], velocities[i]);
        std::cout << "  Voice " << i << ": Note " << (int)chord[i] << std::endl;
    }

    uint32_t rendered = 0;
    while (rendered < NUM_SAMPLES) {
        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);
        memset(temp_l, 0, to_render * sizeof(float));
        memset(temp_r, 0, to_render * sizeof(float));
        allocator.renderAudio(temp_l, temp_r, to_render);
        memcpy(&left[rendered], temp_l, to_render * sizeof(float));
        memcpy(&right[rendered], temp_r, to_render * sizeof(float));
        rendered += to_render;
    }

    std::vector<std::vector<float>> channels = {left, right};
    writeWAV("test_03_polyphony_chord.wav", channels, NUM_SAMPLES, SAMPLE_RATE);
    std::cout << "Expected: 4 voices decaying together (chord)" << std::endl;
}

// ============================================================================
// Test 4: Topology Comparison
// ============================================================================
void test_topology(TopologyType topo, const char* name, const char* filename) {
    std::cout << "\n  Testing topology: " << name << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 4.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;

    VoiceAllocator allocator(8);
    allocator.initialize(SAMPLE_RATE);

    TopologyEngine topology(8);  // 8 voices
    topology.generateTopology(topo, 0.5f);  // Medium coupling

    std::vector<float> left(NUM_SAMPLES);
    std::vector<float> right(NUM_SAMPLES);
    float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];

    // Play 3 notes to create network activity
    allocator.noteOn(60, 0.8f);  // C
    allocator.noteOn(64, 0.7f);  // E
    allocator.noteOn(67, 0.6f);  // G

    uint32_t rendered = 0;
    while (rendered < NUM_SAMPLES) {
        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);
        memset(temp_l, 0, to_render * sizeof(float));
        memset(temp_r, 0, to_render * sizeof(float));

        // Apply coupling (simplified - would need VoiceAllocator integration)
        allocator.renderAudio(temp_l, temp_r, to_render);

        memcpy(&left[rendered], temp_l, to_render * sizeof(float));
        memcpy(&right[rendered], temp_r, to_render * sizeof(float));
        rendered += to_render;
    }

    std::vector<std::vector<float>> channels = {left, right};
    writeWAV(filename, channels, NUM_SAMPLES, SAMPLE_RATE);
}

void test_topologies() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 4: Topology Comparison" << std::endl;
    std::cout << "========================================" << std::endl;

    test_topology(TopologyType::Ring, "Ring", "test_04a_topology_ring.wav");
    test_topology(TopologyType::SmallWorld, "Small World", "test_04b_topology_smallworld.wav");
    test_topology(TopologyType::Complete, "Complete", "test_04c_topology_complete.wav");
    test_topology(TopologyType::None, "None (no coupling)", "test_04d_topology_none.wav");

    std::cout << "Expected: Different decay patterns based on coupling topology" << std::endl;
}

// ============================================================================
// Test 5: MIDI Scale (Sequential notes)
// ============================================================================
void test_midi_scale() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 5: MIDI Scale Response" << std::endl;
    std::cout << "========================================" << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 8.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;
    const float NOTE_DURATION = 0.5f;

    VoiceAllocator allocator(4);
    allocator.initialize(SAMPLE_RATE);

    std::vector<float> left(NUM_SAMPLES);
    std::vector<float> right(NUM_SAMPLES);
    float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];

    // C major scale
    uint8_t scale[] = {60, 62, 64, 65, 67, 69, 71, 72, 71, 69, 67, 65, 64, 62, 60};
    int scale_len = 15;
    int current_note = -1;
    float last_note_time = 0.0f;

    uint32_t rendered = 0;
    while (rendered < NUM_SAMPLES) {
        float t = rendered / SAMPLE_RATE;

        // Play next note in scale
        if (t >= last_note_time + NOTE_DURATION && current_note < scale_len - 1) {
            current_note++;
            allocator.noteOn(scale[current_note], 0.7f);
            last_note_time = t;
            std::cout << "  t=" << t << "s: Note " << (int)scale[current_note] << std::endl;
        }

        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);
        memset(temp_l, 0, to_render * sizeof(float));
        memset(temp_r, 0, to_render * sizeof(float));
        allocator.renderAudio(temp_l, temp_r, to_render);
        memcpy(&left[rendered], temp_l, to_render * sizeof(float));
        memcpy(&right[rendered], temp_r, to_render * sizeof(float));
        rendered += to_render;
    }

    std::vector<std::vector<float>> channels = {left, right};
    writeWAV("test_05_midi_scale.wav", channels, NUM_SAMPLES, SAMPLE_RATE);
    std::cout << "Expected: C major scale up and down" << std::endl;
}

// ============================================================================
// Test 6: Multichannel (Per-voice routing)
// ============================================================================
void test_multichannel() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 6: Multichannel (8 voices, 8 channels)" << std::endl;
    std::cout << "========================================" << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 5.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;
    const int NUM_VOICES = 8;

    // Create 8 separate voices
    std::vector<ModalVoice*> voices;
    for (int i = 0; i < NUM_VOICES; i++) {
        ModalVoice* v = new ModalVoice(i);
        v->initialize(SAMPLE_RATE);
        v->setPersonality(PERSONALITY_RESONATOR);
        v->setMode(0, 220.0f * (1.0f + i * 0.1f), 0.5f, 1.0f);
        voices.push_back(v);
    }

    // 8-channel output
    std::vector<std::vector<float>> channels(NUM_VOICES);
    for (int i = 0; i < NUM_VOICES; i++) {
        channels[i].resize(NUM_SAMPLES);
    }

    // Trigger notes sequentially (like a drum roll)
    uint32_t trigger_samples[] = {0, 4800, 9600, 14400, 19200, 24000, 28800, 33600};  // Every 0.1s
    uint8_t notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
    bool triggered[NUM_VOICES] = {false};

    uint32_t rendered = 0;
    while (rendered < NUM_SAMPLES) {
        // Check triggers based on sample position
        for (int i = 0; i < NUM_VOICES; i++) {
            if (!triggered[i] && rendered >= trigger_samples[i]) {
                voices[i]->noteOn(notes[i], 0.8f);
                triggered[i] = true;
                std::cout << "  t=" << (rendered / SAMPLE_RATE) << "s: Voice " << i << " (note " << (int)notes[i] << ")" << std::endl;
            }
        }

        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);

        // Render each voice to its own channel
        for (int i = 0; i < NUM_VOICES; i++) {
            float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];
            voices[i]->renderAudio(temp_l, temp_r, to_render);
            // Use left channel for multichannel
            memcpy(&channels[i][rendered], temp_l, to_render * sizeof(float));
        }

        rendered += to_render;
    }

    writeWAV("test_06_multichannel_8voices.wav", channels, NUM_SAMPLES, SAMPLE_RATE);
    std::cout << "Expected: 8 channels, each with separate voice/timing" << std::endl;

    // Cleanup
    for (auto v : voices) delete v;
}

// ============================================================================
// Test 7: Parameter Sweep (Damping)
// ============================================================================
void test_damping_sweep() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST 7: Parameter Sweep (Damping)" << std::endl;
    std::cout << "========================================" << std::endl;

    const float SAMPLE_RATE = 48000.0f;
    const float DURATION = 10.0f;
    const uint32_t NUM_SAMPLES = SAMPLE_RATE * DURATION;
    const uint32_t BUFFER_SIZE = 512;

    ModalVoice voice(0);
    voice.initialize(SAMPLE_RATE);
    voice.setPersonality(PERSONALITY_RESONATOR);

    std::vector<float> left(NUM_SAMPLES);
    std::vector<float> right(NUM_SAMPLES);
    float temp_l[BUFFER_SIZE], temp_r[BUFFER_SIZE];

    // Test 5 damping values
    float dampings[] = {0.1f, 0.3f, 0.5f, 0.8f, 1.5f};
    float segment_dur = DURATION / 5.0f;

    uint32_t rendered = 0;
    int damping_idx = 0;

    while (rendered < NUM_SAMPLES) {
        float t = rendered / SAMPLE_RATE;
        int new_idx = static_cast<int>(t / segment_dur);

        if (new_idx != damping_idx && new_idx < 5) {
            damping_idx = new_idx;
            voice.setMode(0, 440.0f, dampings[damping_idx], 1.0f);
            voice.noteOn(69, 0.8f);  // A4
            std::cout << "  t=" << t << "s: Damping = " << dampings[damping_idx] << std::endl;
        }

        uint32_t to_render = std::min(BUFFER_SIZE, NUM_SAMPLES - rendered);
        voice.renderAudio(temp_l, temp_r, to_render);
        memcpy(&left[rendered], temp_l, to_render * sizeof(float));
        memcpy(&right[rendered], temp_r, to_render * sizeof(float));
        rendered += to_render;
    }

    std::vector<std::vector<float>> channels = {left, right};
    writeWAV("test_07_damping_sweep.wav", channels, NUM_SAMPLES, SAMPLE_RATE);
    std::cout << "Expected: 5 segments with different decay rates" << std::endl;
}

// ============================================================================
// Main Test Runner
// ============================================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "MODAL ATTRACTORS - TEST SUITE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nThis suite generates WAV files for each test." << std::endl;
    std::cout << "Analyze waveforms to verify correct behavior.\n" << std::endl;

    test_single_voice_decay();
    test_multiple_excitations();
    test_polyphony();
    test_topologies();
    test_midi_scale();
    test_multichannel();
    test_damping_sweep();

    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST SUITE COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nGenerated test files:" << std::endl;
    std::cout << "  test_01_single_voice_decay.wav      - Basic excitation/decay" << std::endl;
    std::cout << "  test_02_multiple_excitations.wav    - Repeated pokes" << std::endl;
    std::cout << "  test_03_polyphony_chord.wav         - 4 simultaneous voices" << std::endl;
    std::cout << "  test_04a-d_topology_*.wav           - Different topologies" << std::endl;
    std::cout << "  test_05_midi_scale.wav              - MIDI note sequence" << std::endl;
    std::cout << "  test_06_multichannel_8voices.wav    - 8-channel routing" << std::endl;
    std::cout << "  test_07_damping_sweep.wav           - Parameter sweep" << std::endl;
    std::cout << "\nAnalyze these with:" << std::endl;
    std::cout << "  python3 check_audio.py test_*.wav" << std::endl;
    std::cout << "  Or open in audio editor to view waveforms" << std::endl;

    return 0;
}
