/*
	This library/driver is hacked from: https://github.com/automote/LTR303
	and updated to work with LTR-329 from this advice:	
		Fixes needed https://github.com/automote/LTR303/issues/2
		And also here https://forum.arduino.cc/index.php?topic=563507.15
	TO DO:
		There is an odd glitch that causes ALS LTR329 CH1 to be 1024 intermittently and that throws of the calculation. 
		See LTR329_Driver.cpp line 460. Need to track that down and fix it.
		Review LTR-329 datasheets (in Datasheets folder) and Appendix A (Lite-On_LTR-303 329_Appendix A Ver_1.0_22 Feb 2013.pdf).
		This might be of limited help: https://github.com/watterott/LTR329ALS01-Breakout
*/

/*
LTR303 illumination sensor library for Arduino
Lovelesh, thingTronics
	
MODIFIED to work better by Neil Emiro, Sep 2019
	
The MIT License (MIT)

Copyright (c) 2015 thingTronics Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

version 0.1
*/

#include "LTR329_Driver.h"
// #include <i2c_t3.h>  //use this for Teensy
#include <Wire.h> //use this for most Arduinos

LTR303::LTR303(void) {
	// LTR303 object
}

boolean LTR303::begin(void) {
	// Initialize LTR303 library with default address (0x39)
	// Always returns true

	_i2c_address = LTR303_ADDR;
//	Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);  //this sets up the I2C 'zero' bus (ie, SDA0/SCL0).  Would be Wire1.begin for the 'one' bus (SDA1/SCL1)
//	Wire.setDefaultTimeout(200000); // 200ms
	return(true);
}

boolean LTR303::setPowerUp(void) {
	// Turn on LTR303, begin integrations
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)

	// Write 0x01 (reset = 0 & mode = 1) to command byte (power on)
	return(writeByte(LTR303_CONTR,0x01));
}

boolean LTR303::setPowerDown(void) {
	// Turn off LTR303
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)

	// Clear command byte (reset = 0 & mode = 0)(power off)
	return(writeByte(LTR303_CONTR,0x00));
}

boolean LTR303::setControl(byte gain, boolean reset = false, boolean mode = false) {
	// Sets the gain, SW reset and mode of LTR303
	// Default value is 0x00
	// If gain = 0, device is set to 1X gain (default)
	// If gain = 1, device is set to 2X gain
	// If gain = 2, device is set to 4X gain
	// If gain = 3, device is set to 8X gain
	// If gain = 4, invalid
	// If gain = 5, invalid
	// If gain = 6, device is set to 48X gain
	// If gain = 7, device is set to 96X gain
	//----------------------------------------
	// If reset = false(0), initial start-up procedure not started (default)
	// If reset = true(1), initial start-up procedure started
	//----------------------------------------
	// If mode = false(0), stand-by mode (default)
	// If mode = true(1), active mode
	
	byte control = 0x00;
	
	// sanity check for gain
	if (gain > 3 && gain < 6) {
		gain = 0x00;
	}
	else if(gain >= 7) {
		gain = 0x00;
	}
	
	// control byte logic
	control |= gain << 2;
	if(reset) {
		control |= 0x02;
	}
	
	if(mode) {
		control |= 0x01;
	}
	
	return(writeByte(LTR303_CONTR,control));
}			
			
boolean LTR303::getControl(byte &gain, boolean reset, boolean mode) {
	// Gets the control register values
	// Default value is 0x00
	// If gain = 0, device is set to 1X gain (default)
	// If gain = 1, device is set to 2X gain
	// If gain = 2, device is set to 4X gain
	// If gain = 3, device is set to 8X gain
	// If gain = 4, invalid
	// If gain = 5, invalid
	// If gain = 6, device is set to 48X gain
	// If gain = 7, device is set to 96X gain
	//----------------------------------------
	// If reset = false(0), initial start-up procedure not started (default)
	// If reset = true(1), initial start-up procedure started
	//----------------------------------------
	// If mode = false(0), stand-by mode (default)
	// If mode = true(1), active mode
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)			
	
	byte control;
	
	// Reading the control byte
	if(readByte(LTR303_CONTR, control)) {
		// Extract gain
		gain = (control & 0x1C) >> 2;
		
		// Extract reset
		reset = (control & 0x02) ? true : false; 
		
		// Extract mode
		mode = (control & 0x01) ? true : false;
		
		// return if successful
		return(true);
	}
	return(false);
}

