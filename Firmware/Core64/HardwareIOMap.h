/*
	PURPOSE: Define/assign/log the version scheme and number.
				Map out all of the IO for each version of the Core 64 hardware.
         		The Hardware_IO_Map.cpp file detects and sets the hardware version.
         		Specify firmware version manually in this file as a string.
	SETUP: Manually update the field assigned as VERSION	
*/

#ifndef HARDWARE_IO_MAP_H
	#define HARDWARE_IO_MAP_H

	#include <stdint.h>
	#include <stdbool.h>

	// Board Detection.
	// 	Reference: https://arduino.stackexchange.com/questions/21137/arduino-how-to-get-the-board-type-in-code
		  #if defined(TEENSYDUINO) 
		      //  --------------- Teensy -----------------
		      #if defined(__MK20DX256__)       
		          #define BOARD "Teensy 3.2 (MK20DX256)" // and Teensy 3.1 (obsolete)
		          #define BOARD_CORE64_TEENSY_32
		      #endif
		  #else // --------------- RP2040 ------------------
		      #if defined(ARDUINO_ARCH_RP2040)       
		          #define BOARD "Raspberry Pi Pico (RP2040)"
		          #define BOARD_CORE64C_RASPI_PICO
		      #endif
		  #endif
	// Display board name which is being compiled for:
		  #pragma message ( "C Preprocessor identified board type:" )
		  #ifdef BOARD
		    #pragma message ( BOARD )
		  #else
		    #error ( "Unsupported board. Choose Teensy 3.1/3.2 or Raspberry Pi Pico in 'Tools > Boards' menu.")
		  #endif
	// End of Board Detection.

	#ifdef __cplusplus
	extern "C" {
	#endif

  // SELECT HARDWARE TO ACTIVATE
	// TODO: Move all of these to variables which can be configured on-the-fly.
			#define HALL_SENSOR_ENABLE							// 
			#define HALL_SWITCH_ENABLE
			#define DIAGNOSTIC_VOLTAGE_MONITOR_ENABLE
			#define NEON_PIXEL_ARRAY							// Serpentine, like Pimoroni Unicorn Hat
			#define CORE64_LED_MATRIX							// Row Major, Progressive layout. Just like an array in C.
	    #if defined BOARD_CORE64_TEENSY_32 
	      #define USE_FASTLED_LIBRARY							// If this is set, use FastLED library (compatible with Teensy)
			  // #define USE_ADAFRUIT_NEOPIXEL_LIBRARY				// If this is set, use Adafruit library (because FastLED is not yet compatible with RasPi Pico)
		  	// #define SCROLLING_TEXT_BYPASS_CORE_MEMORY			// This will scroll text directly to LEDs and bypass (ignore) core memory status.		
	    #elif defined BOARD_CORE64C_RASPI_PICO
		  	#define USE_ADAFRUIT_NEOPIXEL_LIBRARY				// If this is set, use Adafruit library (because FastLED is not yet compatible with RasPi Pico)
		  	#define SCROLLING_TEXT_BYPASS_CORE_MEMORY			// This will scroll text directly to LEDs and bypass (ignore) core memory status.
			#endif
			// #define AMBIENT_LIGHT_SENSOR_LTR329_ENABLE   		// 
			// #define SDCARD_ENABLE                  			// 
			#define OLED_64X128
			// #define OLED_128X128

		/*  HARDWARE VERSION SCHEME (see https://semver.org/)
				Given a version number MAJOR.MINOR.PATCH, increment the
					MAJOR version when you make incompatible API changes,
					MINOR version when you add functionality in a backwards compatible manner, and
					PATCH version when you make backwards compatible bug fixes.
				Hardware version is stored in Board ID EEPROM on the Logic Board.
		*/
	  /********************************** CORE64 HARDWARE VERSION TABLE ******************************************
		| VERSION |  DATE      | DESCRIPTION                                                                       |
		------------------------------------------------------------------------------------------------------------
		|  0.1.0  | 2019-08-09 | Single board prototype development
		|  0.1.5  | 2020-02-22 | Single board, reworked board fixes, prototype development
		|  0.2.0  | 2020-03-16 | Single board, design updated to match v0.1.5, theoretically (not produced)
		|  0.3.0  | 2020-05-30 | Dual board, hardware version detection for 0.2.x (includes v0.1.x) and v0.3.x
		|  0.3.1  | 2020-05-30 | Two voltage regulators, 5V0 and 3V3, powered from common switch point
		|  0.4.0  | 2020-11-28 | Blue LB, Yellow CB with Plane 4 set, as-built bring-up
		|  0.5.0  | 2021-03-20 | Triple-board, Black LB/CB/CM
		|         |            | 
		----------------------------------------------------------------------------------------------------------*/
	  /********************************** CORE64c HARDWARE VERSION TABLE *****************************************
		| VERSION |  DATE      | DESCRIPTION                                                                       |
		------------------------------------------------------------------------------------------------------------
		|  0.1.0  | 2021-05-28 | First layout, abandonded, not prototyped
		|  0.2.0  | 2021-06-04 | White Board, prototyped
		|         |            | 
		----------------------------------------------------------------------------------------------------------*/
		void DetectHardwareVersion ();		// Use once to detect and set the hardware version variables.
		extern uint8_t HardwareVersionMajor  ;
		extern uint8_t HardwareVersionMinor  ;
		extern uint8_t HardwareVersionPatch  ;

		/*	FIRMWARE VERSION SCHEME 
		    Same as hardware version scheme, with additional metadata extensions:
			    -DATE 		Ex. -200101			Two digit month, two digit day of the month
					.TIME 	  Ex. .0800 			Time increments from build to build to clearly denote each unqiue iteration
				Firmware version is manually entered below.
		*/
		/***************************************** FIRMWARE VERSION TABLE ******************************************
		| VERSION |  DATE      | DESCRIPTION                                                                       |
		------------------------------------------------------------------------------------------------------------
		|  0.1.0  | 2019-      | 
		|  0.1.5  | 2020-      | 
		|  0.2.0  | 2020-      | 
		|  0.3.x  | 2020-      | 
		|  0.4.x  | 2020-11-28 | 
		|  0.5.x  | 2021-03-20 | Accept V0.5.x hardware, manual default to the custom-fit LED Matrix.
		|  0.5.x  | 2021-04-25 | Beta Kit Release
		|  0.5.1  | 2021-09-06 | Display firmware version info, backwards compatible with Core64, adding basic Core64c functionality.
		|  0.5.2  | 2021-09-06 | Compile time select FastLED or Neopixel library, scrolling text [only] on Core64c.
		|  0.5.3  | 2021-09-09 | Add and enable scrolling text color change.
		|  0.5.4  | 2021-09-29 | Enable all four hall sensors and/or switches, and OLED, for Core64 V0.5.0 and Core64c V0.2.0
		|  0.5.5  | 2021-10-01 | Display Voltage Input (USB or Bat.) for Core64 V0.5.0 and Core64c V0.2.0.
		|         |            | 
		----------------------------------------------------------------------------------------------------------*/
		const uint8_t FirmwareVersionMajor = 0 ;
		const uint8_t FirmwareVersionMinor = 5 ;
		const uint8_t FirmwareVersionPatch = 5 ;
		const char compile_date[] = __DATE__ " at " __TIME__;	// The date and time this firmware was compiled.
		#define FIRMWARE_SUMMARY "Display Voltage Input (USB or Bat.) for Core64 V0.5.0 and Core64c V0.2.0"
		// TODO: Drop the need to manually enter the following line.
		#define FIRMWAREVERSION "211001.0801"
		// TODO: Expand the following to be an automatically concatenated printable string like this "0.4.0-210530.1340"
		const char FirmwareVersion[] = FIRMWAREVERSION;
		// TODO: Using something like this and then get ride of the #define FIRMWAREVERSION above.
		// const String FirmwareVersion[] = String ("V" + String(FirmwareVersionMajor) + "." + String(FirmwareVersionMinor) + "." + String(FirmwareVersionPatch) + "-" + __DATE__ + " " + __TIME__);

	#if defined BOARD_CORE64_TEENSY_32
		// Core64 HARDWARE v0.5.0
			// PRIMARY AND DEFAULT FUNCTIONALITY
				// HEART BEAT - HELLO WORLD
						#define Pin_Built_In_LED       		13  // * Shared with SPI CLOCK
				// I2C (HARDWARE ID EEPROM, HALL SENSORS, AMBIENT LIGHT SENSOR, OLED, SAO)
						#define Pin_I2C_Bus_Data         	18  // Default Teensy. #define not needed, Wire.h library takes care of this pin configuration.
						#define Pin_I2C_Bus_Clock        	19  // Default Teensy. #define not needed, Wire.h library takes care of this pin configuration.
				// LED ARRAY
						#define Pin_RGB_LED_Array         20	// * Shared 
			  // HALL SWITCHES, AUTOMATICALLY USED IF I2C HALL SENSORS ARE NOT DETECTED
				    #ifdef HALL_SWITCH_ENABLE
							#define PIN_HALL_SWITCH_1				 0
							#define PIN_HALL_SWITCH_2				 1
							#define PIN_HALL_SWITCH_3				14
							#define PIN_HALL_SWITCH_4				10		
				    #endif
				// MATRIX SENSE
						#define Pin_Sense_Reset         	21	// * Shared
						#define Pin_Sense_Pulse         	22	// * Shared
				// MATRIX DRIVE
						#define PIN_WRITE_ENABLE         	23	// * Shared 
						#define PIN_MATRIX_DRIVE_Q1P     	 2	// * Shared
						#define PIN_MATRIX_DRIVE_Q1N     	 3
						#define PIN_MATRIX_DRIVE_Q2P     	 4	// * Shared
						#define PIN_MATRIX_DRIVE_Q2N     	 5	// * Shared
						#define PIN_MATRIX_DRIVE_Q3P     	 6	// * Shared
						#define PIN_MATRIX_DRIVE_Q3N     	 7
						#define PIN_MATRIX_DRIVE_Q4P     	 8	// * Shared
						#define PIN_MATRIX_DRIVE_Q4N     	 9	// * Shared
						#define PIN_MATRIX_DRIVE_Q5P     	16
						#define PIN_MATRIX_DRIVE_Q5N     	17
						#define PIN_MATRIX_DRIVE_Q6P     	24
						#define PIN_MATRIX_DRIVE_Q6N     	25
						#define PIN_MATRIX_DRIVE_Q7P     	26
						#define PIN_MATRIX_DRIVE_Q7N     	27   
						#define PIN_MATRIX_DRIVE_Q8P     	28   
						#define PIN_MATRIX_DRIVE_Q8N     	29   
						#define PIN_MATRIX_DRIVE_Q9P     	30
						#define PIN_MATRIX_DRIVE_Q9N     	31 
						#define PIN_MATRIX_DRIVE_Q10P    	32 
						#define PIN_MATRIX_DRIVE_Q10N    	33	
			  // DIAGNOSTIC VOLTAGE MONITORING, DEFAULT LOGIC BOARD CONFIGURATION, AS MANUFACTURED.
				    #ifdef DIAGNOSTIC_VOLTAGE_MONITOR_ENABLE
							#define Pin_SPARE_3_Assigned_To_Spare_3_Analog_Input A0	// * Shared with Digital Pin 14
							#define Pin_Battery_Voltage    A10  // 1/2 the battery voltage (otherwise known as Digital pin 24)
							#define Pin_SPARE_ANA_6			   A11
							#define Pin_SPARE_ANA_7			   A12
							#define Pin_SPARE_ANA_8			   A13
					    #define Pin_Spare_ADC_DAC		   A14
						#endif
			  // DEBUG PINS, DEFAULT LOGIC BOARD CONFIGURATION, AS MANUFACTURED.

			// OPTIONAL FEATURES, REQUIRES CAREFUL INTEGRATION AROUND PRIMARY PIN USAGE LISTED ABOVE
				// SPI
				    #define Pin_SPI_OLED_CS				 			2	// * Shared, digital output
				    #define Pin_SPI_TOUCH_CS			 			4	// * Shared, digital output
						#define Pin_SPI_SD_CS     		 			6	// * Shared, digital output
				    #define Pin_SPI_LCD_CS				 			8	// * Shared, digital output
				    #define Pin_SPI_LCD_DC				 			9	// * Shared, digital output
				    // #define Pin_SPI_TeensyView_CS			10	// * Shared, digital output
				    #define Pin_SPI_SDO									11  // Default Teensy
				    #define Pin_SPI_SDI									12  // Default Teensy
						// #define Pin_SPI_SD_CD     		  		14	// * Shared, digital input and output
				    #define Pin_SPI_CLK           			13	// * Shared, digital output
						#define Pin_SPI_Reset_Spare_5				15	// * Shared, digital output, multipurpose use, choose one #define below to uncomment and activate
						// #define Pin_SPARE_5_Assigned_To_Spare_5_Output
						// #define Pin_SPARE_5_Assigned_To_Spare_5_Analog
				    #define Pin_SPI_LCD_BACKLIGHT				20	// * Shared, digital output
						#define Pin_SPI_TeensyView_DC				21	// * Shared, digital output
						#define Pin_SPI_OLED_DC							23	// * Shared, digital output
				// IR COMMUNICATION
						#define Pin_IR_OUT					 				 5	// * Shared, digital output, multipurpose use, choose one #define below to uncomment and activate
			    	// #define Pin_Spare_4_IR_IN					10	// * Shared
						// #define Pin_Spare_4_IR_IN_Assigned_To_Spare_4_Output
				// CORE PLANE SELECT, REQUIRES ADDITIONAL COMPONENTS AND MODIFICATIONS
						// #define Pin_SAO_G1_SPARE_1_CP_ADDR_0 0	// * Shared, multipurpose use, choose one #define below to uncomment and activate
						// #define Pin_SAO_G1_SPARE_1_CP_ADDR_0_Assigned_To_CP_ADDR_0_Output
						// #define Pin_SAO_G2_SPARE_2_CP_ADDR_1 1	// * Shared, multipurpose use, choose one #define below to uncomment and activate
						// #define Pin_SAO_G2_SPARE_2_CP_ADDR_1_Assigned_To_CP_ADDR_1_Output
						// #define Pin_SPARE_3_CP_ADDR_2		14	// * Shared, multipurpose use, choose one #define below to uncomment and activate
						// #define Pin_SPARE_3_Assigned_To_Spare_3_Output
						// #define Pin_SPARE_3_CP_ADDR_2_Assigned_To_CP_ADDR_2_Output
						// #define Pin_SPARE_3_Assigned_To_Spare_3_Input
						// #define Pin_SPARE_3_Assigned_To_SPI_SD_CD_Input

	#elif defined BOARD_CORE64C_RASPI_PICO
		// Core64c HARDWARE v0.2.0
			// PRIMARY AND DEFAULT FUNCTIONALITY
				// HEART BEAT - HELLO WORLD
						#define Pin_Built_In_LED         	25  // * digital output
				// I2C (HARDWARE ID EEPROM, HALL SENSORS, AMBIENT LIGHT SENSOR, OLED, SAO)
						#define Pin_I2C_Bus_Data         	14  // RasPi Pico I2C1_SDA GP10 is Pico pin 14. #define not needed, see notes in I2C_Manager.cpp
						#define Pin_I2C_Bus_Clock        	15  // Raspi Pico I2C1_SCL GP11 is Pico pin 15. #define not needed, see notes in I2C_Manager.cpp
				// LED ARRAY
						#define Pin_RGB_LED_Array         22	// * Shared 
			  // HALL SWITCHES, AUTOMATICALLY USED IF I2C HALL SENSORS ARE NOT DETECTED
				    #ifdef HALL_SWITCH_ENABLE
							#define PIN_HALL_SWITCH_1				 5
							#define PIN_HALL_SWITCH_2				 6
							#define PIN_HALL_SWITCH_3				 7
							#define PIN_HALL_SWITCH_4				 8		
				    #endif
			  // DIAGNOSTIC VOLTAGE MONITORING, DEFAULT LOGIC BOARD CONFIGURATION, AS MANUFACTURED.
						#define Pin_Battery_Voltage     	A0  // 1/2 the battery voltage (otherwise known as Digital pin 26)

	#endif

	#ifdef __cplusplus
	} // extern "C"
	#endif

#endif // HARDWARE_IO_MAP_H
