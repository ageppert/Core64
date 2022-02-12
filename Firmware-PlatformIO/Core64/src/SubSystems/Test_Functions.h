/*
Title:			Test Functions
Filename: 		Test_Functions.cpp and Test_Functions.h
Author: 		Andrew Geppert
Rev: 			0.1		
Date:			2021-12-23
Description:	Returns the state pass/fail state of tests for hardware checkout.
Usage:			Call from test mode.
*/
 
#ifndef TEST_FUNCTIONS_H
    #define TEST_FUNCTIONS_H

    #if (ARDUINO >= 100)
        #include <Arduino.h>
    #else
        #include <WProgram.h>
    #endif

    #include <stdint.h>

    uint8_t LoopBackTest();

#endif // TEST_FUNCTIONS_H