boolean LTR303::setMeasurementRate(byte integrationTime, byte measurementRate = 3) {
	// Sets the integration time and measurement rate of the sensor
	// integrationTime is the measurement time for each ALs cycle
	// measurementRate is the interval between DATA_REGISTERS update
	// measurementRate must be set to be equal or greater than integrationTime
	// Default value is 0x03
	// If integrationTime = 0, integrationTime will be 100ms (default)
	// If integrationTime = 1, integrationTime will be 50ms
	// If integrationTime = 2, integrationTime will be 200ms
	// If integrationTime = 3, integrationTime will be 400ms
	// If integrationTime = 4, integrationTime will be 150ms
	// If integrationTime = 5, integrationTime will be 250ms
	// If integrationTime = 6, integrationTime will be 300ms
	// If integrationTime = 7, integrationTime will be 350ms
	//------------------------------------------------------
	// If measurementRate = 0, measurementRate will be 50ms
	// If measurementRate = 1, measurementRate will be 100ms
	// If measurementRate = 2, measurementRate will be 200ms
	// If measurementRate = 3, measurementRate will be 500ms (default)
	// If measurementRate = 4, measurementRate will be 1000ms
	// If measurementRate = 5, measurementRate will be 2000ms
	// If measurementRate = 6, measurementRate will be 2000ms
	// If measurementRate = 7, measurementRate will be 2000ms
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	byte measurement = 0x00;
	
	// Perform sanity checks
	if(integrationTime >= 0x07) {
		integrationTime = 0x00;
	}
	
	if(measurementRate >= 0x07) {
		measurementRate = 0x00;
	}
	
	measurement |= integrationTime << 3;
	measurement |= measurementRate;
	
	return(writeByte(LTR303_MEAS_RATE, measurement));
}

boolean LTR303::getMeasurementRate(byte &integrationTime, byte &measurementRate) {
	// Gets the value of Measurement Rate register
	// Default value is 0x03
	// If integrationTime = 0, integrationTime will be 100ms (default)
	// If integrationTime = 1, integrationTime will be 50ms
	// If integrationTime = 2, integrationTime will be 200ms
	// If integrationTime = 3, integrationTime will be 400ms
	// If integrationTime = 4, integrationTime will be 150ms
	// If integrationTime = 5, integrationTime will be 250ms
	// If integrationTime = 6, integrationTime will be 300ms
	// If integrationTime = 7, integrationTime will be 350ms
	//------------------------------------------------------
	// If measurementRate = 0, measurementRate will be 50ms
	// If measurementRate = 1, measurementRate will be 100ms
	// If measurementRate = 2, measurementRate will be 200ms
	// If measurementRate = 3, measurementRate will be 500ms (default)
	// If measurementRate = 4, measurementRate will be 1000ms
	// If measurementRate = 5, measurementRate will be 2000ms
	// If measurementRate = 6, measurementRate will be 2000ms
	// If measurementRate = 7, measurementRate will be 2000ms
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
			
	byte measurement = 0x00;
	
	// Reading the measurement byte
	if(readByte(LTR303_MEAS_RATE, measurement)) {
		// Extract integration Time
		integrationTime = (measurement & 0x38) >> 3;
		
		// Extract measurement Rate
		measurementRate = measurement & 0x07; 
		
		// return if successful
		return(true);
	}
	return(false);		
}

boolean LTR303::getPartID(byte &partID) {
	// Gets the part number ID and revision ID of the chip
	// Default value is 0x0A
	// part number ID = 0x0A (default)
	// Revision ID = 0x00
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
			
	return(readByte(LTR303_PART_ID, partID));
}

boolean LTR303::getManufacID(byte &manufacID) {
	// Gets the Manufacturers ID
	// Default value is 0x05
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	return(readByte(LTR303_MANUFAC_ID, manufacID));
}

boolean LTR303::getData(unsigned int &CH0, unsigned int &CH1) {
	// Gets the 16-bit channel 0 and channel 1 data
	// Default value of both channels is 0x00
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
//	return(readUInt(LTR303_DATA_CH0_0,CH0) && readUInt(LTR303_DATA_CH1_0,CH1));
/* 	2020-12-01 Andy testing getting LSB from 8A and MSB from 8B, so low and high bytes from channel 0.
	The return above this comment seems to be getting low bytes from two channels, and no high bytes.
	From https://github.com/watterott/LTR329ALS01-Breakout/blob/master/software/LTR329ALS01.ino it looks like I want 
	just one channel, low and high bytes.
	#define LTR303_DATA_CH0_0    0x8A
	#define LTR303_DATA_CH0_1    0x8B
	Maybe not... those readUInt functions should be getting two bytes.
	Not sure on these data types in the Teensy environment?
*/
	return(readUInt(LTR303_DATA_CH0_0,CH0) && readUInt(LTR303_DATA_CH0_1,CH1));


}

