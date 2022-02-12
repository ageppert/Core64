# https://github.com/FARLY7/si7210-driver


# Si7210 Hall-effect sensor driver API
A portable driver written in C for Silicon Lab's Si7210 hall-effect sensor.


## Introduction
This package contains the Si7210 sensor driver.
The driver includes si7210.c, si7210.h, and si7210_defs.h files.

## Integration details
* Integrate si7210.c, si7210.h, and si7210_defs.h files into the project
* Include the si7210.h file in your code like below.

``` c
#include "si7210.h"
```

## File information
* si7210_defs.h : This header file contains definitions used by both the user and driver APIs.
* si7210.h : This header file contains the declarations of the driver APIs.
* lmt01.c : This source file contains the definitions of the driver APIs.

## Supported interfaces
* I2C

## User Guide

### Initialising the device
To intialise the device, the user must create a device structure. The user can do this by creating an instance of the structure si7210_dev. The user must then fill in the various parameters as shown below to initialise the device. THe usermust then choose the particular settings they wish to use, by filling in the 'settings' member of the device struct.

``` c
si7210_status rslt = SI7210_OK;

struct si7210_dev dev = {
    .dev_id   = SI7210_ADDRESS_0;
    .read     = usr_i2c_read;
    .write    = usr_i2c_write;
    .delay_ms = usr_delay_ms;
    .callback = callback;
};

if((rslt = si7210_init(&dev) != SI7210_OK)
    return rslt;

dev.settings.range        = SI7210_20mT;
dev.settings.compensation = SI7210_COMPENSATION_TEMP_NEO;
dev.settings.output_pin   = SI7210_OUTPUT_PIN_HIGH;
 
if((rslt = si7210_set_sensor_settings(&dev)) != SI7210_OK)
    return rslt;

if(rslt == SI7210_OK)
{
    float field;
    float temperature;

    /* Obtain field strength reading from device */
    si7210_get_field_strength(&dev, &field_strength);

    /* Obtain a temperature reading from the device */
    si7210_get_temperature(&dev, &temperature);

    printf("Field: %fmT\tTemperature: %f*C", field, temperature);
}
````

### Templates for function pointers
``` c
si7210_status usr_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    si7210_status rslt = SI7210_OK;

    /* User implemented I2C read function */

    return rslt;
}

si7210_status usr_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    si7210_status rslt = SI7210_OK;

    /* User implemented I2C write function */

    return rslt;
}

void usr_delay_ms(uint32_t period_ms)
{
    /* User implemented delay (ms) function */
}

```
