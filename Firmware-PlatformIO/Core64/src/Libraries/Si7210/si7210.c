// https://github.com/FARLY7/si7210-driver
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
 * File        si7210.c
 * Created by  Sean Farrelly
 * Version     1.0
 * 
 */

/*! @file si7210.c
 * @brief Driver for Si7210 Hall-effect sensor.
 */
#include "Libraries/Si7210/si7210.h"
#include <stddef.h>
#include <math.h>

/* Si7210 Register addresses */
#define SI72XX_HREVID       0xC0U
#define SI72XX_DSPSIGM      0xC1U
#define SI72XX_DSPSIGL      0xC2U
#define SI72XX_DSPSIGSEL    0xC3U
#define SI72XX_POWER_CTRL   0xC4U
#define SI72XX_ARAUTOINC    0xC5U
#define SI72XX_CTRL1        0xC6U
#define SI72XX_CTRL2        0xC7U
#define SI72XX_SLTIME       0xC8U
#define SI72XX_CTRL3        0xC9U
#define SI72XX_A0           0xCAU
#define SI72XX_A1           0xCBU
#define SI72XX_A2           0xCCU
#define SI72XX_CTRL4        0xCDU
#define SI72XX_A3           0xCEU
#define SI72XX_A4           0xCFU
#define SI72XX_A5           0xD0U
#define SI72XX_OTP_ADDR     0xE1U
#define SI72XX_OTP_DATA     0xE2U
#define SI72XX_OTP_CTRL     0xE3U
#define SI72XX_TM_FG        0xE4U

/* Si7210 Register bit masks */
#define CHIP_ID_MASK        0xF0U
#define REV_ID_MASK         0x0FU
#define DSP_SIGSEL_MASK     0x07U
#define MEAS_MASK           0x80U
#define USESTORE_MASK       0x08U
#define ONEBURST_MASK       0x04U
#define STOP_MASK           0x02U
#define SLEEP_MASK          0x01U
#define ARAUTOINC_MASK      0x01U
#define SW_LOW4FIELD_MASK   0x80U
#define SW_OP_MASK          0x7FU
#define SW_FIELDPOLSEL_MASK 0xC0U
#define SW_HYST_MASK        0x3FU
#define SW_TAMPER_MASK      0xFCU
#define SL_FAST_MASK        0x02U
#define SL_TIMEENA_MASK     0x01U
#define DF_BURSTSIZE_MASK   0xE0U
#define DF_BW_MASK          0x1EU
#define DF_IIR_MASK         0x01U
#define OTP_READ_EN_MASK    0x02U
#define OTP_BUSY_MASK       0x01U
#define TM_FG_MASK          0x03U

#define DSP_SIGM_DATA_FLAG      0x80U
#define DSP_SIGM_DATA_MASK      0x7FU
#define DSP_SIGSEL_TEMP_MASK    0x01U
#define DSP_SIGSEL_FIELD_MASK   0x04U

/* Burst sizes */
#define DF_BW_1             0x0U << 1
#define DF_BW_2             0x1U << 1
#define DF_BW_4             0x2U << 1
#define DF_BW_8             0x3U << 1
#define DF_BW_16            0x4U << 1
#define DF_BW_32            0x5U << 1
#define DF_BW_64            0x6U << 1
#define DF_BW_128           0x7U << 1
#define DF_BW_256           0x8U << 1
#define DF_BW_512           0x9U << 1
#define DF_BW_1024          0xAU << 1
#define DF_BW_2048          0xBU << 1
#define DF_BW_4096          0xCU << 1
#define DF_BURSTSIZE_1      0x0U << 5
#define DF_BURSTSIZE_2      0x1U << 5
#define DF_BURSTSIZE_4      0x2U << 5
#define DF_BURSTSIZE_8      0x3U << 5
#define DF_BURSTSIZE_16     0x4U << 5
#define DF_BURSTSIZE_32     0x5U << 5
#define DF_BURSTSIZE_64     0x6U << 5
#define DF_BURSTSIZE_128    0x7U << 5


/*!
 * @brief This internal API reads the factory programmed temperature
 *        offset and gain adjustment values and saves them to device
 *        structure.
 * 
 * @param[in] dev : Structure instance of si210_dev.
 *
 * @return Result of API execution status
 */
