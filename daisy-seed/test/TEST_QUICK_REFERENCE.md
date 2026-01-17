# Test Scenarios Quick Reference

**Quick lookup table for all automated test scenarios**

| # | Scenario | Duration | Events | What It Tests | Expected Sound |
|---|----------|----------|--------|---------------|----------------|
| 1 | **Single Poke Decay** | 10s | 1 | Decay envelope accuracy | Single "ping" → exponential decay |
| 2 | **Note Sequence** | 10s | 8 | MIDI processing | C major scale melody |
| 3 | **Damping Sweep** | 10s | 10 | Damping parameter | Long decay → short decay |
| 4 | **Coupling Sweep** | 10s | 20 | Coupling strength | Random notes, increasing connection |
| 5 | **Velocity Ramp** | 10s | 12 | Velocity scaling | Quiet → loud crescendo |
| 6 | **Rapid Fire** | 10s | 200 | High event rate | Dense, chaotic texture |
| 7 | **Mode Isolation** | 10s | 4 | Individual modes | 4 distinct tones (one per mode) |
| 8 | **Harmonic Series** | 10s | 10 | Harmonic relationships | Rising pitch series |
| 9 | **Resonance Sweep** | 10s | 12 | Resonance control (CC71) | Varying decay times |
| 10 | **Brightness Sweep** | 10s | 16 | Brightness control (CC74) | Dark → bright timbre |
| 11 | **Pitch Bend Sweep** | 10s | 1 | Pitch bend | Sustained tone with glide |
| 12 | **Chaos Mode** | 10s | ~50 | Stress test | Random, evolving texture |

## Build Commands

```bash
# Normal mode (USB MIDI)
pio run -e daisy_seed --target upload

# Test mode (automated scenarios)
pio run -e daisy_seed_test --target upload

# Monitor serial output
pio device monitor
```

## Serial Output Format

```
=== Starting Test: [Name] ===
Duration: 10000 ms

[ 100 ms] Peak: 0.743  Avg: 0.389  Events: 1
[ 200 ms] Peak: 0.648  Avg: 0.341  Events: 1
...

=== Test Complete: [Name] ===
Duration: 10000 ms
Events triggered: [N]
Peak amplitude: 0.952
Average amplitude: 0.284
Samples processed: 480000
```

## LED Indicators

- **5 quick blinks**: System ready
- **3 blinks**: New test starting
- **Solid on**: Inter-test delay (2 seconds)
- **Off**: Test running

## Parameter Ranges

| Parameter | CC | Range | Effect |
|-----------|----|----|--------|
| Damping | 1 | 0-127 | Decay time (high = fast decay) |
| Coupling | 2 | 0-127 | Inter-mode coupling |
| Resonance | 71 | 0-127 | Inverse damping (high = long decay) |
| Brightness | 74 | 0-127 | Mode 2 gain (high = bright) |
| Mode 0 Gain | 20 | 0-127 | Mode 0 level |
| Mode 1 Gain | 21 | 0-127 | Mode 1 level |
| Mode 2 Gain | 22 | 0-127 | Mode 2 level |
| Mode 3 Gain | 23 | 0-127 | Mode 3 level |

## Expected CPU Usage

| Scenario | Typical CPU % |
|----------|---------------|
| Single Poke | 10-12% |
| Note Sequence | 12-14% |
| Rapid Fire | 15-18% |
| Chaos Mode | 16-20% |

## Recording Setup

1. Connect Daisy OUT L/R to audio interface
2. Set interface to 48kHz
3. Upload test firmware: `pio run -e daisy_seed_test --target upload`
4. Start recording
5. Total duration: ~2.5 minutes (12 × 10s + pauses)
6. Stop after cycle completes

## Troubleshooting Quick Fixes

| Problem | Solution |
|---------|----------|
| No serial output | Check USB connection, baud rate 115200 |
| No audio | Verify connections, check master gain |
| Test doesn't start | Re-upload firmware, check LED blinks |
| Dropouts | Increase block size to 96 |
| Too loud | Reduce `MAX_AMPLITUDE_SCALE` in audio_synth.c |
| Too quiet | Increase mode weights or master gain |

## Customization Quick Edits

### Change test duration
`test_scenarios.h`: `#define TEST_DURATION_MS 5000`

### Change inter-test pause
`main_test.cpp`: `const uint32_t INTER_TEST_DELAY_MS = 1000;`

### Change modal frequencies
`main_test.cpp` in `setup()`:
```cpp
modal_node_set_mode(&node, 0, freq_to_omega(440.0f), 0.5f, 1.0f);
```

### Skip to specific test
`main_test.cpp`:
```cpp
current_scenario = TEST_SCENARIO_RAPID_FIRE;  // Start at scenario #6
```

## File Locations

- **Test scenarios**: `src/test/test_scenarios.h/.cpp`
- **Test main**: `src/test/main_test.cpp`
- **Build config**: `platformio.ini` → `[env:daisy_seed_test]`
- **Documentation**: `test/TEST_SCENARIOS_GUIDE.md`

---

For detailed descriptions of each scenario, see [TEST_SCENARIOS_GUIDE.md](TEST_SCENARIOS_GUIDE.md)
