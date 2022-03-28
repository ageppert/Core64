#include <stdint.h>
#include <stdbool.h>

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Wire.h>
#include "Config/HardwareIOMap.h"
#include "Hal/Buttons_HAL.h"
#include "SubSystems/I2C_Manager.h"
#ifdef HALL_SENSOR_ENABLE
  #include "Libraries/Si7210/si7210.h"
  #include "Libraries/Si7210/si7210_defs.h"
#endif // HALL_SENSOR_ENABLE

// #define DEBUG_HALL_SENSORS
// #define DEBUG_HALL_SWITCHES

#ifdef HALL_SENSOR_ENABLE
  #define HALL_SENSOR_FIELD_STRENGTH_ON_POS_LEVEL      1     // Level of mT which registers as a button press
  #define HALL_SENSOR_FIELD_STRENGTH_ON_NEG_LEVEL     -1     // Level of mT which registers as a button press
  si7210_status_t rslt = SI7210_E_DEV_NOT_FOUND;  // Unexpected return from the sensor library, making sure it returns something other than 99

  #if defined BOARD_CORE64_TEENSY_32
    si7210_status_t usr_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
    {
        si7210_status_t rslt = SI7210_OK;

        /* User implemented I2C read function */
        // Serial.print("usr_i2c_read dev_id:     ");
        // Serial.println(dev_id,HEX);   // print the reading
        Wire.beginTransmission(dev_id);
        // Wire.setClock(1000000);             // TO DO: The speed setting is not working.
        Wire.write(reg_addr);
        Wire.endTransmission();
        Wire.requestFrom(dev_id, len);      // Assuming len is 1 because si7210.c only sends 1's when I search for "dev->read"
        *data = (uint8_t)Wire.read();
        // DEBUG
        // Serial.print("usr_i2c_read:     ");
        // Serial.println(*data,DEC);   // print the reading

        return rslt;
    }

    si7210_status_t usr_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
    {
        si7210_status_t rslt = SI7210_OK;

        /* User implemented I2C write function */
        Wire.beginTransmission(dev_id);
        // Wire.setClock(1000000);             // TO DO: The speed setting is not working.
        Wire.write(reg_addr);
        Wire.write(*data);                // Assuming len is 1 because si7210.c only sends 1's when I search for "dev->write"
        Wire.endTransmission();
        // DEBUG
        // Serial.print("usr_i2c_write:");
        // Serial.println(*data,DEC);
        
        return rslt;
    }
  #elif defined BOARD_CORE64C_RASPI_PICO
    MbedI2C I2C1_HALL_SENSOR(p10,p11);
    
    si7210_status_t usr_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
    {
        si7210_status_t rslt = SI7210_OK;

        /* User implemented I2C read function */
        // Serial.print("usr_i2c_read dev_id:     ");
        // Serial.println(dev_id,HEX);   // print the reading
        I2C1_HALL_SENSOR.beginTransmission(dev_id);
        // Wire.setClock(1000000);             // TO DO: The speed setting is not working.
        I2C1_HALL_SENSOR.write(reg_addr);
        I2C1_HALL_SENSOR.endTransmission();
        I2C1_HALL_SENSOR.requestFrom(dev_id, len);      // Assuming len is 1 because si7210.c only sends 1's when I search for "dev->read"
        *data = (uint8_t)I2C1_HALL_SENSOR.read();
        // DEBUG
        // Serial.print("usr_i2c_read:     ");
        // Serial.println(*data,DEC);   // print the reading

        return rslt;
    }

    si7210_status_t usr_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
    {
        si7210_status_t rslt = SI7210_OK;

        /* User implemented I2C write function */
        I2C1_HALL_SENSOR.beginTransmission(dev_id);
        // Wire.setClock(1000000);             // TO DO: The speed setting is not working.
        I2C1_HALL_SENSOR.write(reg_addr);
        I2C1_HALL_SENSOR.write(*data);                // Assuming len is 1 because si7210.c only sends 1's when I search for "dev->write"
        I2C1_HALL_SENSOR.endTransmission();
        // DEBUG
        // Serial.print("usr_i2c_write:");
        // Serial.println(*data,DEC);
        
        return rslt;
    }
  #endif

  void usr_delay_ms(uint32_t period_ms)
  {
      static uint16_t DelayScalar = 500;  // Convert ms to us. 1000 is 1:1. 500 is 2x faster. 2000 is 2x slow.
      delayMicroseconds(period_ms * DelayScalar);
      // delay(period_ms);
  }

  si7210_dev_t HallSensor1 = {
      SI7210_ADDRESS_0,
      usr_i2c_read,
      usr_i2c_write,
      usr_delay_ms
  };
  si7210_dev_t HallSensor2 = {
      SI7210_ADDRESS_1,
      usr_i2c_read,
      usr_i2c_write,
      usr_delay_ms
  };
  si7210_dev_t HallSensor3 = {
      SI7210_ADDRESS_2,
      usr_i2c_read,
      usr_i2c_write,
      usr_delay_ms
  };
  si7210_dev_t HallSensor4 = {
      SI7210_ADDRESS_3,
      usr_i2c_read,
      usr_i2c_write,
      usr_delay_ms
  };
