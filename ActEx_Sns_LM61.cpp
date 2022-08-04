// -*- coding:utf-8-unix -*-
/**
 * @file   ActEx_Sns_LM61.cpp
 * @author alnasl
 * @date   Mon Aug  1 13:22:32 2022
 *
 * @brief  Act sample for LM61 analog temperature sensors. Send data as PAM_AMB in Act_samples.
 *
 * Copyright (C) 2022 Mono Wireless Inc. All Rights Reserved.
 * Released under MW-OSSLA-*J,*E (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT).
 */

// Include Files //////////////////////////////////////////////////////////////

#include <TWELITE>
#include <NWK_SIMPLE>
#include <STG_STD>
#include <SM_SIMPLE>

#include "AQM0802/AQM0802.hpp"      // AQM0802 local library for TWELITE
#include "LM61/LM61.hpp"            // LM61 local library for TWELITE


// Macro Definitions //////////////////////////////////////////////////////////

#define CELSIUS_SYMBOL uint8_t(0xF2) << 'C' // Degree Celsius symbol for ST7032 display controller


// Function Prototypes ////////////////////////////////////////////////////////

void sleepNow();


// Global Objects /////////////////////////////////////////////////////////////

AQM0802 display;                // AQM0802 Display object
LM61 temp_sensor;               // LM61 Sensor object

const uint32_t DEFAULT_APP_ID = 0x1234abcd;     // Default Application ID
const uint8_t DEFAULT_CHANNEL = 13;             // Channel
uint32_t OPT_BITS = 0;                          // Option bits
uint8_t LID = 0;                                // Logical ID

const char FOURCHARS[] = "PAB1";    // Act Identifier

/// Sensor value
struct {
	uint32_t u32luminance;
	int16_t i16temp;
	int16_t i16humid;
} sensor;

/// Application states
enum class STATE : uint8_t {
	INTERACTIVE = 255,
	INIT = 0,
	SENSOR,
	TX,
	TX_WAIT_COMP,
	GO_SLEEP
};

// Simple state machine
SM_SIMPLE<STATE> step;


// Setup and Loop Procedure ///////////////////////////////////////////////////

/**
 * @fn      setup
 * @brief   the setup procedure (called on boot)
 *
 * @param   none
 * @return  none
 */
void setup() {
    // Initialize state machine
	step.setup();

	// Load settings and network objects
	auto&& set = the_twelite.settings.use<STG_STD>(); // load save/load settings(interactive mode) support
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>(); // load network support

	/// Configure settings
	set << SETTINGS::appname("AMB");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set.hide_items(E_STGSTD_SETID::POWER_N_RETRY, E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);

    // Interactive mode
    if (digitalRead(PIN_DIGITAL::DIO12) == PIN_STATE::LOW) {
        set << SETTINGS::open_at_start();
		step.next(STATE::INTERACTIVE);
		return;
	}

    // Load settings values
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.
	LID = set.u8devid(); // load from settings.
	if (LID == 0) LID = 0xFE; // if still 0, set 0xFE (anonymous child)

    // Apply settings
	the_twelite << set;
    the_twelite << TWENET::tx_power(3);

	/// Configure network
	nwk << set; // apply settings (from interactive mode)
	nwk << NWK_SIMPLE::logical_id(LID); // set LID again (LID can also be configured by DIP-SW.)

    // Setup the sensor
    temp_sensor.setup(PIN_ANALOGUE::A1, 0.0f);

    // Setup the display
    display.begin(TYPE_AQM1602);

    // let the TWELITE begin!
	the_twelite.begin();

    // Init message
    Serial << "--- AMB:" << FOURCHARS << " ---" << mwx::crlf;
	Serial	<< format("-- app:x%08x/ch:%d/lid:%d",
                      the_twelite.get_appid(),
                      the_twelite.get_channel(),
                      nwk.get_config().u8Lid) << mwx::crlf;
	Serial 	<< format("-- pw:%d/retry:%d/opt:x%08x",
                      the_twelite.get_tx_power(),
                      nwk.get_config().u8RetryDefault,
                      OPT_BITS) << mwx::crlf;

    display << "ActEx Sense LM61" << mwx::crlf
            << "AMB:" << FOURCHARS << mwx::crlf;
    delay(1000);
    display << format("AppID:  %08x", the_twelite.get_appid()) << mwx::crlf
            << format("Ch: %02d  LID: %03d", the_twelite.get_channel(), nwk.get_config().u8Lid) << mwx::crlf;
    delay(2000);
}