static si7210_status_t si7210_read_temperature_calib_values(si7210_dev_t *dev);

/*!
 * @brief This internal API loads the factory compensation data from
 *        OTP registers into run-time registers.
 *
 * @param[in] dev : Structure instance of si210_dev.
 *
 * @return Result of API execution status
 */
static si7210_status_t si7210_load_compensation_values(si7210_dev_t *dev);

/*!
 * @brief This internal API is used to validate the device pointer for
 *        null conditions.
 *
 * @param[in] dev : Structure instance of si210_dev.
 *
 * @return Result of API execution status
 */
static si7210_status_t null_ptr_check(const si7210_dev_t *dev);

/**
  * @brief This API initialises the device and checks if responding.
  */
si7210_status_t si7210_init(si7210_dev_t *dev)
{
    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Set callback as __weak implementation. */
    if(dev->threshold_callback == NULL)
        dev->threshold_callback = si7210_threshold_callback;

    /* Wake device up incase it is sleeping. */
    if((rslt = si7210_wakeup(dev)) != SI7210_OK)
        return rslt;

    uint8_t try_count = 5;
    while(try_count)
    {
        /* Check if device is responding */
        if((rslt = si7210_check(dev)) == SI7210_OK)
            break;

        /* Wait for 10ms and count unsuccessful attempts */
        dev->delay_ms(10); // Default was 10
        try_count--;
    }

    /* Error: Timeout / device not found */
    if(rslt != SI7210_OK)
        return SI7210_E_DEV_NOT_FOUND;

    /* Set device and internal driver settings */
    if((rslt = si7210_set_sensor_settings(dev)) != SI7210_OK)
        return rslt;
    
    /* Disable periodic auto-wakeup by device, and tamper detect. */
    if((rslt !=si7210_write_reg(dev, SI72XX_CTRL3, (uint8_t) ~SL_TIMEENA_MASK, 0)) != SI7210_OK)
        return rslt;

    /* Disable tamper detection by setting sw_tamper to 63 */
    rslt |= si7210_write_reg(dev, SI72XX_CTRL3, SL_FAST_MASK | SL_TIMEENA_MASK, 63 << 2);
        
    return rslt;
}

/**
  * @brief This API de-initialises the device and puts it to sleep mode.
  */
si7210_status_t si7210_deinit(si7210_dev_t *dev)
{
    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Disable sleep timer */
    rslt |= si7210_write_reg(dev, SI72XX_CTRL3, (uint8_t) ~SL_TIMEENA_MASK, 0);

    /* Disable user store of values, and enter sleep mode with no measurements */
    rslt |= si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK, SLEEP_MASK);

    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    return rslt;
}

/**
  * @brief This API changes the settings of the device and internal driver.
  * 
  * @param[in] dev : Si7210 device structure
  * 
  * @return result of API execution status
  * @retval si7210_status
  */
si7210_status_t si7210_set_sensor_settings(si7210_dev_t *dev)
{
    si7210_status_t rslt;

    /* Range */
    /* Range is used internally, no need to change device. */

    /* Compensation */
    /* Compensation is used internally, no need to change device. */

    /* Output pin */
    uint8_t ctrl1 = 0;
    /* Set sw_low4field bit if output pin is in LOW configuration.
     * Clear sw_low4field bit if output is in HIGH configuration.*/
    switch(dev->settings.output_pin)
    {
        case SI7210_OUTPUT_PIN_LOW:  ctrl1 |= SW_LOW4FIELD_MASK;  break;
        case SI7210_OUTPUT_PIN_HIGH: ctrl1 &= ~SW_LOW4FIELD_MASK; break;
    }

    /* Set SW_LOW4FIELD */
    rslt = si7210_write_reg(dev, SI72XX_CTRL1, (uint8_t) ~SW_LOW4FIELD_MASK, ctrl1);
   
    return rslt;
}

/*!
 * @brief This API gets the last measured field strength from the device.
 *        The value is correctly compensated.
 */
