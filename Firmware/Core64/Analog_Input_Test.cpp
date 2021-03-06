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

  // #define SEND_ANALOG_TO_SERIAL_PORT
  #define BATTERY_FILTER_SIZE  8 // Must be the decimal equivalent of the SHIFT that follows here
  #define BATTERY_FILTER_SHIFT 3 // bit shift 3 is divide by 8

  static uint16_t BatteryQuarterAvgmV = 0 ;
  static uint16_t BatteryAvgmV = 0 ;
  static float    BatteryQuarterAvgV  = 0 ;
  static float    BatteryAvgV  = 0 ;
  #if defined BOARD_CORE64_TEENSY_32
    // TO DO: Is the analog reference really 3.3V, set in Teensy?
    // ADC reading is 1/4 of the battery voltage, scaled relative to 3.3V Analog Reference
    // 3300 mV per 1023 counts = 3.226 milliVolts/count [1/4 battery voltage]
    // Multiply above result by 4 = 12.904 milliVolts/count [full battery voltage] 
    // ADC reading * 12.904 = battery voltage in milliVolts
  static float BatteryScalarADCtomV = 12.904; // 1:4
  #elif defined BOARD_CORE64C_RASPI_PICO
    // Arduino is scaling the native Pico ADC 0-4095 to 0-1023 for compatibility which requires multiply 4x below.
    // ADC reading is 1/3 of the battery voltage, scaled relative to 3.3V Analog Reference
    // 3300 mV per 4096 counts = .8057 milliVolts/count [1/3 battery voltage]
    // Multiply above result by 3 = 2.417 milliVolts/count [full battery voltage] 
    // ADC reading * 2.417 = battery voltage in milliVolts
  static float BatteryScalarADCtomV = 2.417*4; // 1:3
  #endif

  static uint16_t Analog_A10 = 0 ;
  static uint16_t BatteryVoltageFilterArray[BATTERY_FILTER_SIZE];
  static uint16_t BatteryVoltageFilterArrayTotal;
  static uint8_t BatteryVoltageFilterArrayPosition = 0;
  static bool BatteryVoltageFilterLoaded = false;

  static float Bus_5V0_V         = 0 ;
  static float Bus_3V3_V         = 0 ;
  static float Core_Col0_Q3P_Q3N_V = 0 ;
  static float Core_Col0_Q1P_Q1N_V = 0 ;
  static float Core_Row0_Q7P_Q7N_V = 0 ;
  static float Core_Row0_Q9P_Q9N_V = 0 ;

  static uint16_t Analog_A0  = 0 ;
  static uint16_t Analog_A1  = 0 ;
  static uint16_t Analog_A11 = 0 ;
  static uint16_t Analog_A12 = 0 ;
  static uint16_t Analog_A13 = 0 ;
  static uint16_t Analog_A14 = 0 ;
  static float    Bus_5V0_ADCtomV   = 6.452; // 1:2
  static float    Bus_3V3_ADCtomV   = 3.226; // 1:1
  static float    Core_Col0_ADCtomV = 3.226; // 1:1
  static float    Core_Row0_ADCtomV = 3.226; // 1:1

  uint16_t GetBatteryVoltagemV() {
    return (BatteryAvgmV);
  }

  float GetBatteryVoltageV() {
    return (BatteryAvgV);
  }

  float GetBus5V0VoltageV() {
    return (Bus_5V0_V);
  }

  float GetBus3V3VoltageV() {
    return (Bus_3V3_V);
  }

  float GetCoreTC0V() {
    return (Core_Col0_Q3P_Q3N_V);
  }

  float GetCoreBC0V() {
    return (Core_Col0_Q1P_Q1N_V);
  }

  float GetCoreLR0V() {
    return (Core_Row0_Q7P_Q7N_V);
  }

  float GetCoreRR0V() {
    return (Core_Row0_Q9P_Q9N_V);
  }

  void ReadAnalogVoltage() {

    #if defined BOARD_CORE64_TEENSY_32
      if ( (HardwareVersionMinor == 4) || (HardwareVersionMinor == 5) || (HardwareVersionMinor == 6) ) 
      {
        Analog_A10 = analogRead ( Pin_Battery_Voltage   );  //  VBAT_MON
        #if defined Pin_SPARE_3_Assigned_To_Spare_3_Analog_Input
        Analog_A0  = analogRead ( Pin_SPARE_3_Assigned_To_Spare_3_Analog_Input );  //  5V0_MON
        #endif
        Analog_A1  = analogRead ( Pin_SPI_Reset_Spare_5 );  //  3V3_MON and VMEM
        Analog_A11 = analogRead ( Pin_SPARE_ANA_6       );  //  TC0_MON ("top of COL 0 resistor" between Q3P and Q3N transistors)
        Analog_A12 = analogRead ( Pin_SPARE_ANA_7       );  //  BC0_MON ("bottom of COL 0 resistor" between Q1P and Q1N transistors)
        Analog_A13 = analogRead ( Pin_SPARE_ANA_8       );  //  LR0_MON ("left of ROW 0 resistor" between Q7P and Q7N transistors)
        Analog_A14 = analogRead ( Pin_Spare_ADC_DAC     );  //  RR0_MON ("right of ROW 0 resistor" between Q9P and Q9N transistors)
      }
    #elif defined BOARD_CORE64C_RASPI_PICO
      if ( (HardwareVersionMinor == 2) ) 
      {
        Analog_A10 = analogRead ( Pin_Battery_Voltage   );  //  VBAT_MON
      }
    #endif

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

    Bus_5V0_V           = Analog_A0  * Bus_5V0_ADCtomV   / 1000.0 ;
    Bus_3V3_V           = Analog_A1  * Bus_3V3_ADCtomV   / 1000.0 ;
    Core_Col0_Q3P_Q3N_V = Analog_A11 * Core_Col0_ADCtomV / 1000.0 ;
    Core_Col0_Q1P_Q1N_V = Analog_A12 * Core_Col0_ADCtomV / 1000.0 ;
    Core_Row0_Q7P_Q7N_V = Analog_A13 * Core_Row0_ADCtomV / 1000.0 ;
    Core_Row0_Q9P_Q9N_V = Analog_A14 * Core_Row0_ADCtomV / 1000.0 ;

  }

  void AnalogSetup() {
    // Nothing to do here because Teensy and Raspi Pico do not require explicit set-up for analog input pins.
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
      #ifdef SEND_ANALOG_TO_SERIAL_PORT
        Serial.print((BatteryAvgV),2);
        Serial.print(", ");
        Serial.print(Bus_5V0_V,2);
        Serial.print(", ");
        Serial.print(Bus_3V3_V,2);
        Serial.print(", ");
        Serial.print(Core_Col0_Q3P_Q3N_V,2);
        Serial.print(", ");
        Serial.print(Core_Col0_Q1P_Q1N_V,2);
        Serial.print(", ");
        Serial.print(Core_Row0_Q7P_Q7N_V,2);
        Serial.print(", ");
        Serial.print(Core_Row0_Q9P_Q9N_V,2);
        Serial.println();
      #endif
    }
  }

  void AnalogUpdateCoresOnly() {
    ReadAnalogVoltage();
    Serial.print((BatteryAvgV),2);
    Serial.print(", ");
    Serial.print(Bus_5V0_V,2);
    Serial.print(", ");
    Serial.print(Bus_3V3_V,2);
    Serial.print(", ");
    Serial.print(Core_Col0_Q3P_Q3N_V,2);
    Serial.print(", ");
    Serial.print(Core_Col0_Q1P_Q1N_V,2);
    Serial.print(", ");
    Serial.print(Core_Row0_Q7P_Q7N_V,2);
    Serial.print(", ");
    Serial.print(Core_Row0_Q9P_Q9N_V,2);
    Serial.println();
  }
