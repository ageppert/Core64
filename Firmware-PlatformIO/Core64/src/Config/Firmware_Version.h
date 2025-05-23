/*
	PURPOSE: Define and describe the firmware version scheme and number.
         		Specify firmware version manually in this file as a string.
	SETUP: Manually update the field assigned as VERSION	
*/

#ifndef FIRMWARE_VERSION_H
	#define FIRMWARE_VERSION_H
	#include <stdint.h>
	#include <stdbool.h>

	const char compile_date[] = __DATE__ " at " __TIME__;	// Automatically updated date and time this firmware was compiled.
	// TODO: Drop the need to manually enter the following line.
	#define FIRMWAREVERSION "    " // was "211225.1055"
	// TODO: Expand the following text/character value to be an automatically concatenated and printable string like this "0.4.0-210530.1340"
	const char FirmwareVersion[] = FIRMWAREVERSION;
	// TODO: Using something like this and then get ride of the #define FIRMWAREVERSION above.
	// const String FirmwareVersion[] = String ("V" + String(FirmwareVersionMajor) + "." + String(FirmwareVersionMinor) + "." + String(FirmwareVersionPatch) + "-" + __DATE__ + " " + __TIME__);

	/*	FIRMWARE VERSION SCHEME 
		Same as hardware version scheme, with additional metadata extensions:
			-DATE 		Ex. -200101			Two digit month, two digit day of the month
				.TIME 	  Ex. .0800 			Time increments from build to build to clearly denote each unqiue iteration
			Firmware version is manually entered below.
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
	|  0.6.0  | 2021-12-23 | Enable Core64 V0.6.0 like V0.5.0 for bring-up. Maintain backwards compatibility.
	|  0.6.1  | 2021-12-25 | Print serial number zero-padded to 6 digits. Remove manual date/time from firmware version suffix.
	|  0.6.2  | 2021-12-25 | Backout zero-padded S/N in Core64c with RP2040.
	|  0.6.3  | 2021-12-25 | Display firmware version major.minor.patch to OLED, replacing manual text string.
	|  0.7.0  | 2022-01-02 | Move from Arduino to PlatformIO, split files into sub-directories, broke Core64c (I2C1) compatibility.
	|  0.7.1  | 2022-01-22 | Fixing I2C issues so Core64 and Core64c both work.
	|  0.7.2  | 2022-01-30 | Move Commandline Handler and Mode Manager out to their own files, outside of main.c
	|  0.7.3  | 2022-02-04 | Move Serial setup outside of main.c, add debug level to commandline.
	|  0.7.4  | 2022-02-12 | Organize TopLevelMode list, placeholder GAUSS menu.
	|  0.7.5  | 2022-02-12 | Implement DGAUSS MENU, draw mode UI improved, organized sub-menu category modes.
	|  0.7.6  | 2022-03-05 | Add array of string descriptions for all modes.
	|  0.7.7  | 2022-03-13 | Move modes into discrete files in sub-folders by sub-menu. Update start-up symbol sequence.
	|  0.7.8  | 2022-03-18 | Add Game of Snake!
	|  0.7.9  | 2022-03-28 | Refine start-up mode sequence, I2C, hall sensor button set-up.
	|  0.7.10 | 2022-04-05 | WIP Update I2C interactions to enable Core64c to scroll text and update OLED display.
	|  0.7.11 | 2022-04-06 | Update Pico code so I2C1 port works with OLED, Hall Sensors, EEPROM.
	|  0.7.12 | 2022-04-11 | Update Pico code so all multicolor LED Matrix functions work.
	|  0.7.13 | 2022-04-11 | Core64c, Pico, implement core driver!
	|  0.7.14 | 2022-04-28 | Add thanks, automatic looping of demo modes, move snake game symbols to their own array, revise menu time-outs.
	|  0.7.15 | 2022-04-30 | Core64c, Pico, partial fix for rainbow demo, faster core read/write bit, all analog enabled/tested.
	|  0.7.16 | 2022-05-03 | Core64c, Pico, SPI and SD Card tested, all spare GPIO test mode, hall switch test, use built-in Pico VSYS/3 to read 5V0 and GPIO24 for VBUS sense.
	|  0.7.17 | 2022-05-26 | Core64c LB HWV0.3.0 recognize and bring-up.
	|  0.7.18 | 2022-06-11 | Core64c LB GPIO Test Mode configures all spare GPIO as digital outputs for testing.
	|  0.7.19 | 2022-07-11 | Core64 LB disable SD Card by default so heartbeat runs, expand hardware version checks to major and minor numbers.
	|  0.7.20 | 2022-09-04 | Add Core64c, update LED heartbeat and Analog Input VSYS for compatibility with optional Pico W. Add placeholder MODE_MANUFACTURING_EEPROM_FACTORY_WRITE.
	|  0.7.21 | 2022-09-17 | Add basic game of Pong, protect for LED Matrix Array overload
	|  0.7.22 | 2022-10-10 | Refine simplistic game of Pong. Works with Neon Pixels too!
	|  0.7.23 | 2022-12-27 | Core64, add core # to MODE_CORE_TOGGLE_BITS_WITH_3V3_READ analog voltage data stream, add read ANA7 (A12) connected above CAE FET with modified LB. 
	|  0.7.24 | 2023-02-19 | Core64, add compatibility for Core64 V0.7+ Pico Version, remove unused Detect_Board file.
	|  0.7.25 | 2023-07-07 | Add glamor shot display mode in first special menu, set to pong game
	|  0.7.26 | 2023-07-24 | Enhance App_Drawing to become App_Paint
	|  0.7.27 | 2023-09-04 | Neon Pixels, add #define to test flux only on bit 0 with Neon Pixels for demo with oscilloscope.
	|  0.7.28 | 2023-09-04 | Corrected mistakes when I merged to master. Should now behave like 0.7.27.
	|  0.8.0  | 2023-09-04 | Core16 support introduced.
	|  0.8.1  | 2023-09-12 | Core16 LED and Core Matrix working in most modes.
	|  0.8.2  | 2023-09-15 | Core16 DGAUSS Menu working.
	|  0.8.3  | 2023-09-16 | Core16 Tested all hardware functions. Add debug pin 1 and 2 for Core16 HWV0.1.0 and HWV0.2.0. Move LED Array Init after EEPROM Board ID read.
	|  0.8.4  | 2023-10-04 | Core16 WIP test fixture mods, can program S/Ns 300006-300105
	|  0.8.5  | 2023-10-14 | Add commands to set start and end cores to test.
	|  0.8.6  | 2023-10-21 | Core16 adapt paint app to 4x4. Changed auto-run test >=300056
	|  0.8.7  | 2023-11-01 | Core16 adapt snake game to 4x4. 0.8.6 adapted paint app, not snake game.
	|  0.8.8  | 2023-11-01 | Core16 adapt pong game to 4x4. Disable factory programming.
	|  0.8.9  | 2023-11-01 | All Core Boards, display flux detection menu symbol, disable all hall sensors in flux detector mode. Update to work with Teensy-based Core64s.
	|  0.8.10 | 2023-12-08 | All ICM (Interactive Core Memory) Kits, display voltage on OLED in all modes, update factory setting mode, extend OutputToSerialShiftRegister, enable and test Pico Spare SR EFGH.
	|  0.8.11 | 2024-04-11 | All ICM (Interactive Core Memory) Kits, add option to drive SPI Color OLED 1.5 inch from https://www.adafruit.com/product/1431. Stay in scrolling demo if Neon Pixels enabled. Core64 Teensy SPI Color OLED and SD Card not fully implemented.
	|  0.8.12 | 2024-08-24 | Core64 and Core64c default brightness reduced with the Led Matrix V1.3 being too bright (200->125). Update to latest versions of platform, SSD1306, BusIO and manually include pgmspace.h in SSD1306 for MBP M1 fresh install compatibility. Update factory reset config details for fresh batches of Core64 and Core64c.
	|  0.8.13 | 2024-08-25 | Update Core64c factory config values.
	|  0.8.14 | 2025-01-13 | Update factory config values Core64 HWV0.8.4.
	|  0.8.15 | 2025-04-22 | Change to FastLED library to remove LED glitches. Enable optional MIDI SAO support.
	|         |            | 
	----------------------------------------------------------------------------------------------------------*/
	#define FIRMWARE_DESCRIPTION "Change to FastLED library to remove LED glitches. Enable optional MIDI SAO support."
	const uint8_t FirmwareVersionMajor = 0 ;				// Update manually.
	const uint8_t FirmwareVersionMinor = 8 ;				// Update manually.
	const uint8_t FirmwareVersionPatch = 15;				// Update manually.

#endif // FIRMWARE_VERSION_H
