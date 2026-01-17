/**
 * @file test_scenarios.h
 * @brief Automated test scenarios with simulated MIDI input
 *
 * This module provides automated testing of the modal resonator
 * by simulating MIDI events and parameter changes. Each scenario
 * runs for a fixed duration and generates specific sonic behaviors.
 *
 * Features:
 * - Automated MIDI note sequences
 * - Parameter sweeps (damping, coupling, gains)
 * - Audio analysis and logging
 * - Reproducible test conditions
 *
 * Usage:
 * - Enable TEST_MODE in build flags
 * - Select scenario in main.cpp
 * - Monitor audio output and serial logs
 */

#ifndef TEST_SCENARIOS_H
#define TEST_SCENARIOS_H

#include <stdint.h>
#include <stdbool.h>

extern "C" {
#include "core/modal_node.h"
#include "audio/audio_synth.h"
#include "midi/usb_midi_handler.h"
}

// ============================================================================
// Test Configuration
// ============================================================================

#define TEST_DURATION_MS 10000  // 10 seconds per test
#define AUDIO_LOG_INTERVAL_MS 100  // Log every 100ms

// ============================================================================
// Test Scenario Types
// ============================================================================

/**
 * @brief Available test scenarios
 */
typedef enum {
    TEST_SCENARIO_SINGLE_POKE,       ///< Single note-on, measure decay
    TEST_SCENARIO_NOTE_SEQUENCE,     ///< Melody sequence
    TEST_SCENARIO_DAMPING_SWEEP,     ///< Sweep damping parameter
    TEST_SCENARIO_COUPLING_SWEEP,    ///< Sweep coupling strength
    TEST_SCENARIO_VELOCITY_RAMP,     ///< Notes with increasing velocity
    TEST_SCENARIO_RAPID_FIRE,        ///< Rapid poke events
    TEST_SCENARIO_MODE_ISOLATION,    ///< Test individual modes
    TEST_SCENARIO_HARMONIC_SERIES,   ///< Play harmonic series
    TEST_SCENARIO_RESONANCE_SWEEP,   ///< Sweep resonance (CC71)
    TEST_SCENARIO_BRIGHTNESS_SWEEP,  ///< Sweep brightness (CC74)
    TEST_SCENARIO_PITCH_BEND_SWEEP,  ///< Pitch bend automation
    TEST_SCENARIO_CHAOS,             ///< Random notes and parameters
    TEST_SCENARIO_COUNT              ///< Number of scenarios
} test_scenario_t;

// ============================================================================
// Test State
// ============================================================================

/**
 * @brief Test runner state
 */
typedef struct {
    test_scenario_t scenario;        ///< Current scenario
    uint32_t start_time_ms;          ///< Test start timestamp
    uint32_t elapsed_ms;             ///< Elapsed time
    uint32_t last_log_ms;            ///< Last audio log timestamp
    uint32_t event_count;            ///< Number of events triggered
    bool running;                    ///< Test running flag

    // Audio analysis
    float peak_amplitude;            ///< Peak amplitude this test
    float avg_amplitude;             ///< Average amplitude
    uint32_t sample_count;           ///< Samples processed

    // References
    modal_node_t* node;
    audio_synth_t* synth;
    midi_handler_t* midi_handler;
} test_runner_t;

// ============================================================================
// Core API
// ============================================================================

/**
 * @brief Initialize test runner
 *
 * @param runner Pointer to test runner state
 * @param node Pointer to modal node
 * @param synth Pointer to audio synthesis
 * @param midi_handler Pointer to MIDI handler
 */
void test_runner_init(test_runner_t* runner,
                     modal_node_t* node,
                     audio_synth_t* synth,
                     midi_handler_t* midi_handler);

/**
 * @brief Start a test scenario
 *
 * @param runner Pointer to test runner
 * @param scenario Scenario to run
 */
void test_runner_start(test_runner_t* runner, test_scenario_t scenario);

/**
 * @brief Stop current test
 *
 * @param runner Pointer to test runner
 */
void test_runner_stop(test_runner_t* runner);

/**
 * @brief Update test (call in main loop)
 *
 * Generates MIDI events and parameter changes according to scenario.
 * Should be called at ~1kHz.
 *
 * @param runner Pointer to test runner
 * @return true if test is still running, false if completed
 */
bool test_runner_update(test_runner_t* runner);

/**
 * @brief Process audio for analysis
 *
 * Call this from audio callback to track amplitude statistics.
 *
 * @param runner Pointer to test runner
 * @param left Left channel buffer
 * @param right Right channel buffer
 * @param size Buffer size
 */
void test_runner_process_audio(test_runner_t* runner,
                               const float* left,
                               const float* right,
                               size_t size);

/**
 * @brief Log test results
 *
 * Prints scenario name, duration, peak/avg amplitude, etc.
 *
 * @param runner Pointer to test runner
 */
void test_runner_log_results(test_runner_t* runner);

/**
 * @brief Get scenario name
 *
 * @param scenario Scenario enum
 * @return Human-readable name
 */
const char* test_scenario_name(test_scenario_t scenario);

// ============================================================================
// Scenario Implementations (internal)
// ============================================================================

void test_scenario_single_poke(test_runner_t* runner);
void test_scenario_note_sequence(test_runner_t* runner);
void test_scenario_damping_sweep(test_runner_t* runner);
void test_scenario_coupling_sweep(test_runner_t* runner);
void test_scenario_velocity_ramp(test_runner_t* runner);
void test_scenario_rapid_fire(test_runner_t* runner);
void test_scenario_mode_isolation(test_runner_t* runner);
void test_scenario_harmonic_series(test_runner_t* runner);
void test_scenario_resonance_sweep(test_runner_t* runner);
void test_scenario_brightness_sweep(test_runner_t* runner);
void test_scenario_pitch_bend_sweep(test_runner_t* runner);
void test_scenario_chaos(test_runner_t* runner);

#endif // TEST_SCENARIOS_H
