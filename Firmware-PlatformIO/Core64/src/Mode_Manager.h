/*
PURPOSE: 
SETUP:
*/
#ifndef MODE_MANAGER_H
    #define MODE_MANAGER_H
    #include <stdint.h>

    // All of these items in the Enum list must be updated in the corresponding TOP_LEVEL_MODE_NAME_ARRAY of Mode_Manager.cpp to match 1:1.
    enum TopLevelMode                  // Top Level Mode State Machine
    {
    MODE_START_POWER_ON                             ,                        // Start-up, then go to DEMO #101 as the default
        MODE_START_INIT                             ,                        // TODO: 
        MODE_START_EEPROM                           ,                        // TODO: 
        MODE_START_SEQUENCE_COMPLETE                ,                        // TODO: 
    MODE_DGAUSS_MENU                                ,                        // DGAUSS MENU (TOP LEVEL MENU)
        MODE_DEMO_SUB_MENU                          ,                  // TODO: Implement sub-menu
            MODE_DEMO_SCROLLING_TEXT                ,             // Scrolling text
            MODE_DEMO_LED_TEST_ONE_MATRIX_MONO      ,             // Testing LED Driver with matrix array and monochrome color
            MODE_DEMO_LED_TEST_ONE_MATRIX_COLOR     ,             // Testing LED Driver with matrix array and multi-color symbols
            MODE_DEMO_END_OF_LIST                   ,             // End of the demo loop, go back to the beginning.
            //MODE_DEMO_LOW_POWER                   ,             // TODO: Lower power mode
            //MODE_DEMO_SLEEP                       ,             // TODO: Sleep mode 
        MODE_GAME_SUB_MENU                          ,                  // TODO: Implement sub-menu
            //MODE_GAME_SNAKE                       ,                  // TODO: 
            //MODE_GAME_PONG                        ,                  // TODO: 
        MODE_APP_SUB_MENU                           ,                    // TODO: Implement sub-menu
            MODE_APP_DRAW                           ,                  // Drawing mode, of course!
        MODE_UTIL_SUB_MENU                          ,                  // TODO: Implement sub-menu
            MODE_UTIL_FLUX_DETECTOR                 ,                  // Flux detector, shows cores being affected by magnetic flux in real-time
            //MODE_UTIL_64BIT_MEMORY                ,                  // TODO: read/write 64 bit values from serial port
        MODE_SPECIAL_SUB_MENU                       ,                  // TODO: Implement sub-menu
            MODE_LED_TEST_ALL_BINARY                ,                  // Test LED Driver with binary values
            MODE_LED_TEST_ONE_STRING                ,                  // Testing LED Driver
            MODE_TEST_EEPROM                        ,                  // Read out the raw EEPROM values
            MODE_LED_TEST_ALL_COLOR                 ,                  // Test LED Driver with all pixels and all colors
            MODE_CORE_TOGGLE_BIT                    ,                  // Test one core with one function
            MODE_CORE_TEST_ONE                      ,                  // Testing core #coreToTest and displaying core state
            MODE_CORE_TEST_MANY                     ,                  // Testing multiple cores and displaying core state
            MODE_HALL_TEST                          ,                  // Testing hall switch and sensor response
            MODE_SPECIAL_LOOPBACK_TEST              ,                  // For the manufacturing test fixture, test unused IO pins
            //MODE_SPECIAL_STANDALONE_TEST          ,                  // TODO: For the manufacturing test fixture, test unused IO pins
            MODE_SPECIAL_HARD_REBOOT                        ,                 // Hard Reboot.
        MODE_SETTINGS_SUB_MENU                      ,                  // TODO: Implement sub-menu
            //MODE_SETTINGS_BRIGHTNESS              ,                  // TODO: Adjust screen brightness, and select automatic adjustment on/off
            //MODE_SETTINGS_EEPROM_USER_SETTINGS    ,                  // TODO:
            //MODE_SETTINGS_EEPROM_USER_NAME        ,                  // TODO:
    MODE_MANUFACTURING_MENU                         ,                  // TODO: Implement sub-menu. Only accessible from the command line. Not accessible from DGAUSS menu.
        //MODE_SPECIAL_EEPROM_FACTORY_PROGRAM       ,             // TODO:
    MODE_LAST                                                        // Last one, return to Startup 0. Note, this is different than "default" which is handled by switch/case statement.
    } ;

    extern const char* TOP_LEVEL_MODE_NAME_ARRAY[];

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
