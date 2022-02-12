/*
PURPOSE: 
SETUP:
*/
 
#ifndef MODE_MANAGER_H
    #define MODE_MANAGER_H

    #include <stdint.h>

    void    SetModeTopLevelDefault();

    void    SetTopLevelMode         (uint8_t value);
    void    SetTopLevelModeInc      ();
    uint8_t GetTopLevelMode         ();
    
    void    SetTopLevelModeChanged  (bool value);
    bool    GetTopLevelModeChanged  ();

    void    TopLevelModeRun         ();

#endif
