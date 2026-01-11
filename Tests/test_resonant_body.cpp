//
//  test_resonant_body.cpp
//  ModalAttractors
//
//  Standalone test for ResonantBodyProcessor DSP
//  Generates test audio and outputs to WAV file
//
//  Created by Claude on 2026-01-11.
//

#include "ResonantBodyProcessor.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple WAV file writer
class WavWriter {
public:
    WavWriter(const char* filename, float sample_rate, uint16_t num_channels)
        : sample_rate_(sample_rate)
        , num_channels_(num_channels)
        , file_(filename, std::ios::binary)
    {
        if (!file_) {
            std::cerr << "Failed to open file: " << filename << std::endl;
        }
    }

    ~WavWriter() {
        if (file_) {
            finalize();
        }
    }

    void write(const float* left, const float* right, uint32_t num_frames) {
        for (uint32_t i = 0; i < num_frames; i++) {
            // Clamp and convert to 16-bit PCM
            int16_t sample_L = static_cast<int16_t>(std::clamp(left[i], -1.0f, 1.0f) * 32767.0f);
            int16_t sample_R = static_cast<int16_t>(std::clamp(right[i], -1.0f, 1.0f) * 32767.0f);

            file_.write(reinterpret_cast<const char*>(&sample_L), sizeof(int16_t));
            file_.write(reinterpret_cast<const char*>(&sample_R), sizeof(int16_t));

            num_samples_ += num_channels_;
        }
    }

private:
    void finalize() {
        uint32_t data_size = num_samples_ * sizeof(int16_t);
        uint32_t file_size = 36 + data_size;

        // Seek to beginning and write header
        file_.seekp(0);

        // RIFF header
        file_.write("RIFF", 4);
        file_.write(reinterpret_cast<const char*>(&file_size), 4);
        file_.write("WAVE", 4);

        // fmt chunk
        file_.write("fmt ", 4);
        uint32_t fmt_size = 16;
        file_.write(reinterpret_cast<const char*>(&fmt_size), 4);

        uint16_t audio_format = 1;  // PCM
        file_.write(reinterpret_cast<const char*>(&audio_format), 2);
        file_.write(reinterpret_cast<const char*>(&num_channels_), 2);

        uint32_t sample_rate_int = static_cast<uint32_t>(sample_rate_);
        file_.write(reinterpret_cast<const char*>(&sample_rate_int), 4);

        uint32_t byte_rate = sample_rate_int * num_channels_ * sizeof(int16_t);
        file_.write(reinterpret_cast<const char*>(&byte_rate), 4);

        uint16_t block_align = num_channels_ * sizeof(int16_t);
        file_.write(reinterpret_cast<const char*>(&block_align), 2);

        uint16_t bits_per_sample = 16;
        file_.write(reinterpret_cast<const char*>(&bits_per_sample), 2);

        // data chunk
        file_.write("data", 4);
        file_.write(reinterpret_cast<const char*>(&data_size), 4);

        file_.close();
    }

    float sample_rate_;
    uint16_t num_channels_;
    uint32_t num_samples_ = 0;
    std::ofstream file_;
};

// Test signal generators
void generateImpulseTrain(float* buffer, uint32_t num_frames, float sample_rate,
                          float impulse_rate_hz, float amplitude) {
    const uint32_t impulse_period = static_cast<uint32_t>(sample_rate / impulse_rate_hz);

    for (uint32_t i = 0; i < num_frames; i++) {
        if (i % impulse_period == 0) {
            buffer[i] = amplitude;
        } else {
            buffer[i] = 0.0f;
        }
    }
}