si7210_status_t si7210_get_field_strength(si7210_dev_t *dev, float *field)
{
    si7210_status_t rslt;
    /* Temp variable used for register read values */
    uint8_t val = 0;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;
   
    /* Stop the control loop by setting stop bit */
    if((rslt = si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK | USESTORE_MASK, STOP_MASK)) != SI7210_OK) /* WARNING: Removed USE_STORE MASK */
        return rslt;

    /* Load compensation values from OTP registers, to be used in measurement. */
    if((rslt = si7210_load_compensation_values(dev)) != SI7210_OK)
        return rslt;

    /* Use a burst size of 128/4096 samples in FIR and IIR modes */
    // rslt |= si7210_write_reg(dev, SI72XX_CTRL4, 0, DF_BURSTSIZE_128 | DF_BW_4096); // default
    rslt |= si7210_write_reg(dev, SI72XX_CTRL4, 0, DF_BURSTSIZE_16 | DF_BW_32);

    /* Selet field strength measurement */
    rslt |= si7210_write_reg(dev, SI72XX_DSPSIGSEL, 0, DSP_SIGSEL_FIELD_MASK);

    /* Start measurement */
    rslt |= si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK | USESTORE_MASK, ONEBURST_MASK);

    /* Error configuring and starting measurement. */
    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    /* Wait until measurement complete by checking fresh data bit */
    uint8_t retries = 5;
    while(retries)
    {
        /* Read most-significant byte */
        if((rslt = si7210_read_reg(dev, SI72XX_DSPSIGM, &val)) == SI7210_OK)
        {
            if((val & DSP_SIGM_DATA_FLAG) == DSP_SIGM_DATA_FLAG)
                break;
        }
        else
            return rslt;

        /* Wait 1ms until reading complete */
        dev->delay_ms(1); // Default was 10
        retries--;
    };

    /* Timeout error */
    if((val & DSP_SIGM_DATA_FLAG) != DSP_SIGM_DATA_FLAG)
        return SI7210_E_TIMEOUT;
    
    /*! ================================================== */
    /*! For calculations below, refer to Si7210 datasheet. */
    /*! ================================================== */

    int32_t value = 256 * (val & DSP_SIGM_DATA_MASK);

    /* Read least-significant byte of data */
    if((rslt = si7210_read_reg(dev, SI72XX_DSPSIGL, &val)) != SI7210_OK)
        return rslt;
        
    value += val;
    value -= 16384U;

    float raw_field = (float) value;

    if(dev->settings.range == SI7210_20mT)
        *field = raw_field * 0.00125; /* rawField * 1.25 */
    else if(dev->settings.range == SI7210_200mT)
        *field = raw_field * 0.0125; /* rawField * 12.5 */
    else
        rslt = SI7210_E_INVALID_ARG;

    /* Set SW_OP and SW_LOW4FIELD */
    rslt |= si7210_read_reg(dev, SI72XX_CTRL1, &val);

    /* Set SW_HYST (Hysterisis) to 0 */
    rslt |= si7210_read_reg(dev, SI72XX_CTRL2, &val);

    return rslt;
}

/*!
  * @brief This API gets the last measured temperature from the device.
  *        The value is correctly compensated.
  */
si7210_status_t si7210_get_temperature(si7210_dev_t *dev, float *temperature)
{
    /* Temp variable used for register read values */
    uint8_t val = 0;

    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Stop the control loop by setting stop bit */
    rslt |= si7210_write_reg(dev, SI72XX_POWER_CTRL, (uint8_t) ~(STOP_MASK), STOP_MASK);

    /* Do not use burst measurement type */
    rslt |= si7210_write_reg(dev, SI72XX_CTRL4, 0, 0x0);

    /* Select temperature measurement */
    rslt |= si7210_write_reg(dev, SI72XX_DSPSIGSEL, 0, DSP_SIGSEL_TEMP_MASK);

    /* Start measurement */
    rslt |= si7210_write_reg(dev, SI72XX_POWER_CTRL, (uint8_t) ~(STOP_MASK), ONEBURST_MASK);

    /* Error configuring and starting measurement */
    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    /* Wait until measurement complete by checking fresh data bit */
    uint8_t retries = 5;
    while(retries)
    {
        /* Read most-significant byte */
        if((rslt = si7210_read_reg(dev, SI72XX_DSPSIGM, &val)) == SI7210_OK)
        {
            if((val & DSP_SIGM_DATA_FLAG) == DSP_SIGM_DATA_FLAG)
                break;
        }
        else
            return rslt;

        /* Wait 1ms until reading complete */
        dev->delay_ms(1);
        retries--;
    };

    /* Timeout error */
    if((val & DSP_SIGM_DATA_FLAG) != DSP_SIGM_DATA_FLAG)
        return SI7210_E_TIMEOUT;

    /*! ================================================== */
    /*! For calculations below, refer to Si7210 datasheet. */
    /*! ================================================== */

    int32_t value = 32 * (val & DSP_SIGM_DATA_MASK);
    
    /* Read the least-significant byte */
    if((rslt = si7210_read_reg(dev, SI72XX_DSPSIGL, &val)) != SI7210_OK)
        return rslt;

    value += (val >> 3);

    /* If no offset and gain values exist, read them now. */
    if (dev->calib_data.temperature_offset == 0 && dev->calib_data.temperature_gain == 0)
    {
        if((rslt = si7210_read_temperature_calib_values(dev)) != SI7210_OK)
            return rslt;
    }

    /* Error reading OTP values */
    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    float temp_c = (float) value;
    float offset = dev->calib_data.temperature_offset / 16.0;
    float gain   = 1 + (dev->calib_data.temperature_gain / 2048.0);
    
    temp_c = gain * (-3.83e-6F * temp_c * temp_c + 0.16094F * temp_c - 279.80F - 0.222F * 3.3F) + offset;

    *temperature = temp_c;

    return rslt;
}

