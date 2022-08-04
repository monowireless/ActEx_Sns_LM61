// -*- coding:utf-8-unix -*-
/**
 * @file   LM61.hpp
 * @author alnasl
 * @date   Mon Aug  1 15:15:18 2022
 *
 * @brief  Act driver for LM61 analog temperature sensors
 *
 * Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT).
 */

#pragma once

// Include Files //////////////////////////////////////////////////////////////

#include <TWELITE>

#include <stdint.h>             // For uint8_t


// Macro Definitions //////////////////////////////////////////////////////////

/// Output voltage at 0 degrees (mV)
#define LM61_OUTPUT_AT_0 600


// Type Definitions ///////////////////////////////////////////////////////////

struct lm61_temperature_s {
    bool is_negative;
    uint8_t integer;
    uint8_t fractional;
};


// Class Definition ///////////////////////////////////////////////////////////

class LM61 {
private:
    /**
     * @brief   Analogue input pin connected to the LM61's Vout pin
     */
    uint8_t pin_analogue;
    /**
     * @brief   Temperature offset value
     */
    lm61_temperature_s temp_offset;

public:
    /**
     * @fn      LM61
     * @brief   Constructor
     */
    LM61();

    /**
     * @fn      ~LM61
     * @brief   Destructor
     */
    ~LM61() {}

public:
    /**
     * @fn      setup
     * @brief   Setup LM61
     *
     * @param   pin     Analogue input pin (Default: PIN_ANALOGUE::A1)
     * @param   offset  Temperature offset (Default: 0.0f)
     * @return  none
     */
    void setup(const uint8_t pin = PIN_ANALOGUE::A1, const float offset = 0.0f);

    /**
     * @fn      begin
     * @brief   Begin sensing
     *
     * @param   none
     * @return  none
     */
    inline void begin() {
        // Begin ADC
        Analogue.begin(pack_bits(this->pin_analogue));
    }

    /**
     * @fn      end
     * @brief   End sensing
     *
     * @param   none
     * @return  none
     */
    inline void end() {
        Analogue.end();
    }

    /**
     * @fn      available
     * @brief   Check if the input is available
     *
     * @param   none
     * @retval  true    Available
     * @retval  false   Not available
     */
    inline bool available() {
        return Analogue.available();
    }

    /**
     * @fn      read
     * @brief   Read temperature
     *
     * @param   none
     * @return  Temperature as lm61_temperature_s struct
     */
    lm61_temperature_s read();

    /**
     * @fn      read_cent
     * @brief   Read temperature (hundredfold)
     *
     * @param   none
     * @return  Temperature (hundredfold)
     */
    int16_t read_cent();

    /**
     * @fn      read_float
     * @brief   Read temperature in float (slow)
     *
     * @param   none
     * @return  Temperature as float
     */
    float read_float();

public:
    inline lm61_temperature_s ConvertToStruct(const int16_t temperature) {
        auto divided = div100(temperature);
        return {
            .is_negative = divided.b_neg ? true : false,
            .integer = divided.quo,
            .fractional = divided.rem
        };
    }
    inline lm61_temperature_s ConvertToStruct(const float temperature) {
        auto divided = div100((int16_t)(temperature * 100));
        return {
            .is_negative = divided.b_neg ? true : false,
            .integer = divided.quo,
            .fractional = divided.rem
        };
    }
    inline int16_t ConvertToCent(const lm61_temperature_s temperature) {
        int16_t converted = temperature.integer * 100 + temperature.fractional;
        if (temperature.is_negative) {
            converted *= -1;
        }
        return converted;
    }
    inline int16_t ConvertToCent(const float temperature) {
        return (int16_t)(temperature * 100.0f);
    }
    inline float ConvertToFloat(const lm61_temperature_s temperature) {
        float converted = temperature.integer + temperature.fractional / 100.0f;
        if (temperature.is_negative) {
            converted *= -1;
        }
        return converted;
    }
    inline float ConvertToFloat(const int16_t temperature) {
        return temperature / 100.0f;
    }
};