void generateSineSweep(float* buffer, uint32_t num_frames, float sample_rate,
                       float start_freq, float end_freq, float amplitude) {
    const float duration = static_cast<float>(num_frames) / sample_rate;
    const float freq_rate = (end_freq - start_freq) / duration;

    float phase = 0.0f;
    for (uint32_t i = 0; i < num_frames; i++) {
        const float t = static_cast<float>(i) / sample_rate;
        const float freq = start_freq + freq_rate * t;

        buffer[i] = amplitude * std::sin(phase);

        phase += 2.0f * M_PI * freq / sample_rate;

        // Wrap phase
        while (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
    }
}

void generateWhiteNoise(float* buffer, uint32_t num_frames, float amplitude) {
    for (uint32_t i = 0; i < num_frames; i++) {
        buffer[i] = amplitude * (2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f);
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== ModalAttractor Resonant Body Effect Test ===" << std::endl;
    std::cout << std::endl;

    // Test parameters
    const float SAMPLE_RATE = 48000.0f;
    const float DURATION_SEC = 5.0f;
    const uint32_t NUM_FRAMES = static_cast<uint32_t>(SAMPLE_RATE * DURATION_SEC);

    std::cout << "Sample rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Duration: " << DURATION_SEC << " seconds" << std::endl;
    std::cout << "Total frames: " << NUM_FRAMES << std::endl;
    std::cout << std::endl;

    // Create processor
    ResonantBodyProcessor processor;
    processor.initialize(SAMPLE_RATE);

    // Configure effect parameters
    processor.setBaseFrequency(220.0f);  // A3
    processor.setBodySize(0.5f);         // Medium body
    processor.setMaterial(0.7f);         // Fairly resonant
    processor.setExcitation(0.8f);       // Strong excitation
    processor.setMorph(0.0f);            // No pitch morphing for this test
    processor.setMix(1.0f);              // 100% wet (resonator only)

    std::cout << "Processor configuration:" << std::endl;
    std::cout << "  Base frequency: 220 Hz (A3)" << std::endl;
    std::cout << "  Body size: 0.5 (medium)" << std::endl;
    std::cout << "  Material: 0.7 (resonant)" << std::endl;
    std::cout << "  Excitation: 0.8 (strong)" << std::endl;
    std::cout << "  Mix: 1.0 (100% wet)" << std::endl;
    std::cout << std::endl;

    // Test 1: Impulse train (percussive)
    std::cout << "Test 1: Impulse train (4 Hz, percussive)" << std::endl;
    {
        std::vector<float> input(NUM_FRAMES);
        std::vector<float> outputL(NUM_FRAMES);
        std::vector<float> outputR(NUM_FRAMES);

        generateImpulseTrain(input.data(), NUM_FRAMES, SAMPLE_RATE, 4.0f, 0.5f);

        processor.reset();
        processor.process(input.data(), input.data(), outputL.data(), outputR.data(), NUM_FRAMES);

        WavWriter writer("test_resonant_body_impulse.wav", SAMPLE_RATE, 2);
        writer.write(outputL.data(), outputR.data(), NUM_FRAMES);

        std::cout << "  Output: test_resonant_body_impulse.wav" << std::endl;
        std::cout << "  Final resonator energy: " << processor.getResonatorEnergy() << std::endl;
    }
    std::cout << std::endl;

    // Test 2: Sine sweep (frequency response)
    std::cout << "Test 2: Sine sweep (50 Hz - 2000 Hz)" << std::endl;
    {
        std::vector<float> input(NUM_FRAMES);
        std::vector<float> outputL(NUM_FRAMES);
        std::vector<float> outputR(NUM_FRAMES);

        generateSineSweep(input.data(), NUM_FRAMES, SAMPLE_RATE, 50.0f, 2000.0f, 0.3f);

        processor.reset();
        processor.process(input.data(), input.data(), outputL.data(), outputR.data(), NUM_FRAMES);

        WavWriter writer("test_resonant_body_sweep.wav", SAMPLE_RATE, 2);
        writer.write(outputL.data(), outputR.data(), NUM_FRAMES);

        std::cout << "  Output: test_resonant_body_sweep.wav" << std::endl;
        std::cout << "  Final resonator energy: " << processor.getResonatorEnergy() << std::endl;
    }
    std::cout << std::endl;

    // Test 3: White noise (broadband excitation)
    std::cout << "Test 3: White noise (broadband excitation)" << std::endl;
    {
        std::vector<float> input(NUM_FRAMES);
        std::vector<float> outputL(NUM_FRAMES);
        std::vector<float> outputR(NUM_FRAMES);

        generateWhiteNoise(input.data(), NUM_FRAMES, 0.1f);

        processor.reset();
        processor.process(input.data(), input.data(), outputL.data(), outputR.data(), NUM_FRAMES);

        WavWriter writer("test_resonant_body_noise.wav", SAMPLE_RATE, 2);
        writer.write(outputL.data(), outputR.data(), NUM_FRAMES);

        std::cout << "  Output: test_resonant_body_noise.wav" << std::endl;
        std::cout << "  Final resonator energy: " << processor.getResonatorEnergy() << std::endl;
    }
    std::cout << std::endl;

    // Test 4: Pitch morphing test
    std::cout << "Test 4: Sine sweep with pitch morphing enabled" << std::endl;
    {
        std::vector<float> input(NUM_FRAMES);
        std::vector<float> outputL(NUM_FRAMES);
        std::vector<float> outputR(NUM_FRAMES);

        generateSineSweep(input.data(), NUM_FRAMES, SAMPLE_RATE, 100.0f, 500.0f, 0.3f);

        processor.reset();
        processor.setMorph(0.5f);  // Enable morphing
        processor.process(input.data(), input.data(), outputL.data(), outputR.data(), NUM_FRAMES);

        WavWriter writer("test_resonant_body_morph.wav", SAMPLE_RATE, 2);
        writer.write(outputL.data(), outputR.data(), NUM_FRAMES);

        std::cout << "  Output: test_resonant_body_morph.wav" << std::endl;
        std::cout << "  Morph enabled: 0.5" << std::endl;
        std::cout << "  Final resonator energy: " << processor.getResonatorEnergy() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "=== All tests completed successfully ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Generated files:" << std::endl;
    std::cout << "  1. test_resonant_body_impulse.wav - Impulse train (listen for resonant ringing)" << std::endl;
    std::cout << "  2. test_resonant_body_sweep.wav   - Frequency sweep (hear resonant peaks)" << std::endl;
    std::cout << "  3. test_resonant_body_noise.wav   - White noise (resonator as filter)" << std::endl;
    std::cout << "  4. test_resonant_body_morph.wav   - Morphing test (resonator tracks input pitch)" << std::endl;
    std::cout << std::endl;

    return 0;
}