/*!
 * @brief This API puts the device into it's independent sleep/measure cycle.
 *        The device will continue to periodically measure the field and
 *        trigger the output pin when the threshold is crossed.
 */
si7210_status_t si7210_start_periodic_measurement(si7210_dev_t *dev)
{
    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Enable periodic wakeup */
    if((rslt = si7210_write_reg(dev, SI72XX_CTRL3, (uint8_t) ~SL_TIMEENA_MASK, SL_TIMEENA_MASK)) != SI7210_OK)
        return rslt;

    /* Start measurement */
    /* Change to ~STOP_MASK with STOP_MASK */
    if((rslt = si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK | USESTORE_MASK, 0)) != SI7210_OK)
        return rslt;

    return rslt;
}

/*!
  * @brief This API sets the threshold triggering of the device.
  *        Whereby, the output pin of the device will trigger when the magnetic
  *        field exceeds a particular threshold.
  */
si7210_status_t si7210_set_threshold(si7210_dev_t *dev, float threshold)
{
    si7210_status_t rslt;
    uint8_t ctrl1 = 0;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    if(dev->settings.range == SI7210_200mT)
    {
        if(threshold >= 190.0)
            return SI7210_E_INVALID_ARG;
    }
    else if(dev->settings.range == SI7210_20mT)
    {
        if(threshold >= 19.0)
            return SI7210_E_INVALID_ARG;
    }
    
    /* Stop the control state measurement loop */
    /* Replace with ~STOP_MASK */
    if((rslt = si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK | USESTORE_MASK, STOP_MASK)) != SI7210_OK)
        return rslt;
    
    switch(dev->settings.range)
    {
        case SI7210_200mT: threshold /= 0.05;  break; // 200mT
        case SI7210_20mT:  threshold /= 0.005; break; // 20mT
    }

    /* Keep threshold within supported bounds */
    if(threshold < 16)
        threshold = 16;    
    else if(threshold > 3840)
        threshold = 3840;

    uint8_t divides = 0;

    while((threshold >= 16) && (threshold >= 31))
    {
        threshold = threshold / 2;
        divides++;
    }

    threshold -= 16;
    threshold = roundf(threshold);

    uint8_t sw_op_3_0 = (uint8_t) threshold;
    uint8_t sw_op_6_4 = divides;

    ctrl1 |= (sw_op_3_0 & 0x0F) << 0;
    ctrl1 |= (sw_op_6_4 & 0x07) << 4;

    /* Set SW_OP */
    rslt |= si7210_write_reg(dev, SI72XX_CTRL1, (uint8_t) ~SW_OP_MASK, ctrl1);

    /* Disable hysterisis by setting sw_hyst to 63 */
    rslt |= si7210_write_reg(dev, SI72XX_CTRL2, 0, 63);
    
    /* Ensure the OTP data is not reloaded after each sleep cycle */
    rslt |= si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK, USESTORE_MASK | STOP_MASK);

    if(rslt != SI7210_OK)
        return SI7210_E_IO;
    
    return rslt;
}

