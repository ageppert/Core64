/*
PURPOSE: 
SETUP:
*/
#ifndef MODE_MANAGER_H
    #define MODE_MANAGER_H
    #include <stdint.h>
    enum TopLevelMode                  // Top Level Mode State Machine
    {
    MODE_START_POWER_ON              =   0,                  // Start-up, then go to DEMO #101 as the default
    //MODE_START_INIT                  =   1,                  // TODO: 
    //MODE_START_EEPROM                =   2,                  // TODO: 
    
    MODE_DGAUSS_MENU                  = 100,                  // DGAUSS MENU (TOP LEVEL MENU)

        MODE_DEMO_SUB_MENU              = 200,                  // TODO: Implement sub-menu
            MODE_DEMO_SCROLLING_TEXT             = 201,             // Scrolling text
            MODE_DEMO_LED_TEST_ONE_MATRIX_MONO   = 202,             // Testing LED Driver with matrix array and monochrome color
            MODE_DEMO_LED_TEST_ONE_MATRIX_COLOR  = 203,             // Testing LED Driver with matrix array and multi-color symbols
            MODE_DEMO_END_OF_LIST                = 204,             // End of the demo loop, go back to the beginning.
            //MODE_DEMO_LOW_POWER                  = 208,             // TODO: Lower power mode
            //MODE_DEMO_SLEEP                      = 209,             // TODO: Sleep mode 

        MODE_GAME_SUB_MENU              = 300,                  // TODO: Implement sub-menu
            //MODE_GAME_SNAKE                 = 301,                  // TODO: 
            //MODE_GAME_PONG                  = 302,                  // TODO: 

        MODE_APP_SUB_MENU               = 400,                  // TODO: Implement sub-menu
            MODE_APP_DRAW                   = 401,                  // Drawing mode, of course!

        MODE_UTIL_SUB_MENU              = 500,                  // TODO: Implement sub-menu
            MODE_UTIL_FLUX_DETECTOR         = 501,                  // Flux detector, shows cores being affected by magnetic flux in real-time
            //MODE_UTIL_64BIT_MEMORY         = 502,                  // TODO: read/write 64 bit values from serial port

        MODE_SETTINGS_SUB_MENU          = 600,                  // TODO: Implement sub-menu
            //MODE_SETTINGS_BRIGHTNESS        = 601,                  // TODO: Adjust screen brightness, and select automatic adjustment on/off
            //MODE_SPECIAL_EEPROM_USER_SETTINGS      = 602,                  // TODO:
            //MODE_SPECIAL_EEPROM_USER_NAME      = 714,                  // TODO:

        MODE_SPECIAL_SUB_MENU           = 700,                  // TODO: Implement sub-menu
            MODE_LED_TEST_ALL_BINARY        = 701,                  // Test LED Driver with binary values
            MODE_LED_TEST_ONE_STRING        = 702,                  // Testing LED Driver
            MODE_TEST_EEPROM                = 703,                  // Read out the raw EEPROM values
            MODE_LED_TEST_ALL_COLOR         = 704,                  // Test LED Driver with all pixels and all colors
            MODE_CORE_TOGGLE_BIT            = 705,                  // Test one core with one function
            MODE_CORE_TEST_ONE              = 706,                  // Testing core #coreToTest and displaying core state
            MODE_CORE_TEST_MANY             = 707,                  // Testing multiple cores and displaying core state
            MODE_HALL_TEST                  = 708,                  // Testing hall switch and sensor response
            MODE_SPECIAL_LOOPBACK_TEST      = 709,                  // For the manufacturing test fixture, test unused IO pins
            //MODE_SPECIAL_STANDALONE_TEST      = 712,                  // TODO: For the manufacturing test fixture, test unused IO pins
            MODE_HARD_REBOOT                = 799,                 // Hard Reboot.

    // Only accessible from the command line. Not accessible from DGAUSS menu.
    MODE_MANUFACTURING_MENU             = 800,                  // TODO: Implement sub-menu
        //MODE_SPECIAL_EEPROM_FACTORY_PROGRAM      = 801,             // TODO:


    MODE_LAST                       = 999                  // Last one, return to Startup 0.
    } ;

    void    TopLevelModeSetToDefault();

    void    TopLevelModeSet          (uint16_t value);
    void    TopLevelModeSetInc       ();
    void    TopLevelModeSetDec       ();
    uint16_t TopLevelModeGet         ();

    void TopLevelModeSetPrevious     (uint16_t value);
    uint16_t TopLevelModeGetPrevious ();

    void    TopLevelModeSetChanged   (bool value);
    bool    TopLevelModeGetChanged   ();

    void TopLevelThreeSoftButtonGlobalEnableSet (bool value);
    bool TopLevelThreeSoftButtonGlobalEnableGet ();

    void    TopLevelModeManagerRun   ();

#endif