#endif // HALL_SENSOR_ENABLE


void Buttons_Setup() {
  if ( (HardwareVersionMinor == 2) || (HardwareVersionMinor == 4) || (HardwareVersionMinor == 5) || (HardwareVersionMinor == 6) )
  // TODO: back out HWV minor == 2 and replace with hardware type == Core64c
  {
    #ifdef HALL_SENSOR_ENABLE
      if( HardwareConnectedCheckButtonHallSensors() ) {
        if((rslt = si7210_init(&HallSensor1)) != SI7210_OK)
            {
              Serial.print("      Init Result Code Not OK: ");
              Serial.println(rslt,DEC);
              //return rslt;
            }

        HallSensor1.settings.range        = SI7210_20mT;
        HallSensor1.settings.compensation = SI7210_COMPENSATION_TEMP_NEO;
        HallSensor1.settings.output_pin   = SI7210_OUTPUT_PIN_HIGH;
        
        if((rslt = si7210_set_sensor_settings(&HallSensor1)) != SI7210_OK)
            {
              Serial.print("      Set Sensor Settings Result Code Not OK: ");
              Serial.println(rslt,DEC);
              //return rslt;
            }
        else { Serial.println("      Hall 1 Set Sensor Settings Result OK."); }

        if((rslt = si7210_set_sensor_settings(&HallSensor2)) != SI7210_OK)
            {
              Serial.print("      Set Sensor Settings Result Code Not OK: ");
              Serial.println(rslt,DEC);
              //return rslt;
            }
        else { Serial.println("      Hall 2 Set Sensor Settings Result OK."); }

        if((rslt = si7210_set_sensor_settings(&HallSensor3)) != SI7210_OK)
            {
              Serial.print("      Set Sensor Settings Result Code Not OK: ");
              Serial.println(rslt,DEC);
              //return rslt;
            }
        else { Serial.println("      Hall 3 Set Sensor Settings Result OK."); }

        if((rslt = si7210_set_sensor_settings(&HallSensor4)) != SI7210_OK)
            {
              Serial.print("      Set Sensor Settings Result Code Not OK: ");
              Serial.println(rslt,DEC);
              //return rslt;
            }
        else { Serial.println("      Hall 4 Set Sensor Settings Result OK."); }

        if(rslt == SI7210_OK)
        {
            Serial.print("      Set Sensor Settings Result Code OK: ");
            Serial.println(rslt,DEC);
            float field_strength;
            float temperature;

            /* Obtain field strength reading from device */
            si7210_get_field_strength(&HallSensor1, &field_strength);

            /* Obtain a temperature reading from the device */
            si7210_get_temperature(&HallSensor1, &temperature);

            Serial.print("      Field / Temperature: ");
            Serial.print(field_strength);
            Serial.print(" / ");
            Serial.println(temperature);
        }

        rslt = si7210_check(&HallSensor1);
        Serial.print("      H1 SENSOR CHECK Result Code: ");
        Serial.println(rslt,DEC);

        rslt = si7210_self_test(&HallSensor1);
        Serial.print("      H1 SELF TEST Result Code: ");
        Serial.println(rslt,DEC);
      }
    #endif // HALL_SENSOR_ENABLE

    #ifdef HALL_SWITCH_ENABLE
      pinMode(PIN_HALL_SWITCH_1, INPUT_PULLUP);
      pinMode(PIN_HALL_SWITCH_2, INPUT_PULLUP);
      pinMode(PIN_HALL_SWITCH_3, INPUT_PULLUP);
      pinMode(PIN_HALL_SWITCH_4, INPUT_PULLUP);
    #endif // HALL_SWITCH_ENABLE
  }
}