/*!
  * @brief This API reads a register from Si7210 device.
  */
si7210_status_t si7210_read_reg(si7210_dev_t *dev, uint8_t reg, uint8_t *val)
{
    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Call user implemented method for I2C read */
    rslt = dev->read(dev->dev_id, reg, val, 1);

    return rslt;
}

/*!
  * @brief This API writes a value to a register of Si7210 device.
  */
si7210_status_t si7210_write_reg(si7210_dev_t *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
    si7210_status_t rslt;

    uint8_t temp_val = 0;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* A mask has been used, must read original value from register. */
    if(mask != 0)
    {
        /* Call user implemented method for I2C read */
        if((rslt = dev->read(dev->dev_id, reg, &temp_val, 1)) != SI7210_OK)
            return rslt;

        temp_val &= mask;
    }

    temp_val |= val;

    /* Call user implemented method for I2C write */
    if((rslt = dev->write(dev->dev_id, reg, &temp_val, 1)) != SI7210_OK)
        return rslt;

    return rslt;
}

/*!
  * @brief This API checks the device is responding to commands.
  */
si7210_status_t si7210_check(si7210_dev_t *dev)
{
    si7210_status_t rslt = SI7210_OK;
    uint8_t temp;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;
    
    /* Read the device ID and revision fields to check comms. */
    rslt = si7210_read_reg(dev, SI72XX_HREVID, &temp);

    return rslt;
}

/*!
  * @brief This API puts the device into SLEEP mode.
 */
si7210_status_t si7210_sleep(si7210_dev_t *dev)
{
    uint8_t temp;

    si7210_status_t rslt;
    
    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;
    
    /* Read CTRL3 register */
    rslt |= si7210_read_reg(dev, SI72XX_CTRL3, &temp);
     
    temp &= 0xFEU; /* Clear SLTIMENA bit of CTRL3 register */
    
    /* Write back new CTRL3 register value  */
    rslt |= si7210_write_reg(dev, SI72XX_CTRL3, 0, temp);

    /* Read POWER_CTRL register */
    rslt |= si7210_read_reg(dev, SI72XX_POWER_CTRL, &temp);
    
    temp = (temp & 0xF8U) | 0x01; /* Clear STOP and set SLEEP bits */
    
    /* Write back POWER_CTRL register value */
    rslt |= si7210_write_reg(dev, SI72XX_POWER_CTRL, MEAS_MASK, temp);

    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    return rslt;
}

/*!
 * @brief This API wakes the device from SLEEP mode.
 */ 
si7210_status_t si7210_wakeup(si7210_dev_t *dev)
{
    uint8_t temp = 0;
    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Wake the device up by sending a WRITE request. 
     * API call will fail as we are unable to write to memory address 0x00,
     * however it will wake the part up. */
    dev->write(dev->dev_id, 0x00, &temp, 1);
        
    return rslt;
}

/*!
 * @brief This API executes a self-test sequence offered by the device.
 *        It uses an internal coil to generate and test the + and - field.
 */
si7210_status_t si7210_self_test(si7210_dev_t *dev)
{
    float field_pos, field_neg;
    si7210_status_t rslt;

    /* Check for null pointer in device structure */
    if((rslt = null_ptr_check(dev)) != SI7210_OK)
        return rslt;

    /* Enable test field generator coil in POSITIVE direction. */
    rslt |= si7210_write_reg(dev, SI72XX_TM_FG, 0, 1);

    /* Measure field strength */
    rslt |= si7210_get_field_strength(dev, &field_pos); // 200mT

    /* Enable test field generator coil in POSITIVE direction. */
    rslt |= si7210_write_reg(dev, SI72XX_TM_FG, 0, 2);

    /* Measure field strength */
    rslt |= si7210_get_field_strength(dev, &field_neg);

    /* Disable test field generator coil. */
    rslt |= si7210_write_reg(dev, SI72XX_TM_FG, 0, 0);

    /* Error in performing self-test measurement. */
    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    float b_out = 1.16 * SI7210_VDD;
    float b_upper = b_out + (b_out * 0.25); /* +25% */
    float b_lower = b_out - (b_out * 0.25); /* -25% */

    if( (field_pos <= b_upper) &&
        (field_pos >= b_lower) &&
        (field_neg >= (b_upper * -1)) &&
        (field_neg <= (b_lower * -1)))
    {
        rslt = SI7210_OK;
    }
    else
    {
        rslt = SI7210_E_SELF_TEST_FAIL;
    }

    return rslt;
}

