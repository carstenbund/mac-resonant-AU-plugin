/**
 * @file test_scenarios.cpp
 * @brief Automated test scenarios implementation
 */

#include "test_scenarios.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#ifdef DAISY_SEED
#include "daisy_seed.h"
using namespace daisy;
extern DaisySeed hw;
#define TEST_LOG(fmt, ...) hw.PrintLine(fmt, ##__VA_ARGS__)
#else
#include <stdio.h>
#define TEST_LOG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#endif

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Get current time in milliseconds
 */
static uint32_t get_time_ms() {
#ifdef DAISY_SEED
    return System::GetNow();
#else
    return 0;  // Mock for desktop testing
#endif
}

/**
 * @brief Simulate MIDI note on
 */
static void simulate_note_on(test_runner_t* runner, uint8_t note, uint8_t velocity) {
    if (runner->midi_handler) {
        midi_handler_note_on(runner->midi_handler, note, velocity);
        runner->event_count++;
    }
}

/**
 * @brief Simulate MIDI CC
 */
static void simulate_cc(test_runner_t* runner, uint8_t cc_num, uint8_t cc_val) {
    if (runner->midi_handler) {
        midi_handler_cc(runner->midi_handler, cc_num, cc_val);
    }
}

/**
 * @brief Simulate pitch bend
 */
static void simulate_pitch_bend(test_runner_t* runner, int16_t bend) {
    if (runner->midi_handler) {
        midi_handler_pitch_bend(runner->midi_handler, bend);
    }
}

// ============================================================================
// Initialization
// ============================================================================

void test_runner_init(test_runner_t* runner,
                     modal_node_t* node,
                     audio_synth_t* synth,
                     midi_handler_t* midi_handler) {
    memset(runner, 0, sizeof(test_runner_t));

    runner->node = node;
    runner->synth = synth;
    runner->midi_handler = midi_handler;
    runner->running = false;
}

void test_runner_start(test_runner_t* runner, test_scenario_t scenario) {
    runner->scenario = scenario;
    runner->start_time_ms = get_time_ms();
    runner->elapsed_ms = 0;
    runner->last_log_ms = 0;
    runner->event_count = 0;
    runner->peak_amplitude = 0.0f;
    runner->avg_amplitude = 0.0f;
    runner->sample_count = 0;
    runner->running = true;

    TEST_LOG("\n=== Starting Test: %s ===", test_scenario_name(scenario));
    TEST_LOG("Duration: %d ms", TEST_DURATION_MS);
}

void test_runner_stop(test_runner_t* runner) {
    runner->running = false;
    test_runner_log_results(runner);
}

// ============================================================================
// Main Update Loop
// ============================================================================

bool test_runner_update(test_runner_t* runner) {
    if (!runner->running) return false;

    // Update elapsed time
    runner->elapsed_ms = get_time_ms() - runner->start_time_ms;

    // Check if test completed
    if (runner->elapsed_ms >= TEST_DURATION_MS) {
        test_runner_stop(runner);
        return false;
    }

    // Periodic audio logging
    if (runner->elapsed_ms - runner->last_log_ms >= AUDIO_LOG_INTERVAL_MS) {
        float avg = (runner->sample_count > 0) ?
                   (runner->avg_amplitude / runner->sample_count) : 0.0f;

        TEST_LOG("[%5d ms] Peak: %.3f  Avg: %.3f  Events: %d",
                runner->elapsed_ms,
                runner->peak_amplitude,
                avg,
                runner->event_count);

        runner->last_log_ms = runner->elapsed_ms;
        runner->peak_amplitude = 0.0f;  // Reset for next interval
    }

    // Run scenario-specific logic
    switch (runner->scenario) {
        case TEST_SCENARIO_SINGLE_POKE:
            test_scenario_single_poke(runner);
            break;
        case TEST_SCENARIO_NOTE_SEQUENCE:
            test_scenario_note_sequence(runner);
            break;
        case TEST_SCENARIO_DAMPING_SWEEP:
            test_scenario_damping_sweep(runner);
            break;
        case TEST_SCENARIO_COUPLING_SWEEP:
            test_scenario_coupling_sweep(runner);
            break;
        case TEST_SCENARIO_VELOCITY_RAMP:
            test_scenario_velocity_ramp(runner);
            break;
        case TEST_SCENARIO_RAPID_FIRE:
            test_scenario_rapid_fire(runner);
            break;
        case TEST_SCENARIO_MODE_ISOLATION:
            test_scenario_mode_isolation(runner);
            break;
        case TEST_SCENARIO_HARMONIC_SERIES:
            test_scenario_harmonic_series(runner);
            break;
        case TEST_SCENARIO_RESONANCE_SWEEP:
            test_scenario_resonance_sweep(runner);
            break;
        case TEST_SCENARIO_BRIGHTNESS_SWEEP:
            test_scenario_brightness_sweep(runner);
            break;
        case TEST_SCENARIO_PITCH_BEND_SWEEP:
            test_scenario_pitch_bend_sweep(runner);
            break;
        case TEST_SCENARIO_CHAOS:
            test_scenario_chaos(runner);
            break;
        default:
            break;
    }

    return true;
}