uint32_t ButtonState(uint8_t button_number, uint32_t clear_duration) // send a 1 or more to clear, 0 to use normally)
{ 
  static uint32_t thistime = 0;
  static uint32_t Button1lasttime = 0;
  static uint32_t Button2lasttime = 0;
  static uint32_t Button3lasttime = 0;
  static uint32_t Button4lasttime = 0;
  static uint32_t Button1delta = 0;
  static uint32_t Button2delta = 0;
  static uint32_t Button3delta = 0;
  static uint32_t Button4delta = 0;
  static uint32_t duration_b1 = 0;  // Button 1 duration ON
  static uint32_t duration_b2 = 0;  // Button 2 duration ON
  static uint32_t duration_b3 = 0;  // Button 3 duration ON
  static uint32_t duration_b4 = 0;  // Button 4 duration ON
  // static uint8_t  state_b1 = 0;
  // static uint8_t  state_b2 = 0;
  // static uint8_t  state_b3 = 0;
  // static uint8_t  state_b4 = 0;
  static bool     sensed = false;

    #ifdef HALL_SWITCH_ENABLE             // These need to be set every time to ensure they are configured for use here.
      pinMode(PIN_HALL_SWITCH_1, INPUT_PULLUP);
      pinMode(PIN_HALL_SWITCH_2, INPUT_PULLUP);
      pinMode(PIN_HALL_SWITCH_3, INPUT_PULLUP);
      pinMode(PIN_HALL_SWITCH_4, INPUT_PULLUP);
    #endif // HALL_SWITCH_ENABLE

    thistime = millis();
    sensed = false;
    float field_strength;

    if( HardwareConnectedCheckButtonHallSensors() ) {
      switch (button_number) {
        case 1:
          if(clear_duration == 1) { duration_b1 = 0 ;}
          Button1delta = thistime - Button1lasttime ;
          #ifdef HALL_SENSOR_ENABLE
            si7210_get_field_strength(&HallSensor1, &field_strength);
            if(   ((int)field_strength > HALL_SENSOR_FIELD_STRENGTH_ON_POS_LEVEL) 
               || ((int)field_strength < HALL_SENSOR_FIELD_STRENGTH_ON_NEG_LEVEL) )
              { duration_b1 = duration_b1 + Button1delta ; sensed = true; }
          #endif
          #ifdef HALL_SWITCH_ENABLE
            if(digitalRead(PIN_HALL_SWITCH_1)==0) { duration_b1 = duration_b1 + Button1delta ; sensed = true; }
          #endif
          if(!sensed) { duration_b1 = 0; }
          break;
        
        case 2:
          if(clear_duration == 1) { duration_b2 = 0 ;}
          Button2delta = thistime - Button2lasttime ;
          #ifdef HALL_SENSOR_ENABLE
            si7210_get_field_strength(&HallSensor2, &field_strength);
            if(   ((int)field_strength > HALL_SENSOR_FIELD_STRENGTH_ON_POS_LEVEL) 
               || ((int)field_strength < HALL_SENSOR_FIELD_STRENGTH_ON_NEG_LEVEL) )
              { duration_b2 = duration_b2 + Button2delta ; sensed = true; }
          #endif
          #ifdef HALL_SWITCH_ENABLE
            if(digitalRead(PIN_HALL_SWITCH_2)==0) { duration_b2 = duration_b2 + Button2delta ; sensed = true; }
          #endif
          if(!sensed) { duration_b2 = 0; }
          break;

        case 3:
          if(clear_duration == 1) { duration_b3 = 0 ;}
          Button3delta = thistime - Button3lasttime ;
          #ifdef HALL_SENSOR_ENABLE
            si7210_get_field_strength(&HallSensor3, &field_strength);
            if(   ((int)field_strength > HALL_SENSOR_FIELD_STRENGTH_ON_POS_LEVEL) 
               || ((int)field_strength < HALL_SENSOR_FIELD_STRENGTH_ON_NEG_LEVEL) )
              { duration_b3 = duration_b3 + Button3delta ; sensed = true; }
          #endif
          #ifdef HALL_SWITCH_ENABLE
            if(digitalRead(PIN_HALL_SWITCH_3)==0) { duration_b3 = duration_b3 + Button3delta ; sensed = true; }
          #endif
          if(!sensed) { duration_b3 = 0; }
          break;
        case 4:
          if(clear_duration == 1) { duration_b4 = 0 ;}
          Button4delta = thistime - Button4lasttime ;
          #ifdef HALL_SENSOR_ENABLE
            si7210_get_field_strength(&HallSensor4, &field_strength);
            if(   ((int)field_strength > HALL_SENSOR_FIELD_STRENGTH_ON_POS_LEVEL) 
               || ((int)field_strength < HALL_SENSOR_FIELD_STRENGTH_ON_NEG_LEVEL) )
              { duration_b4 = duration_b4 + Button4delta ; sensed = true; }
          #endif
          #ifdef HALL_SWITCH_ENABLE
            if(digitalRead(PIN_HALL_SWITCH_4)==0) { duration_b4 = duration_b4 + Button4delta ; sensed = true; }
          #endif
          if(!sensed) { duration_b4 = 0; }
          break;

        default:
          Serial.println("Invalid Button");
          break;
        }
      }
  #if defined DEBUG_HALL_SENSORS  
    Serial.print("HALL SENSOR 1,2,3,4: ");
    Serial.print(duration_b1);
    Serial.print(", ");
    Serial.print(duration_b2);
    Serial.print(", ");
    Serial.print(duration_b3);
    Serial.print(", ");
    Serial.print(duration_b4);
    Serial.println("");
  #endif

  #if defined DEBUG_HALL_SWITCHES  
    Serial.print("HALL SWITCH 1,2,3,4: ");
    Serial.print(digitalRead(PIN_HALL_SWITCH_1));
    Serial.print(", ");
    Serial.print(digitalRead(PIN_HALL_SWITCH_2));
    Serial.print(", ");
    Serial.print(digitalRead(PIN_HALL_SWITCH_3));
    Serial.print(", ");
    Serial.print(digitalRead(PIN_HALL_SWITCH_4));
    Serial.println("");
  #endif

          // DEBUG
          // Serial.print("Hall 1 Field / On Duration: ");
          // Serial.print(field_strength);
          // Serial.print(" / ");
          // Serial.println(duration_b1);
  
  switch (button_number) {
    case 1:
      Button1lasttime = thistime;
      return duration_b1; // in ms
      break;
    case 2:
      Button2lasttime = thistime;
      return duration_b2; // in ms
      break;
    case 3:
      Button3lasttime = thistime;
      return duration_b3; // in ms
      break;
    case 4:
      Button4lasttime = thistime;
      return duration_b4; // in ms
      break;

    default:
      Serial.println("Invalid Button");
      break;
  }

}
