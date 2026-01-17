/**
 * @file main_test.cpp
 * @brief Test Runner - Automated scenario testing
 *
 * This is an alternative main file that runs automated test scenarios
 * instead of responding to external MIDI input.
 *
 * Build with: pio run -e daisy_seed_test
 *
 * Operation:
 * - Cycles through all test scenarios sequentially
 * - Each scenario runs for TEST_DURATION_MS (10 seconds)
 * - Logs audio statistics to serial output
 * - LED blinks to indicate scenario changes
 *
 * To switch between normal and test mode:
 * - Normal: pio run -e daisy_seed
 * - Test:   pio run -e daisy_seed_test
 */

#include "daisy_seed.h"
#include "test_scenarios.h"

extern "C" {
#include "core/modal_node.h"
#include "audio/audio_synth.h"
#include "midi/usb_midi_handler.h"
}

using namespace daisy;

// ============================================================================
// Global State
// ============================================================================

DaisySeed hw;
modal_node_t node;
audio_synth_t synth;
midi_handler_t midi_handler;
test_runner_t test_runner;

// Test sequencing
test_scenario_t current_scenario = TEST_SCENARIO_SINGLE_POKE;
bool test_running = false;
uint32_t scenario_start_time = 0;
const uint32_t INTER_TEST_DELAY_MS = 2000;  // 2 second pause between tests

// ============================================================================
// Audio Callback
// ============================================================================

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size) {
    // Generate audio
    audio_synth_process(&synth, out[0], out[1], size);

    // Process for test analysis
    test_runner_process_audio(&test_runner, out[0], out[1], size);
}

// ============================================================================
// Test Sequencer
// ============================================================================

void StartNextTest() {
    if (current_scenario >= TEST_SCENARIO_COUNT) {
        // All tests complete, restart from beginning
        hw.PrintLine("\n\n========================================");
        hw.PrintLine("ALL TESTS COMPLETE - Restarting cycle");
        hw.PrintLine("========================================\n\n");

        System::Delay(3000);
        current_scenario = TEST_SCENARIO_SINGLE_POKE;
    }

    // Reset modal node to clean state
    modal_node_reset(&node);
    modal_node_start(&node);

    // Start test scenario
    test_runner_start(&test_runner, current_scenario);
    test_running = true;

    // Blink LED 3 times to indicate test start
    for (int i = 0; i < 3; i++) {
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(100);
    }
}

void ProcessTestSequence() {
    if (test_running) {
        // Update current test
        bool still_running = test_runner_update(&test_runner);

        if (!still_running) {
            // Test completed
            test_running = false;
            scenario_start_time = System::GetNow();

            // Move to next scenario
            current_scenario = (test_scenario_t)((int)current_scenario + 1);

            // LED on during inter-test delay
            hw.SetLed(true);
        }
    } else {
        // Inter-test delay
        if (System::GetNow() - scenario_start_time >= INTER_TEST_DELAY_MS) {
            hw.SetLed(false);
            StartNextTest();
        }
    }
}

// ============================================================================
// Initialization
// ============================================================================

void setup() {
    // Initialize Daisy Seed hardware
    hw.Init();
    hw.SetAudioBlockSize(48);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    // Initialize serial for logging
    hw.StartLog(true);
    System::Delay(100);

    hw.PrintLine("\n\n========================================");
    hw.PrintLine("Modal Resonator - Automated Test Suite");
    hw.PrintLine("========================================");
    hw.PrintLine("Scenarios: %d", TEST_SCENARIO_COUNT);
    hw.PrintLine("Duration per test: %d ms", TEST_DURATION_MS);
    hw.PrintLine("========================================\n");

    // Initialize modal node (default configuration)
    modal_node_init(&node, 0, PERSONALITY_RESONATOR);

    // Mode 0: 110 Hz fundamental
    modal_node_set_mode(&node, 0, freq_to_omega(110.0f), 0.5f, 1.0f);

    // Mode 1: 220.5 Hz octave + detune
    modal_node_set_mode(&node, 1, freq_to_omega(220.5f), 0.6f, 0.8f);

    // Mode 2: 330 Hz brightness
    modal_node_set_mode(&node, 2, freq_to_omega(330.0f), 1.0f, 0.5f);

    // Mode 3: 55 Hz sub-bass
    modal_node_set_mode(&node, 3, freq_to_omega(55.0f), 0.3f, 0.6f);

    modal_node_start(&node);

    // Initialize audio synthesis
    audio_synth_init(&synth, &node);

    // Initialize MIDI handler
    midi_handler_init(&midi_handler, &node);

    // Initialize test runner
    test_runner_init(&test_runner, &node, &synth, &midi_handler);

    // Seed random number generator
    srand(System::GetNow());

    // Start audio
    hw.StartAudio(AudioCallback);

    // LED blink to indicate ready
    for (int i = 0; i < 5; i++) {
        hw.SetLed(true);
        System::Delay(100);
        hw.SetLed(false);
        System::Delay(100);
    }

    System::Delay(1000);

    // Start first test
    StartNextTest();
}

// ============================================================================
// Main Loop
// ============================================================================

void loop() {
    // Run test sequence
    ProcessTestSequence();

    // Continue modal integration at 500Hz
    static uint32_t last_control_ms = 0;
    uint32_t now = System::GetNow();

    if (now - last_control_ms >= 2) {  // 2ms = 500Hz
        modal_node_step(&node);
        last_control_ms = now;
    }

    System::Delay(1);
}

// ============================================================================
// Entry Point
// ============================================================================

int main(void) {
    setup();

    while (1) {
        loop();
    }
}
