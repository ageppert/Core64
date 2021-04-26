/*
 * MIT License
 * 
 * Copyright (c) 2019 Sean Farrelly
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * File        Si7210_defs.h
 * Created by  Sean Farrelly
 * Version     1.0
 * 
 */

/*! @file Si7210_defs.h
 * @brief Definitions used by driver/user for Si7210 Sensor API.
 */

/*!
 * @defgroup SI7210 DEF API
 */
#ifndef _SI7210_DEFS_H_
#define _SI7210_DEFS_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Possible I2C slave addresses */
#define SI7210_ADDRESS_0    (0x30U ) // << 1)
#define SI7210_ADDRESS_1    (0x31U ) // << 1)
#define SI7210_ADDRESS_2    (0x32U ) // << 1)
#define SI7210_ADDRESS_3    (0x33U ) // << 1)

/* Vdd of SI7210. This is used in device's self-test calculations */
#define SI7210_VDD          (3.3f)

/*!
 * @brief Si7210 API status result code.
 */
typedef enum
{
    SI7210_OK,                  /* Sucess                               */
    SI7210_E_NULL_PTR,          /* Null pointer found                   */
    SI7210_E_INVALID_ARG,       /* Invalid function arguments           */
    SI7210_E_IO,                /* Device IO error                      */
    SI7210_E_TIMEOUT,           /* Device timeout                       */
    SI7210_E_DEV_NOT_FOUND,     /* Device not found                     */
    SI7210_E_SELF_TEST_FAIL,    /* Device self-test failed              */
    SI7210_E_SETTINGS,          /* Unable to set device settings        */
    SI7210_W_THRESHOLD_BOUNDS   /* Threshold parameter outside bounds   */
} si7210_status_t;

/*!
 * @brief Measurement scale of reading.
 */
typedef enum
{
    SI7210_20mT,    /* 20mT measurement range */
    SI7210_200mT    /* 200mT measurement range */
} si7210_range_t;

/*!
 * @brief Magnet temperature compensation.
 */
typedef enum
{
    SI7210_COMPENSATION_NONE,     /* No magnet temperature compensation          */
    SI7210_COMPENSATION_TEMP_NEO, /* Magnet temperature compensation (Neodymium) */
    SI7210_COMPENSATION_TEMP_CER, /* Magnet temperature compensation (Ceramic)   */
} si7210_compensation_t;

/*!
 * @brief State of Si7210's output pin when field is above threshold setting.
 */
typedef enum
{
    SI7210_OUTPUT_PIN_LOW,  /* Output pin is LOW when field is above threshold.  */   
    SI7210_OUTPUT_PIN_HIGH  /* Output pin is HIGH when field is above threshold. */
} si7210_output_pin_t;

/*!
 * @brief Type definitions
 */
typedef si7210_status_t (*si7210_com_fptr_t)(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len);
typedef void (*si7210_delay_fptr_t)(uint32_t period);
//typedef void (*si7210_callback_fptr_t)(void *context);

/*!
 * @brief Si7210 calibration data structure.
 * 
 * @note For use of internal driver, not user.
 */
struct si7210_calib_data
{
    int8_t temperature_offset; /* Temperature sensor offset adjustment */   
    int8_t temperature_gain;   /* Temperature sensor gain adjustment */
};

/*!
 * @brief Si7210 settings data structure.
 */
struct si7210_settings
{
    si7210_range_t         range;        /* Measurement range of magentic reading             */
    si7210_compensation_t  compensation; /* Temperature compensation of magentic reading      */
    si7210_output_pin_t    output_pin;   /* State of output pin when field is above threshold */
};

/*!
 * @brief Si7210 sensor structure
 */
typedef struct _si7210_dev_t
{
    uint8_t dev_id;                      /* Device ID                           */
        
    si7210_com_fptr_t read;              /* I2C read function pointer           */
    
    si7210_com_fptr_t write;             /* I2C write function pointer          */
    
    si7210_delay_fptr_t delay_ms;        /* Delay (ms) function pointer         */

    void (* threshold_callback)(struct _si7210_dev_t *dev);             /*!< SPI Tx Completed callback          */

    //si7210_callback_fptr_t callback;     /* Threshold callback function pointer */
    
    struct si7210_settings settings;     /* Si7210 device settings */

    struct si7210_calib_data calib_data; /* Calibration data                    */

} si7210_dev_t;

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* SI7210_H_ */
/** @}*/