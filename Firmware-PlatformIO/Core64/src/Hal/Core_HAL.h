/*
PURPOSE: Get and set core values from the logical array as bits, bytes or long integers
SETUP:
PURPOSE: Test the digital IO pins that are planned for addressing the core matrix with non-blocking code
SETUP:
- Configure in Digital_IO_Test.c
*/
 
#pragma once

#ifndef CORE_API_H
#define CORE_API_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <stdint.h>
#include <stdbool.h>

extern bool CoreArrayMemory [8][8];

// NEW API Command List
// TODO: Add core plane select
void Core_Mem_Bit_Write(uint8_t bit, bool value); 				// bit 0-63, 0 or 1
void Core_Mem_Bit_Write_With_V_MON(uint8_t bit, bool value); 	// bit 0-63, 0 or 1 with 4 analog values (Row/Col 0) pushed to serial port
bool Core_Mem_Bit_Read(uint8_t bit); 							// bit 0-63, return 0 or 1
void CoreSetup();
void CoreClearAll();
void CoreSetAll();
void Core_Mem_Array_Write();						// Transfer CoreArrayMemory 8x8 Array FLASH RAM into Core Memory RAM
void Core_Mem_Array_Read();							// Transfer Core Memory RAM into CoreArrayMemory 8x8 Array FLASH RAM 
void Core_Mem_Array_Write_Test_Pattern();
void Core_Mem_Scan_For_Magnet();							// Monitor for flux interference in Core Memory RAM, reported through CoreArrayMemory

// OLD API Command List
// extern void CoreWriteBit(uint8_t bit, bool value);
// extern bool CoreReadBit(uint8_t bit);
extern void CoreWriteLongInt(uint64_t value);
extern uint64_t CoreReadLongInt();
extern void CoreWriteArray(); 		// TO DO Add a pointer to the array
// extern uint64_t CoreReadArray(); 	// TO DO Add a pointer to the array
void ScrollTextToCoreMemory();

void AllDriveIoSafe();
void AllDriveIoReadAndStore();
void AllDriveIoRecallAndWrite();
extern void AllDriveIoSetBit(uint8_t bit);
extern void AllDriveIoClearBit(uint8_t bit);
void AllDriveIoEnable();
void AllDriveIoDisable();
extern bool CoreStateChangeFlag(bool clearFlag);

void IOESpare1_On();
void IOESpare1_Off();
void IOESpare2_On();
void IOESpare2_Off();

#endif // CORE_API_H
