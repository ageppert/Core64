/*
Title:          Debug Pins Management
Filename:       Debug_Pins_HAL.cpp and Debug_Pins_HAL.h
Author:         Andrew Geppert
Rev: 			0.1
Date:			2022-01-04
Description:	Setup and access the Debug Pins that available
Usage:			Debug_Pins_Setup() to configure pin mode.
                Debug_Pin_X(1) to set state of pin to ON.
                Debug_Pin_X(0) to set state of pin to OFF.
                Where "X" is GP pin number if the packaged [Arduino compatible] microcontroller board.
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
    extern void Debug_Pin_1 (bool High_nLow);

#endif // DEBUG_PINS_HAL_H