/*!
 * @brief This API should be called inside the ISR capturing the triggering
 *        of the device's output pin.
 *        This is required to support threshold trigger functionality.
 */
void si7210_irq_handler(si7210_dev_t *dev)
{
    dev->threshold_callback(dev);
}

/*!
 * @brief This function represents a placeholder for the callback called
 *        by the driver in the event the output pin is triggered.
 */
__attribute__((weak)) void si7210_threshold_callback(si7210_dev_t *dev)
{
    /* Prevent unused argument(s) compilation warning */
    (void)dev;

    /* NOTE : This function should not be modified, when the callback is needed,
              the si7210_threshold_callback should be implemented in the user file
     */
}

/*!
 * @brief This internal API reads the factory programmed temperature
 *        offset and gain adjustment values.
 */
static si7210_status_t si7210_read_temperature_calib_values(si7210_dev_t *dev)
{
    si7210_status_t rslt = SI7210_OK;

    /* Read out compensation values */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, 0x1DU);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg(dev, SI72XX_OTP_DATA, (uint8_t*) &dev->calib_data.temperature_offset);

    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, 0x1EU);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg(dev, SI72XX_OTP_DATA, (uint8_t*) &dev->calib_data.temperature_gain);

    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    return rslt;
}

/*!
 * @brief This internal API loads the factory compensation data from
 *        OTP registers into run-time registers.
 */
static si7210_status_t si7210_load_compensation_values(si7210_dev_t *dev)
{
    si7210_status_t rslt = SI7210_OK;
    uint8_t base_addr;
    uint8_t val;

    switch(dev->settings.compensation)
    {
        case SI7210_COMPENSATION_NONE:     { base_addr = 0x21; break; }
        case SI7210_COMPENSATION_TEMP_NEO: { base_addr = 0x2D; break; }
        case SI7210_COMPENSATION_TEMP_CER: { base_addr = 0x39; break; }
        default: { return SI7210_E_SETTINGS; }
    }

    if(dev->settings.range == SI7210_200mT)
        base_addr += 6;

    /* Load A0 register */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, base_addr);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg (dev, SI72XX_OTP_DATA, &val);
    rslt |= si7210_write_reg(dev, SI72XX_A0, 0, val);

    /* Load A1 register */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, base_addr + 1);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg (dev, SI72XX_OTP_DATA, &val);
    rslt |= si7210_write_reg(dev, SI72XX_A1, 0, val);
    
    /* Load A2 register */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, base_addr + 2);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg (dev, SI72XX_OTP_DATA, &val);
    rslt |= si7210_write_reg(dev, SI72XX_A2, 0, val);
    
    /* Load A3 register */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, base_addr + 3);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg (dev, SI72XX_OTP_DATA, &val);
    rslt |= si7210_write_reg(dev, SI72XX_A3, 0, val);
    
    /* Load A4 register */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, base_addr + 4);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg (dev, SI72XX_OTP_DATA, &val);
    rslt |= si7210_write_reg(dev, SI72XX_A4, 0, val);
    
    /* Load A5 register */
    rslt |= si7210_write_reg(dev, SI72XX_OTP_ADDR, 0, base_addr + 5);
    rslt |= si7210_write_reg(dev, SI72XX_OTP_CTRL, 0, OTP_READ_EN_MASK);
    rslt |= si7210_read_reg (dev, SI72XX_OTP_DATA, &val);
    rslt |= si7210_write_reg(dev, SI72XX_A5, 0, val);

    if(rslt != SI7210_OK)
        return SI7210_E_IO;

    return rslt;
}

/*!
 * @brief This internal API is used to validate the device pointer for
 * null conditions.
 */
static si7210_status_t null_ptr_check(const si7210_dev_t *dev)
{
    si7210_status_t rslt;

    if (dev == NULL || dev->read == NULL || dev->write == NULL)
    {
        /* Device structure pointer is not valid */
        rslt = SI7210_E_NULL_PTR;
    }
    else
    {
        /* Device structure is fine */
        rslt = SI7210_OK;
    }

    return rslt;
}