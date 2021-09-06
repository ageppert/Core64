#include <stdint.h>
#include <stdbool.h>

/*
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
*/

#include "Analog_Input_Test.h"

#include "HardwareIOMap.h"
#if defined BOARD_CORE64_TEENSY_32

  // #define SEND_ANALOG_TO_SERIAL_PORT
  #define BATTERY_FILTER_SIZE  8 // 
  #define BATTERY_FILTER_SHIFT 3 // bit shift 3 is divide by 8

  static uint16_t BatteryQuarterAvgmV = 0 ;
  static uint16_t BatteryAvgmV = 0 ;
  static float    BatteryQuarterAvgV  = 0 ;
  static float    BatteryAvgV  = 0 ;
    // TO DO: Is the analog reference really 3.3V, set in Teensy?
    // ADC reading is 1/4 of the battery voltage, scaled relative to 3.3V Analog Reference
    // 3300 mV per 1023 counts = 3.226 milliVolts/count [1/4 battery voltage]
    // Multiply above result by 4 = 12.904 milliVolts/count [full battery voltage] 
    // ADC reading * 12.904 = battery voltage in milliVolts
  static float BatteryScalarADCtomV = 12.903; // 1:4
  static uint16_t Analog_A10 = 0 ;
  static uint16_t BatteryVoltageFilterArray[BATTERY_FILTER_SIZE];
  static uint16_t BatteryVoltageFilterArrayTotal;
  static uint8_t BatteryVoltageFilterArrayPosition = 0;
  static bool BatteryVoltageFilterLoaded = 0;

  static float Bus_5V0      = 0 ;
  static float Core_Col0_Q5  = 0 ;
  static float Core_Col0_R9  = 0 ;
  static float Core_Col0_U7  = 0 ;
  static float Core_Col0_Q2  = 0 ;
  static float Core_Col0_Q21 = 0 ;

  static uint16_t Analog_A0  = 0 ;
  static float    Bus_5V0_ADCtomV = 6.452; // 1:2
  static uint16_t Analog_A1  = 0 ;
  static uint16_t Analog_A11 = 0 ;
  static uint16_t Analog_A12 = 0 ;
  static uint16_t Analog_A13 = 0 ;
  static uint16_t Analog_A14 = 0 ;
  static float    Core_Col0_ADCtomV = 3.226; // 1:1

  uint16_t GetBatteryVoltagemV() {
    return (BatteryAvgmV);
  }

  float GetBatteryVoltageV() {
    return (BatteryAvgV);
  }

  void ReadAnalogVoltage() {

    if ( (HardwareVersionMinor == 4) || (HardwareVersionMinor == 5) ) 
    {
      Analog_A10 = analogRead ( Pin_Battery_Voltage   );  //  VBAT_MON
      Analog_A0  = analogRead ( Pin_SPARE_3_Assigned_To_Spare_3_Analog_Input );  //  5V0       // TO DO: Test with serial
      Analog_A1  = analogRead ( Pin_SPI_Reset_Spare_5 );  //  VMEM (3V3 or "top of upper drive transitor, emitter")
      Analog_A11 = analogRead ( Pin_SPARE_ANA_6       );  //  R9 ("top of COL0 resistor" between Q3P and Q3N transistors)
      Analog_A12 = analogRead ( Pin_SPARE_ANA_7       );  //  D7 ("bottom of COL0 resistor" or "top of diode" before it enters the diode drop)
      Analog_A13 = analogRead ( Pin_SPARE_ANA_8       );  //  Q2 ("top of bottom drive transistor, collector")
      Analog_A14 = analogRead ( Pin_Spare_ADC_DAC     );  //  Q21 ("top of Enable FET, drain")
    }
    
    if(!BatteryVoltageFilterLoaded)
    {
      for(uint8_t i = 0; i < BATTERY_FILTER_SIZE; i++)
      {
        BatteryVoltageFilterArray[i] = Analog_A10;
      }
      BatteryVoltageFilterLoaded = 1;
    }
    
    if(BatteryVoltageFilterArrayPosition > (BATTERY_FILTER_SIZE-1) )
    {
      BatteryVoltageFilterArrayPosition = 0;
    }
    else
    {
      BatteryVoltageFilterArrayPosition++;
    }

    BatteryVoltageFilterArray[BatteryVoltageFilterArrayPosition] = Analog_A10;

    BatteryVoltageFilterArrayTotal = 0;
    for(uint8_t i = 0; i < BATTERY_FILTER_SIZE; i++)
    {
      BatteryVoltageFilterArrayTotal = BatteryVoltageFilterArrayTotal + BatteryVoltageFilterArray[i];
    }
    BatteryQuarterAvgmV = BatteryVoltageFilterArrayTotal >> BATTERY_FILTER_SHIFT;
    BatteryAvgmV        = (uint16_t)(BatteryQuarterAvgmV * BatteryScalarADCtomV);
    BatteryAvgV         = (float)(BatteryAvgmV / 1000.0) ;

    Bus_5V0       = Analog_A0  * Bus_5V0_ADCtomV   / 1000.0 ;
    Core_Col0_Q5  = Analog_A1  * Core_Col0_ADCtomV / 1000.0 ;
    Core_Col0_R9  = Analog_A11 * Core_Col0_ADCtomV / 1000.0 ;
    Core_Col0_U7  = Analog_A12 * Core_Col0_ADCtomV / 1000.0 ;
    Core_Col0_Q2  = Analog_A13 * Core_Col0_ADCtomV / 1000.0 ;
    Core_Col0_Q21 = Analog_A14 * Core_Col0_ADCtomV / 1000.0 ;

  }

  void AnalogSetup() {
    // Nothing to do here because Teensy does not require explicit set-up for analog-only pins
  }

  void AnalogUpdate() {
    static unsigned long ReadPeriodms = 100;
    static unsigned long NowTime = 0;
    static unsigned long AnalogReadTimer = 0;
    
    NowTime = millis();
    if ((NowTime - AnalogReadTimer) >= ReadPeriodms)
    {
      AnalogReadTimer = NowTime;
      ReadAnalogVoltage();
      /*
      Serial.print("Battery ");
      Serial.print(GetBatteryVoltagemV());
      Serial.println(" mV.");
      */
      #ifdef SEND_ANALOG_TO_SERIAL_PORT
        Serial.print((BatteryAvgV),2);
        Serial.print(", ");
        Serial.print(Bus_5V0,2);
        Serial.print(", ");
        Serial.print(Core_Col0_Q5,2);
        Serial.print(", ");
        Serial.print(Core_Col0_R9,2);
        Serial.print(", ");
        Serial.print(Core_Col0_U7,2);
        Serial.print(", ");
        Serial.print(Core_Col0_Q2,2);
        Serial.print(", ");
        Serial.print(Core_Col0_Q21,2);
        Serial.println();
      #endif
    }
  }

#elif defined BOARD_CORE64C_RASPI_PICO

#endif