boolean LTR303::getStatus(boolean valid, byte &gain, boolean intrStatus, boolean dataStatus) {
	// Gets the status information of LTR303
	// Default value is 0x00
	// If valid = false(0), Sensor data is valid (default)
	// If valid = true(1), Sensor data is invalid
	//--------------------------------------------
	// If gain = 0, device is set to 1X gain (default)
	// If gain = 1, device is set to 2X gain
	// If gain = 2, device is set to 4X gain
	// If gain = 3, device is set to 8X gain
	// If gain = 4, invalid
	// If gain = 5, invalid
	// If gain = 6, device is set to 48X gain
	// If gain = 7, device is set to 96X gain
	//---------------------------------------------
	// If intrStatus = false(0), INTR in inactive (default)
	// If intrStatus = true(1), INTR in active
	//---------------------------------------------
	// If dataStatus = false(0), OLD data (already read) (default)
	// If dataStatus = true(1), NEW data
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	byte status = 0x00;
	
	// Reading the status byte
	if(readByte(LTR303_STATUS, status)) {
		// Extract validity
		valid = (status & 0x80) ? true : false;
	
		// Extract gain
		gain = (status & 0x70) >> 4;
	
		// Extract interrupt status
		intrStatus = (status & 0x08) ? true : false;
	
		// Extract data status
		dataStatus = (status & 0x04) ? true : false;
		
		// return if successful
		return(true);
	}
	return(false);
}

boolean LTR303::setInterruptControl(boolean intrMode, boolean polarity = false) {
	// Sets up interrupt operations
	// Default value is 0x08
	// If intrMode = false(0), INT pin is inactive (default)
	// If intrMode = true(1), INT pin is active
	//------------------------------------------------------
	// If polarity = false(0), INT pin is active at logic 0 (default)
	// If polarity = true(1), INT pin is active at logic 1
	//------------------------------------------------------
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	byte intrControl = 0x00;
	
	intrControl |= polarity << 2;
	intrControl |= intrMode << 1;
	
	return(writeByte(LTR303_INTERRUPT, intrControl));
}

boolean LTR303::getInterruptControl(boolean polarity, boolean intrMode) {
	// Sets up interrupt operations
	// Default value is 0x08
	// If polarity = false(0), INT pin is active at logic 0 (default)
	// If polarity = true(1), INT pin is active at logic 1
	//------------------------------------------------------
	// If intrMode = false(0), INT pin is inactive (default)
	// If intrMode = true(1), INT pin is active
	//------------------------------------------------------
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	byte intrControl = 0x00;
	
	// Reading the interrupt byte
	if(readByte(LTR303_INTERRUPT, intrControl)) {
		// Extract polarity
		polarity = (intrControl & 0x04) ? true : false;
	
		// Extract mode
		intrMode = (intrControl & 0x02) ? true : false;
	
		// return if successful
		return(true);
	}
	return(false);
}

boolean LTR303::setThreshold(unsigned int upperLimit, unsigned int lowerLimit) {
	// Sets the upper limit and lower limit of the threshold
	// Default value of upper threshold is 0xFF and lower threshold is 0x00
	// Both the threshold are 16-bit integer values
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	return(writeUInt(LTR303_THRES_UP_0,upperLimit) && writeUInt(LTR303_THRES_LOW_0,lowerLimit));
}

boolean LTR303::getThreshold(unsigned int &upperLimit, unsigned int &lowerLimit) {
	// Gets the upper limit and lower limit of the threshold
	// Default value of upper threshold is 0xFF and lower threshold is 0x00
	// Both the threshold are 16-bit integer values
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
			
	return(readUInt(LTR303_THRES_UP_0,upperLimit) && readUInt(LTR303_THRES_LOW_0,lowerLimit));		
}