void test_runner_process_audio(test_runner_t* runner,
                               const float* left,
                               const float* right,
                               size_t size) {
    if (!runner->running) return;

    for (size_t i = 0; i < size; i++) {
        float sample = fabsf(left[i]) + fabsf(right[i]);  // Mono sum

        if (sample > runner->peak_amplitude) {
            runner->peak_amplitude = sample;
        }

        runner->avg_amplitude += sample;
        runner->sample_count++;
    }
}

void test_runner_log_results(test_runner_t* runner) {
    float avg = (runner->sample_count > 0) ?
               (runner->avg_amplitude / runner->sample_count) : 0.0f;

    TEST_LOG("\n=== Test Complete: %s ===", test_scenario_name(runner->scenario));
    TEST_LOG("Duration: %d ms", runner->elapsed_ms);
    TEST_LOG("Events triggered: %d", runner->event_count);
    TEST_LOG("Peak amplitude: %.3f", runner->peak_amplitude);
    TEST_LOG("Average amplitude: %.3f", avg);
    TEST_LOG("Samples processed: %d", runner->sample_count);
    TEST_LOG("=====================================\n");
}

const char* test_scenario_name(test_scenario_t scenario) {
    switch (scenario) {
        case TEST_SCENARIO_SINGLE_POKE:       return "Single Poke Decay";
        case TEST_SCENARIO_NOTE_SEQUENCE:     return "Note Sequence";
        case TEST_SCENARIO_DAMPING_SWEEP:     return "Damping Sweep";
        case TEST_SCENARIO_COUPLING_SWEEP:    return "Coupling Sweep";
        case TEST_SCENARIO_VELOCITY_RAMP:     return "Velocity Ramp";
        case TEST_SCENARIO_RAPID_FIRE:        return "Rapid Fire";
        case TEST_SCENARIO_MODE_ISOLATION:    return "Mode Isolation";
        case TEST_SCENARIO_HARMONIC_SERIES:   return "Harmonic Series";
        case TEST_SCENARIO_RESONANCE_SWEEP:   return "Resonance Sweep";
        case TEST_SCENARIO_BRIGHTNESS_SWEEP:  return "Brightness Sweep";
        case TEST_SCENARIO_PITCH_BEND_SWEEP:  return "Pitch Bend Sweep";
        case TEST_SCENARIO_CHAOS:             return "Chaos Mode";
        default:                              return "Unknown";
    }
}

// ============================================================================
// Scenario Implementations
// ============================================================================

/**
 * @brief Single poke, measure decay envelope
 */
void test_scenario_single_poke(test_runner_t* runner) {
    // Trigger single note at start
    if (runner->elapsed_ms == 0) {
        simulate_note_on(runner, 60, 100);  // Middle C, forte
    }
    // Let it decay naturally for 10 seconds
}

/**
 * @brief Play a melody sequence
 */
void test_scenario_note_sequence(test_runner_t* runner) {
    // C major scale: C D E F G A B C
    const uint8_t notes[] = {60, 62, 64, 65, 67, 69, 71, 72};
    const int num_notes = sizeof(notes) / sizeof(notes[0]);
    const uint32_t note_interval = 800;  // 800ms per note

    uint32_t note_idx = runner->elapsed_ms / note_interval;

    // Trigger note at start of each interval
    if (note_idx < num_notes && (runner->elapsed_ms % note_interval) == 0) {
        simulate_note_on(runner, notes[note_idx], 80);
    }
}

/**
 * @brief Sweep damping from low to high
 */
void test_scenario_damping_sweep(test_runner_t* runner) {
    // Trigger poke every second
    if (runner->elapsed_ms % 1000 == 0 && runner->elapsed_ms > 0) {
        simulate_note_on(runner, 60, 100);
    }

    // Sweep damping continuously (CC1 = Mod Wheel)
    // 0.0 → 1.0 over 10 seconds
    float t = runner->elapsed_ms / (float)TEST_DURATION_MS;
    uint8_t cc_val = (uint8_t)(t * 127.0f);
    simulate_cc(runner, 1, cc_val);  // CC1 = damping
}

/**
 * @brief Sweep coupling strength
 */
void test_scenario_coupling_sweep(test_runner_t* runner) {
    // Trigger pokes every 500ms
    if (runner->elapsed_ms % 500 == 0 && runner->elapsed_ms > 0) {
        simulate_note_on(runner, 60 + (rand() % 12), 80);  // Random note in octave
    }

    // Sweep coupling (CC2 = Breath)
    float t = runner->elapsed_ms / (float)TEST_DURATION_MS;
    uint8_t cc_val = (uint8_t)(t * 127.0f);
    simulate_cc(runner, 2, cc_val);
}

/**
 * @brief Notes with increasing velocity
 */
