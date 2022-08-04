// -*- coding:utf-8-unix -*-
/**
 * @file   LM61.cpp
 * @author alnasl
 * @date   Mon Aug  1 15:15:18 2022
 *
 * @brief  Act driver for LM61 analog temperature sensors
 *
 * Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT).
 */

// Include Files //////////////////////////////////////////////////////////////

#include "LM61.hpp"

#include <TWELITE>


// Static Objects /////////////////////////////////////////////////////////////


// Static Methods /////////////////////////////////////////////////////////////


// Public Methods /////////////////////////////////////////////////////////////

LM61::LM61()
    : pin_analogue(PIN_ANALOGUE::A1),
      temp_offset{.is_negative = false, .integer = 0, .fractional = 0}
{
}

void LM61::setup(const uint8_t pin, const float offset) {
    // Set analogue input pin
    this->pin_analogue = pin;
    // Set Temperature offset
    this->temp_offset = this->ConvertToStruct(offset);
    // Setup ADC
    Analogue.setup(true);
}


lm61_temperature_s LM61::read()
{
    // Calculation
    // Equivalent of (Analogue.read() - LM61_OUTPUT_AT_0) / 10
    auto div_result = div10(Analogue.read(this->pin_analogue) - LM61_OUTPUT_AT_0);
    lm61_temperature_s temperature = {
        .is_negative = div_result.b_neg ? true : false,
        .integer = this->temp_offset.is_negative ? div_result.quo - this->temp_offset.integer : div_result.quo + this->temp_offset.integer,
        .fractional = this->temp_offset.is_negative ? div_result.rem - this->temp_offset.fractional : div_result.rem + this->temp_offset.fractional
    };
    return temperature;
}

int16_t LM61::read_cent()
{
    return this->ConvertToCent(this->read());
}

float LM61::read_float()
{
    return this->ConvertToFloat(this->read());
}