boolean LTR303::setIntrPersist(byte persist) {
	// Sets the interrupt persistance i.e. controls the N number of times the 
	// measurement data is outside the range defined by upper and lower threshold
	// Default value is 0x00
	// If persist = 0, every sensor value out of threshold range (default)
	// If persist = 1, every 2 consecutive value out of threshold range
	// If persist = 2, every 3 consecutive value out of threshold range
	// If persist = 3, every 4 consecutive value out of threshold range
	// If persist = 4, every 5 consecutive value out of threshold range
	// If persist = 5, every 6 consecutive value out of threshold range
	// If persist = 6, every 7 consecutive value out of threshold range
	// If persist = 7, every 8 consecutive value out of threshold range
	// If persist = 8, every 9 consecutive value out of threshold range
	// If persist = 9, every 10 consecutive value out of threshold range
	// If persist = 10, every 11 consecutive value out of threshold range
	// If persist = 11, every 12 consecutive value out of threshold range
	// If persist = 12, every 13 consecutive value out of threshold range
	// If persist = 13, every 14 consecutive value out of threshold range
	// If persist = 14, every 15 consecutive value out of threshold range
	// If persist = 15, every 16 consecutive value out of threshold range
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	// sanity check
	if(persist >= 15) {
		persist = 0x00;
	}
			
	return(writeByte(LTR303_INTR_PERS,persist));
}

boolean LTR303::getIntrPersist(byte &persist) {
	// Gets the interrupt persistance i.e. controls the N number of times the measurement data is outside the range defined by upper and lower threshold
	// Default value is 0x00
	// If persist = 0, every sensor value out of threshold range (default)
	// If persist = 1, every 2 consecutive value out of threshold range
	// If persist = 2, every 3 consecutive value out of threshold range
	// If persist = 3, every 4 consecutive value out of threshold range
	// If persist = 4, every 5 consecutive value out of threshold range
	// If persist = 5, every 6 consecutive value out of threshold range
	// If persist = 6, every 7 consecutive value out of threshold range
	// If persist = 7, every 8 consecutive value out of threshold range
	// If persist = 8, every 9 consecutive value out of threshold range
	// If persist = 9, every 10 consecutive value out of threshold range
	// If persist = 10, every 11 consecutive value out of threshold range
	// If persist = 11, every 12 consecutive value out of threshold range
	// If persist = 12, every 13 consecutive value out of threshold range
	// If persist = 13, every 14 consecutive value out of threshold range
	// If persist = 14, every 15 consecutive value out of threshold range
	// If persist = 15, every 16 consecutive value out of threshold range
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
	
	return(readByte(LTR303_INTR_PERS,persist));
}

// Get the right lux algorithm
boolean LTR303::getLux(byte gain, unsigned char IntTime, unsigned int CH0, unsigned int CH1, double &lux) {
	// Convert raw data to lux
	// gain: 0 (1X) or 7 (96X), see getControl()
	// integrationTime: integration time in ms, from getMeasurementRate()
	// CH0, CH1: results from getData()
	// lux will be set to resulting lux calculation
	// returns true (1) if calculation was successful
	// returns false (0) AND lux = 0.0 IF EITHER SENSOR WAS SATURATED (0XFFFF)

	double ratio, ALS_INT;
	int ALS_GAIN;

	// Determine if either sensor saturated (0xFFFF)
	// If so, abandon ship (calculation will not be accurate)
	if ((CH0 == 0xFFFF) || (CH1 == 0xFFFF)) {
		lux = 0.0;
		return(false);
	}
	
	// There is an odd glitch that causes ALS LTR329 CH1 to be 1024 intermittently and that throws of the calculation.
	if (CH1 == 1024) {
		CH1 = 0;
	}


	// We will need the ratio for subsequent calculations
	ratio = CH1 / (CH0 + CH1);

	// Gain can take any value from 0-7, except 4 & 5
	// If gain = 4, invalid
	// If gain = 5, invalid
	switch (gain) {
	case 0:			   // If gain = 0, device is set to 1X gain (default)
		ALS_GAIN = 1;
		break;
	case 1:			   // If gain = 1, device is set to 2X gain
		ALS_GAIN = 2;
		break;
	case 2:			  // If gain = 2, device is set to 4X gain	 
		ALS_GAIN = 4;
		break;
	case 3:			  // If gain = 3, device is set to 8X gain	  
		ALS_GAIN = 8;
		break;
	case 6:		     // If gain = 6, device is set to 48X gain
		ALS_GAIN = 48;
		break;
	case 7:			  // If gain = 7, device is set to 96X gain  
		ALS_GAIN = 96;
		break;
	default:		  // If gain = 0, device is set to 1X gain (default)	 	 
		ALS_GAIN = 1;
		break;
	}

	switch (IntTime) {
	case 0:				 // If integrationTime = 0, integrationTime will be 100ms (default)
		ALS_INT = 1;
		break;
	case 1:			 	 // If integrationTime = 1, integrationTime will be 50ms
		ALS_INT = 0.5;
		break;
	case 2:				 // If integrationTime = 2, integrationTime will be 200ms
		ALS_INT = 2;
		break;
	case 3:				  // If integrationTime = 3, integrationTime will be 400ms
		ALS_INT = 4;
		break;
	case 4:				  // If integrationTime = 4, integrationTime will be 150ms
		ALS_INT = 1.5;
		break;
	case 5:				  // If integrationTime = 5, integrationTime will be 250ms
		ALS_INT = 2.5;
		break;
	case 6:				  // If integrationTime = 6, integrationTime will be 300ms
		ALS_INT = 3;
		break;
	case 7:				  // If integrationTime = 7, integrationTime will be 350ms
		ALS_INT = 3.5;
		break;
	default:		 	 // If integrationTime = 0, integrationTime will be 100ms (default)
		ALS_INT = 1;
		break;
	}

	// Determine lux per datasheet equations:
	if (ratio < 0.45) {
		lux = ((1.7743 * CH0) + (1.1059 * CH1)) / ALS_GAIN / ALS_INT;
		return(true);
	}

	if ((ratio < 0.64) && (ratio >= 0.45)) {
		lux = ((4.2785 * CH0) + (1.9548 * CH1)) / ALS_GAIN / ALS_INT;
		return(true);
	}

	if ((ratio < 0.85) && (ratio >= 0.64)) {
		lux = ((0.5926 * CH0) + (0.1185 * CH1)) / ALS_GAIN / ALS_INT;
		return(true);
	}

	// if (ratio >= 0.85)
	else {
		lux = 0.0;
		return(true);
	}	
}

