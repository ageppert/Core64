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
 * File        si7210.h
 * Created by  Sean Farrelly
 * Version     1.0
 * 
 */

/*! @file si7210.h
 * @brief Driver for Si7210 Hall-effect sensor.
 */

/*!
 * @defgroup SI7210 Sensor API
 */
#ifndef _SI7210_H_
#define _SI7210_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "si7210_defs.h"

/*!
 * @brief This API initialises the device and checks if responding.
 * 
 * @param[in] dev : Si7210 device structure
 * 
 * @return result of API execution status
 * @retval si7210_status
 */
si7210_status_t si7210_init(si7210_dev_t *dev);

/*!
 * @brief This API de-initialises the device and puts it to sleep mode.
 * 
 * @param[in] dev : Si7210 device structure
 * 
 * @return result of API execution status
 * @retval si7210_status
 */
si7210_status_t si7210_deinit(si7210_dev_t *dev);

/*!
 * @brief This API changes the settings of the device and internal driver.
 * 
 * @param[in] dev : Si7210 device structure
 * 
 * @return result of API execution status
 * @retval si7210_status
 */
si7210_status_t si7210_set_sensor_settings(si7210_dev_t *dev);

/*!
 * @brief This API gets the last measured field strength from the device.
 *        The value is correctly compensated.
 * 
 * @param[in]  dev   : Si7210 device structure.
 * @param[out] field : Pointer to value which is to be written.
 *  
 * @return Success of operation
 * @retval si7210_status
 */
si7210_status_t si7210_get_field_strength(si7210_dev_t *dev, float *field);

/*!
 * @brief This API gets the last measured temperature from the device.
 *        The value is correctly compensated.
 * 
 * @param[in]  dev         : Si7210 device structure.
 * @param[out] temperature : Pointer to value which is to be written.
 *  
 * @return Success of operation
 * @retval si7210_status
 */
si7210_status_t si7210_get_temperature(si7210_dev_t *dev, float *temperature);

/*!
 * @brief This API sets the threshold triggering of the device.
 *        Whereby, the output pin of the device will trigger when the magnetic
 *        field exceeds a particular threshold.
 * 
 * @note The user must inlude the si7210_irq_handler function call inside the
 *       ISR of the GPIO receiving the output pin, and pass the sensor context.
 * 
 * @param[in] dev       : Si7210 device structure.
 * @param[in] threshold : Value at which to set the treshold.
 * @param[in] range     : Range of value (20mT or 200mT).
 * @param[in] pin       : State of output pin when field reading exceeds threshold.
 * @param[out] data : Pointer to where register value is to be stored.
 *  
 * @return Success of read operation.
 * @retval si7210_status
 */
si7210_status_t si7210_set_threshold(si7210_dev_t *dev, float threshold);

/*!
 * @brief This API puts the device into it's independent sleep/measure cycle.
 *        The device will continue to periodically measure the field and
 *        trigger the output pin when the threshold is crossed.
 * 
 * @param[in]  dev         : Si7210 device structure.
 *  
 * @return Success of operation
 * @retval si7210_status
 */
si7210_status_t si7210_start_periodic_measurement(si7210_dev_t *dev);

/*!
 * @brief This API reads a register from Si7210 device.
 * 
 * @param[in] dev : Si7210 device structure.
 * @param[in] reg : Register number to read from.
 * @param[out] data : Pointer to where register value is to be stored.
 *  
 * @return Success of read operation.
 * @retval si7210_status
 */
si7210_status_t si7210_read_reg(si7210_dev_t *dev, uint8_t reg, uint8_t *val);

/*!
 * @brief This API writes a value to a register of Si7210 device.
 * 
 * @param[in] dev : Si7210 device structure.
 * @param[in] reg : Register number to write to.
 * @param[in] mask : 1-byte mask used to keep original register bits.
 * @param[in] data : 1-byte data to write to register.
 * 
 * @return Success of write operation.
 * @retval si7210_status
 */
si7210_status_t si7210_write_reg(si7210_dev_t *dev, uint8_t reg, uint8_t mask, uint8_t val);

/*!
 * @brief This API checks if the device is responding.
 * 
 * @param[in] dev : Si7210 device structure.
 * 
 * @return Success of operation.
 * @retval si7210_status
 */
si7210_status_t si7210_check(si7210_dev_t *dev);

/*!
 * @brief This API executes a self-test sequence offered by the device.
 *        It uses an internal coil to generate and test the + and - field.
 * 
 * @note Important: There must not be any external magnetic field in the vicinity
 *       for the test to run successfully.
 *       The user must also ensure the correct Vdd value is defined in si7210_defs.h.
 * 
 * @param[in] dev : Si7210 device structure.
 * 
 * @return Success of operation.
 * @retval si7210_status
 */
si7210_status_t si7210_self_test(si7210_dev_t *dev);

/*!
 * @brief This API puts the device into SLEEP mode.
 * 
 * @param[in] dev : Si7210 device structure.
 * 
 * @return Success of operation.
 * @retval si7210_status
 */
si7210_status_t si7210_sleep(si7210_dev_t *dev);

/*!
 * @brief This API wakes the device from SLEEP mode.
 * 
 * @param[in] dev : Si7210 device structure.
 * @param[in] reg : Register number to write to.
 * @param[in] data : 1-byte data to write to register.
 * 
 * @return Success of operation.
 * @retval si7210_status
 */
si7210_status_t si7210_wakeup(si7210_dev_t *dev);

/*!
 * @brief This API offers a callback which is triggered whenever the output
 *        pin triggers an interrupt.
 * 
 * @param[in] dev : Pointer to device structure.
 * 
 * @return void
 */
void si7210_threshold_callback(si7210_dev_t *dev);

/*!
 * @brief This API should be called inside the ISR capturing the triggering
 *        of the device's output pin.
 *        This is required to support threshold trigger functionality.
 * 
 * @param[in] dev : Si7210 device structure.
 * 
 * @return void
 */
void si7210_irq_handler(si7210_dev_t *dev);


#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* SI7210_H_ */
/** @}*/