void test_scenario_velocity_ramp(test_runner_t* runner) {
    const uint32_t interval = 800;
    uint32_t note_idx = runner->elapsed_ms / interval;

    if ((runner->elapsed_ms % interval) == 0 && note_idx < 12) {
        // Velocity from 10 to 127
        uint8_t velocity = 10 + (note_idx * 10);
        if (velocity > 127) velocity = 127;

        simulate_note_on(runner, 60, velocity);
    }
}

/**
 * @brief Rapid poke events (stress test)
 */
void test_scenario_rapid_fire(test_runner_t* runner) {
    // Trigger note every 50ms (20 Hz)
    if (runner->elapsed_ms % 50 == 0) {
        uint8_t note = 48 + (rand() % 24);  // Random note C3-C5
        uint8_t velocity = 60 + (rand() % 40);  // Random velocity 60-100
        simulate_note_on(runner, note, velocity);
    }
}

/**
 * @brief Test each mode individually
 */
void test_scenario_mode_isolation(test_runner_t* runner) {
    // Divide test into 4 segments (2.5s each)
    uint32_t segment = runner->elapsed_ms / 2500;

    // At start of each segment, reset mode gains
    if (runner->elapsed_ms % 2500 == 0) {
        // Mute all modes except current
        for (int k = 0; k < MAX_MODES; k++) {
            uint8_t gain = (k == segment) ? 127 : 0;
            simulate_cc(runner, 20 + k, gain);  // CC 20-23 = mode gains
        }

        // Trigger poke
        simulate_note_on(runner, 60, 100);
    }
}

/**
 * @brief Play harmonic series
 */
void test_scenario_harmonic_series(test_runner_t* runner) {
    // Play harmonics of C2 (MIDI 36): 36, 48, 60, 67, 72, 76, 79, 81
    const uint8_t harmonics[] = {36, 48, 60, 67, 72, 76, 79, 81, 83, 84};
    const int num_harmonics = sizeof(harmonics) / sizeof(harmonics[0]);
    const uint32_t interval = 1000;

    uint32_t idx = runner->elapsed_ms / interval;

    if ((runner->elapsed_ms % interval) == 0 && idx < num_harmonics) {
        simulate_note_on(runner, harmonics[idx], 90);
    }
}

/**
 * @brief Sweep resonance parameter (CC71)
 */
void test_scenario_resonance_sweep(test_runner_t* runner) {
    // Trigger poke every 800ms
    if (runner->elapsed_ms % 800 == 0 && runner->elapsed_ms > 0) {
        simulate_note_on(runner, 60, 100);
    }

    // Sweep resonance: low → high → low (triangle wave)
    float t = runner->elapsed_ms / (float)TEST_DURATION_MS;
    float triangle = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
    uint8_t cc_val = (uint8_t)(triangle * 127.0f);
    simulate_cc(runner, 71, cc_val);  // CC71 = Resonance
}

/**
 * @brief Sweep brightness (CC74)
 */
void test_scenario_brightness_sweep(test_runner_t* runner) {
    // Continuous note-on every 600ms
    if (runner->elapsed_ms % 600 == 0 && runner->elapsed_ms > 0) {
        simulate_note_on(runner, 60, 80);
    }

    // Sweep brightness
    float t = runner->elapsed_ms / (float)TEST_DURATION_MS;
    uint8_t cc_val = (uint8_t)(t * 127.0f);
    simulate_cc(runner, 74, cc_val);  // CC74 = Brightness
}

/**
 * @brief Pitch bend sweep
 */
void test_scenario_pitch_bend_sweep(test_runner_t* runner) {
    // Trigger note at start
    if (runner->elapsed_ms == 0) {
        simulate_note_on(runner, 60, 100);
    }

    // Sweep pitch bend continuously
    // -8192 to +8191 (±2 semitones)
    float t = runner->elapsed_ms / (float)TEST_DURATION_MS;
    int16_t bend = (int16_t)((t * 2.0f - 1.0f) * 8192.0f);  // -1 to +1
    simulate_pitch_bend(runner, bend);
}

/**
 * @brief Random chaos (stress test and sonic exploration)
 */
void test_scenario_chaos(test_runner_t* runner) {
    // Random notes at random intervals
    static uint32_t next_note_time = 0;

    if (runner->elapsed_ms >= next_note_time) {
        uint8_t note = 36 + (rand() % 48);  // C2 to C6
        uint8_t velocity = 40 + (rand() % 88);
        simulate_note_on(runner, note, velocity);

        // Next note in 50-500ms
        next_note_time = runner->elapsed_ms + 50 + (rand() % 450);
    }

    // Random CC changes every 200ms
    if (runner->elapsed_ms % 200 == 0) {
        int cc_choice = rand() % 4;
        uint8_t cc_num = 0;

        switch (cc_choice) {
            case 0: cc_num = 1; break;   // Damping
            case 1: cc_num = 71; break;  // Resonance
            case 2: cc_num = 74; break;  // Brightness
            case 3: cc_num = 2; break;   // Coupling
        }

        uint8_t cc_val = rand() % 128;
        simulate_cc(runner, cc_num, cc_val);
    }
}