/**
 * @fn      loop
 * @brief   the loop procedure (called every event)
 *
 * @param   none
 * @return  none
 */
void loop() {
    do {
		switch (step.state()) {
		case STATE::INTERACTIVE:
            break;

		case STATE::INIT:
			// Start sensor capture
            temp_sensor.begin();
            step.next(STATE::SENSOR);
            break;

		case STATE::SENSOR:
			// Now sensor data are ready.
			if (temp_sensor.available()) {
				sensor.u32luminance = 0xFFFFFFFF;
				sensor.i16temp = temp_sensor.read_cent();
				sensor.i16humid = 0x8000;

				Serial << "..finish sensor capture." << mwx::crlf
                       << "             lumi=" << "none" << mwx::crlf
                       << "  LM61     : temp=" << div100(sensor.i16temp) << 'C' << mwx::crlf
                       << "             humd=" << "none" << '%' << mwx::crlf;
				Serial.flush();

                display << "Temp: " << div100(sensor.i16temp) << CELSIUS_SYMBOL << mwx::crlf;

				step.next(STATE::TX);
			}
            break;

		case STATE::TX:
			step.next(STATE::GO_SLEEP); // set default next state (for error handling.)

			// Get new packet instance.
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// Set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// Prepare packet payload
				pack_bytes(pkt.get_payload(), // set payload data objects.
                           make_pair(FOURCHARS, 4),  // just to see packet identification, you can design in any.
                           uint32_t(sensor.u32luminance), // luminance
                           uint16_t(sensor.i16temp),
                           uint16_t(sensor.i16humid));

				// Do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << "..transmit request by id = " << int(ret.get_value()) << '.' << mwx::crlf << mwx::flush;
				if (ret) {
					step.clear_flag(); // waiting for flag is set.
					step.set_timeout(100); // set timeout
					step.next(STATE::TX_WAIT_COMP);
				}
			}
            break;

		case STATE::TX_WAIT_COMP:
			if (step.is_timeout()) { // maybe fatal error.
				the_twelite.reset_system();
			}
			if (step.is_flag_ready()) { // when tx is performed
				Serial << "..transmit complete." << mwx::crlf;
				Serial.flush();
				step.next(STATE::GO_SLEEP);
			}
            break;

		case STATE::GO_SLEEP:
			sleepNow();
            break;

		default: // never be here!
			the_twelite.reset_system();
		}
	} while(step.b_more_loop()); // if state is changed, loop more.
}

// Wakeup procedure
void wakeup() {
	Serial	<< mwx::crlf
			<< "--- AMB:" << FOURCHARS << " wake up ---" << mwx::crlf
			<< "..start sensor capture again." << mwx::crlf;
}

// When finishing data transmit, set the flag.
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	step.set_flag(ev.bStatus);
}


// Local functions ////////////////////////////////////////////////////////////

// Perform sleeping
void sleepNow() {
	step.on_sleep(false); // reset state machine.

	// randomize sleep duration.
	uint32_t u32ct = 1750 + random(0,500);

	// output message
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf;
	Serial.flush(); // wait until all message printed.
    display << "Sleep for " << int(u32ct) << "ms" << mwx::crlf;

	// do sleep.
	the_twelite.sleep(u32ct);
}
