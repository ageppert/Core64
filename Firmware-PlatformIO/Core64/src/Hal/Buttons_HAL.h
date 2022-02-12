/*
Title:			  Generic Button Handler
Filename: 		Buttons_HAL.cpp and Buttons_HAL.h
Author: 		  Andrew Geppert
Rev: 			    0.3		
Date:			    2020-05-31
Description:	Returns the state of the button as active/inactive along with duration of that state.
Usage:			  Buttons_Setup() to configure pin mode and low/high to active/inactive logic.
              ButtonsState() to query active/inactive state and duration (ms)
*/
 
#ifndef BUTTONS_HAL_H
    #define BUTTONS_HAL_H

    #if (ARDUINO >= 100)
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #include <stdint.h>

    extern void Buttons_Setup();
    extern uint32_t ButtonState(uint8_t button_number, uint32_t clear_duration);  // Send a 1 or more to clear timer, 0 to read timer)

#endif // BUTTONS_HAL_H