byte LTR303::getError(void) {
	// If any library command fails, you can retrieve an extended
	// error code using this command. Errors are from the wire library: 
	// 0 = Success
	// 1 = Data too long to fit in transmit buffer
	// 2 = Received NACK on transmit of address
	// 3 = Received NACK on transmit of data
	// 4 = Other error

	return(_error);
}

// Private functions:

boolean LTR303::readByte(byte address, byte &value) {
	// Reads a byte from a LTR303 address
	// Address: LTR303 address (0 to 15)
	// Value will be set to stored byte
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)

	// Check if sensor present for read
	Wire.beginTransmission(_i2c_address);
	Wire.write(address);
	_error = Wire.endTransmission();

	// Read requested byte
	if (_error == 0)
	{
		Wire.requestFrom(_i2c_address,1);
		if (Wire.available() == 1)
		{
			value = Wire.read();
			return(true);
		}
	}
	return(false);
}

boolean LTR303::writeByte(byte address, byte value) {
	// Write a byte to a LTR303 address
	// Address: LTR303 address (0 to 15)
	// Value: byte to write to address
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)

	Wire.beginTransmission(_i2c_address);
	Wire.write(address);
	// Write byte
	Wire.write(value);
	_error = Wire.endTransmission();
	if (_error == 0)
		return(true);

	return(false);
}

boolean LTR303::readUInt(byte address, unsigned int &value) {
	// Reads an unsigned integer (16 bits) from a LTR303 address (low byte first)
	// Address: LTR303 address (0 to 15), low byte first
	// Value will be set to stored unsigned integer
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)

	byte high, low;
	
	// Check if sensor present for read
	Wire.beginTransmission(_i2c_address);
	Wire.write(address);
	_error = Wire.endTransmission();

	// Read two bytes (low and high)
	if (_error == 0)
	{
		Wire.requestFrom(_i2c_address,2);
		if (Wire.available() == 2)
		{
			low = Wire.read();
			high = Wire.read();
			// Combine bytes into unsigned int
			value = word(high,low);
			return(true);
		}
	}	
	return(false);
}

boolean LTR303::writeUInt(byte address, unsigned int value) {
	// Write an unsigned integer (16 bits) to a LTR303 address (low byte first)
	// Address: LTR303 address (0 to 15), low byte first
	// Value: unsigned int to write to address
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)

	// Split int into lower and upper bytes, write each byte
	if (writeByte(address,lowByte(value)) 
		&& writeByte(address + 1,highByte(value)))
		return(true);

	return(false);
}