/**
 * @file CvAdapter.h
 * @brief Thin adapter for CV/Gate input to InputMapper
 *
 * Provides a simple pass-through interface for CV/Gate signals to the
 * protocol-agnostic InputMapper. This adapter is designed for Eurorack
 * and hardware modular synthesizer control (e.g., ESP32 ADC inputs).
 */

#pragma once

#include "InputMapper.h"

/**
 * @brief CV/Gate input adapter
 *
 * Thin wrapper that forwards CV voltage readings to an InputMapper instance.
 * Designed for use with ESP32 ADC loops or other CV input systems.
 */
class CvAdapter {
public:
    /**
     * @brief Construct adapter with reference to mapper
     * @param mapper Input mapper to forward CV signals to
     */
    explicit CvAdapter(InputMapper& mapper)
        : mapper_(mapper)
    {}

    /**
     * @brief Set CV pitch input (1V/octave standard)
     * @param v CV pitch voltage
     */
    void setPitchVolts(float v) {
        mapper_.setCvPitchVolts(v);
    }

    /**
     * @brief Set CV energy input
     * @param v CV energy voltage [0..5V]
     */
    void setEnergyVolts(float v) {
        mapper_.setCvEnergyVolts(v);
    }

    /**
     * @brief Set CV gate input
     * @param v CV gate voltage (>2.5V = high)
     */
    void setGateVolts(float v) {
        mapper_.setCvGateVolts(v);
    }

    /**
     * @brief Set CV trigger input
     * @param v CV trigger voltage (>2.5V = high)
     */
    void setTrigVolts(float v) {
        mapper_.setCvTrigVolts(v);
    }

    /**
     * @brief Set CV damping control input
     * @param v CV damping voltage [0..5V]
     */
    void setDampVolts(float v) {
        mapper_.setCvDampVolts(v);
    }

    /**
     * @brief Set CV brightness control input
     * @param v CV brightness voltage [0..5V]
     */
    void setBrightVolts(float v) {
        mapper_.setCvBrightVolts(v);
    }

private:
    InputMapper& mapper_;
};
