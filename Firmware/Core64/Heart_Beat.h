/*
PURPOSE: Heart beat built-in LED with non-blocking code
SETUP:
- Use the built-in LED, and assign the proper pin number in HardwareIOMap.h for the device in use.
*/
 
#ifndef HEART_BEAT_H
	
	#define HEART_BEAT_H

	#include <stdint.h>
	#include <stdbool.h>

	void HeartBeatSetup();
	void HeartBeat();

#endif
