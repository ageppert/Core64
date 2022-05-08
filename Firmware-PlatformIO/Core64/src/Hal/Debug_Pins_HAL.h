/*
Title:          Debug Pins Management
Filename:       Debug_Pins_HAL.cpp and Debug_Pins_HAL.h
Author:         Andrew Geppert
Rev: 			0.1
Date:			2022-01-04
Description:	Setup and access the available Debug Pins as defined in HardwareIOMap.h
Usage:			Debug_Pins_Setup() to configure pin mode.
                Debug_Pin_X(1) to set state of pin to ON.
                Debug_Pin_X(0) to set state of pin to OFF.
                Where "X" is GP number if the packaged [Arduino compatible] microcontroller board. GP number does not always match board pin number.
                TracingPulses_Debug_Pin_X(#) to toggle (pulse/twiddle) GP "X" a # of times.
*/
 
#ifndef DEBUG_PINS_HAL_H
    #define DEBUG_PINS_HAL_H

    #if (ARDUINO >= 100)
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #include <stdint.h>

    extern void Debug_Pins_Setup();
    extern void Debug_Pin_1(bool High_nLow);
    extern void Debug_Pin_2(bool High_nLow);
    extern void Debug_Pin_3(bool High_nLow);
    extern void Debug_Pin_4(bool High_nLow);
    extern void Debug_Pin_5(bool High_nLow);
    extern void Debug_Pin_6(bool High_nLow);
    extern void Debug_Pin_7(bool High_nLow);
    extern void Debug_Pin_8(bool High_nLow);
    extern void Debug_Pin_SPI_CS1(bool High_nLow);
    extern void Debug_Pin_SPI_RST(bool High_nLow); 
    extern void Debug_Pin_SPI_CD(bool High_nLow);
    extern void Debug_Pin_SPI_SDO(bool High_nLow); 
    extern void Debug_Pin_SPI_SDI(bool High_nLow); 
    extern void Debug_Pin_SPI_CLK(bool High_nLow);  
    extern void TracingPulses_Debug_Pin_1(uint8_t numberOfPulses);
    extern void DebugAllGpioToggleTest();

#endif // DEBUG_PINS_HAL_H
