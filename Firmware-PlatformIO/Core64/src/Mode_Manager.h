/*
PURPOSE: 
SETUP:
*/
 
#ifndef MODE_MANAGER_H
    #define MODE_MANAGER_H

    #include <stdint.h>

    void    SetTopLevelModeDefault();

    void    SetTopLevelMode          (uint16_t value);
    void    SetTopLevelModeInc       ();
    uint16_t GetTopLevelMode         ();

    void SetTopLevelModePrevious     (uint16_t value);
    uint16_t GetTopLevelModePrevious ();


    void    SetTopLevelModeChanged   (bool value);
    bool    GetTopLevelModeChanged   ();

    void    TopLevelModeRun          ();

#endif
