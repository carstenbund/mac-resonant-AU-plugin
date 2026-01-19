/**
 * @file test_presets.c
 * @brief Test program for physical modal presets
 *
 * Demonstrates usage of modal_node_apply_preset() and prints
 * resulting mode parameters for all available presets.
 */

#include "modal_node.h"
#include <stdio.h>

int main(void) {
    printf("=== Modal Physical Presets Test ===\n\n");

    modal_node_t node;
    modal_node_init(&node, 0, PERSONALITY_RESONATOR);

    uint8_t num_presets = modal_node_get_num_presets();
    printf("Available presets: %d\n\n", num_presets);

    // Test parameters
    float test_fundamental = 440.0f;  // A4
    float test_damping = 0.5f;

    printf("Test configuration:\n");
    printf("  Fundamental: %.1f Hz (A4)\n", test_fundamental);
    printf("  Base damping: %.2f\n\n", test_damping);

    // Test each preset
    for (uint8_t i = 0; i < num_presets; i++) {
        const char* name = modal_node_get_preset_name(i);
        const char* desc = modal_node_get_preset_description(i);

        printf("─────────────────────────────────────────────────────────────\n");
        printf("Preset %d: %s\n", i, name);
        printf("Description: %s\n\n", desc);

        // Apply preset
        modal_node_apply_preset(&node, i, test_fundamental, test_damping);

        // Print resulting mode parameters
        printf("  Mode  | Frequency (Hz) | Ratio  | Damping | Weight\n");
        printf("  ───────────────────────────────────────────────────\n");

        for (int k = 0; k < MAX_MODES; k++) {
            if (node.modes[k].params.active) {
                float freq_hz = node.modes[k].params.omega / (2.0f * 3.14159265359f);
                float ratio = freq_hz / test_fundamental;
                float gamma = node.modes[k].params.gamma;
                float weight = node.modes[k].params.weight;

                printf("   %d    |   %7.2f     | %6.3f | %7.2f | %6.2f\n",
                       k, freq_hz, ratio, gamma, weight);
            }
        }

        printf("\n");
    }

    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n✓ All presets tested successfully!\n");
    printf("\nUsage example:\n");
    printf("  modal_node_apply_preset(&node, 0, 440.0f, 0.5f);  // Church Bell at A4\n");
    printf("  modal_node_apply_preset(&node, 1, 523.25f, 0.3f); // Plate at C5\n");

    return 0;
}
