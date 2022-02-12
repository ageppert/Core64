



// Core64 and Core64c detection and assignment of correct LED Matrix drive pin.
  #if defined(TEENSYDUINO) 
      //  --------------- Teensy -----------------
      #if defined(__MK20DX256__)       
          #define BOARD "Teensy 3.2 (MK20DX256)" // and Teensy 3.1 (obsolete)
          #define BOARD_TEENSY_32
      #endif
  #else // --------------- RP2040 ------------------
      #if defined(ARDUINO_ARCH_RP2040)       
          #define BOARD "Raspberry Pi Pico (RP2040)"
          #define BOARD_RASPI_PICO
      #endif
  #endif
// Display board name which is being compiled for:
  #pragma message ( "C Preprocessor identified board type:" )
  #ifdef BOARD
    #pragma message ( BOARD )
  #else
    #error ( "Unsupported board. Choose Teensy 3.1/3.2 or Raspberry Pi Pico in 'Tools > Boards' menu.")
  #endif
  
  