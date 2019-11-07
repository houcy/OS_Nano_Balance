/*  OS Nano Balance uses a load cell to measure and report an object's mass.
    Copyright (C) 2019 Benjamin Hubbard

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// External Libraries.
// Load cell amplifier.
#include "src\HX711\src\HX711.h"
// Hard Memory read/write.
#include <EEPROM.h>


// Globals
// HX711 Pins.
// Data Pin
const int hx_dt 	= 2;
// Clock Pin
const int hx_sck = 3;
// Vcc Pin
const int hx_vcc = 4;

// HX711 Constants.
// Number of averages while measuring.
const int hx_num_avgs = 1;
// Number of averages while calibrating.
const int hx_cal_num_avgs = 10;

// HX711 object.
HX711 loadcell;

// Calibration (EEPROM).
// Signature to store in the memory when calibrating.
char cal_sig = 'C';
// Variable to store calibration signature check.
char cal_check;
// Address of calibration signature.
int cal_sig_addr = 0;
// Address of the stored calibration value.
int cal_val_addr = sizeof(char);
// Stores the calibration value from EEPROM.
float cal_val = 0.00f;
// Mass used to calibrate (1 US cup of water);
double standard_mass = 0.2359;
// Mass units.
String units = "kg";
// Sensitivity value.
float sensitivity;

// Button pins.
// Calibrate.
const int btn_cal = 7;
// Tare.
const int btn_tare = 8;

// Readouts.
// Raw value.
double val;
// Scaled value.
float mass;


// initLoadCell() - set up the HX711 for use. Turns on the device, then verifies
// the calibration value.
HX711 initLoadCell() {
	Serial.println("Initializing HX711...");
	
	// Setup HX711 Power supply.
	pinMode(hx_vcc, OUTPUT);
	digitalWrite(hx_vcc, HIGH);
	
	// Give it time to power on.
	delay(500);
	
	// Initialize.
	HX711 loadcell;
	loadcell.begin(hx_dt, hx_sck);
	
	// Wait until it's ready.
	bool is_ready = false;
	int num_retries = 3;
	int wait_delay = 200;
	while (!is_ready) {
		// Give some indication that there is thought occurring.
		Serial.print("...");
		is_ready = loadcell.wait_ready_retry(num_retries, wait_delay);
	}
	
	Serial.println("HX711 Initialized!");
	
	// Set calibration value.
	// Check if the calibration has been saved.
	Serial.println("Loading calibration...");
	EEPROM.get(cal_sig_addr, cal_check);
	if (cal_check == cal_sig) {
		EEPROM.get(cal_val_addr, cal_val);
		Serial.print("Calibration value: ");
		Serial.print(cal_val);
		Serial.print(" ");
		Serial.print(units);
		Serial.println("/div");
		loadcell.set_scale(cal_val);
	} else {
		Serial.println("Calibration value is not stored.");
		Serial.println("Defaulting to calibration of 1 (raw value)");
		loadcell.set_scale(1.00f);
	}

	delay(500);
	
	Serial.println("Tare...");
	loadcell.tare(hx_num_avgs);
	
	return loadcell;
}


// Calibrate() calibrates the load cell.
// loadcell = load cell amplifier object.
// standard_mass = mass used to calibrate.
// units = units to display in the readout. 
void calibrate() {	
	// Give the user a second to release the button.
	delay(1000);
		
	while (digitalRead(btn_cal) == 1) {
		// Start reading, wait until calibration is done.
		if (digitalRead(btn_tare) == 0) {
			// TODO: Make a custom tare function to keep this DRY.
			Serial.println("Tare...");
			loadcell.tare(hx_num_avgs);
		}
		
		// Read the tared value (raw reading).
		val = loadcell.get_value(hx_cal_num_avgs);
		Serial.print("Reading: ");
		Serial.println(val);
		
		// Calculate the sensitivity.
		sensitivity = (float) (val / standard_mass);
			
		// Let the user know what they just set it to.
		Serial.print("New sensitivity: ");
		Serial.print(sensitivity);
		Serial.print(" ");
		Serial.println(units);
	}
	// Set and store the sensitivity.
	// TODO: Make a print_with_units function.
	Serial.print("Using sensitivity: ");
	Serial.print(sensitivity);
	Serial.print(" ");
	Serial.println(units);
	loadcell.set_scale(sensitivity);
	EEPROM.put(cal_sig_addr, cal_sig);
	EEPROM.put(cal_val_addr, sensitivity);
	delay(500);
	return;
}


void setup() {
	Serial.begin(115200);
	while(!Serial){
		// Wait for serial to initialize.
	}
	
	::loadcell = initLoadCell();
	
	/* Tare and Calibarion sensors. Internal pullup resistor defaults the pin to 
	 * HIGH, meaning the tare must be connected to ground when closed/active.
	 */
	pinMode(::btn_cal, INPUT_PULLUP);
	pinMode(::btn_tare, INPUT_PULLUP);
}


void loop() {
	// TODO: make my own running average algorithm.
	// See what's being measured without the calibration (but with tare offset).
	val = loadcell.get_value(hx_num_avgs);
	// Calibrated measurement.
	mass = loadcell.get_units(hx_num_avgs);
	
	Serial.print("Raw value: ");
	Serial.print(val);
	Serial.print(", Mass: ");
	Serial.print(mass);
	Serial.print(" ");
	Serial.println(units);
	
	if (digitalRead(btn_tare) == 0) {
		Serial.println("Tare...");
		loadcell.tare(hx_num_avgs);
	}
	
	if (digitalRead(btn_cal) == 0) {
		Serial.println("Calibrate...");
		calibrate();
	}
}