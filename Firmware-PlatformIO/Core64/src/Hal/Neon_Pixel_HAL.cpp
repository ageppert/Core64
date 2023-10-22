#include <stdint.h>
#include <stdbool.h>

/*
#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif
*/

#include "Config/HardwareIOMap.h"
#include "Config/CharacterMap.h"

#if defined  MCU_TYPE_MK20DX256_TEENSY_32
  #include "Hal/Core_HAL.h"       // TODO: This core_api shouldn't be directly accessed from this files. Interaction should be through higher level application
  #include "Drivers/NeonPixel_Driver.h"
  #include "Hal/LED_Array_HAL.h"

  #ifdef NEON_PIXEL_ARRAY
    #define NUM_PIXELS 64

    NeonPixelMatrix neonPixelMatrix = NeonPixelMatrix(8, 8); // This sets of the size of the screen buffer. Can be larger than display size to make scrolling easier.
    // May need to force a 1ms delay between Neon Pixel updates or the values won't latch.

    // Neon Pixel Array Memory Buffers for user representations of the Neon Pixel Array.
    // Interaction with the abstract memory buffers which define the Neon Pixel Array for the user to view:
    // BINARY [64 bit data word, monochrome]
      // 0 is lower right (LSb), 63 is upper left (MSb), counting right to left, then up to the next row. Each row up is a higher Byte.
      static uint64_t NeonPixelArrayMemoryBinary = 0;
      static uint64_t NeonPixelArrayMemoryBinaryDefault = 0xDEADBEEFC0D3C4FE;
    // STRING [1D 64 pixel string, monochrome]
      // 0 is upper left, 63 is lower right, counting left to right, then down to next row
      static bool NeonPixelArrayMemoryString [64];  
    // MATRIX MONOCHROME [2D 8x8 pixel matrix, monochrome]
      // order y,x : 0,0 is upper left, 7,7 is lower right, counting left to right and top to bottom.
      static bool NeonPixelScreenMemoryMatrixMono [8][8];      


    // Look up tables to translate the 1D and 2D user representations of the array to the Neon Pixel positions used by the Neon Pixel Array Driver.
    const uint8_t NeonPixelPositionBinaryLUT [64] = { // Maps Screen Pixel Position to Neon Pixel Binary Display position.
      63,62,61,60,59,58,57,56, 
      48,49,50,51,52,53,54,55, 
      47,46,45,44,43,42,41,40,
      32,33,34,35,36,37,38,39,
      31,30,29,28,27,26,25,24,
      16,17,18,19,20,21,22,23,
      15,14,13,12,11,10, 9, 8,
       0, 1, 2, 3, 4, 5, 6, 7  
      };
    const uint8_t NeonPixelPosition1DLUT [64] = { // Maps Screen Pixel Position to Neon Pixel 1D array position.
       7,15,23,31,39,47,55,63,
       6,14,22,30,38,46,54,62,
       5,13,21,29,37,45,53,61,
       4,12,20,28,36,44,52,60,
       3,11,19,27,35,43,51,59,
       2,10,18,26,34,42,50,58,
       1, 9,17,25,33,41,49,57,
       0, 8,16,24,32,40,48,56
      };

    const uint8_t NeonScreenPixelPosition2DLUT [8][8] = { // Maps Screen Pixel Position to Neon Pixel 2D array position.
      { 7,15,23,31,39,47,55,63},
      { 6,14,22,30,38,46,54,62},
      { 5,13,21,29,37,45,53,61},
      { 4,12,20,28,36,44,52,60},
      { 3,11,19,27,35,43,51,59},
      { 2,10,18,26,34,42,50,58},
      { 1, 9,17,25,33,41,49,57},
      { 0, 8,16,24,32,40,48,56}
      };

    // PWM Neon Pixel brightness is only in the range 0x00 to 0x7F, while 0x80 to 0xFF is full brightness.
    // High brightness pulls down USB/BAT power available and system becomes instable.
    uint8_t NeonPixelBrightnessOn  = 0x7F;
    uint8_t NeonPixelBrightnessOff = 0x00;
    uint8_t MatrixHeight = 8;
    uint8_t MatrixWidth = 8;


    void Neon_Pixel_Array_Memory_Clear() {
      NeonPixelArrayMemoryBinary = 0;
      for( uint8_t i = 0; i < NUM_PIXELS; i++) {
        NeonPixelArrayMemoryString[i] = 0;
      }
      for( uint8_t y = 0; y < MatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < MatrixWidth; x++) 
        {
          NeonPixelScreenMemoryMatrixMono[y][x] = 0;
        }
      }
    }
    //
    // Copy Core Memory Array bits into monochrome Neon Pixel Array memory
    //
      void CopyCoreMemoryToMonochromeNeonPixelArrayMemory() {
        for( uint8_t y = 0; y < MatrixHeight; y++) {
          for( uint8_t x = 0; x < MatrixWidth; x++) {
            NeonPixelScreenMemoryMatrixMono[y][x] = CoreArrayMemory[y][x];
            NeonPixelScreenMemoryMatrixMono[y][x] = CoreArrayMemory[y][x];
          }
        }
      }

  //
  // Copy Color Font Symbol into Neon Pixel Array memory
  //
    void CopyColorFontSymbolToNeonPixelArrayMemory(uint8_t SymbolNumber) {
      for( uint8_t y = 0; y < MatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < MatrixWidth; x++) 
        {
          NeonPixelScreenMemoryMatrixMono[y][x] = ColorFontSymbols[SymbolNumber][y][x];
        }
      }
    }

  //
  // Copy Led Array memory into Neon Pixel Array memory
  //
    void CopyLedArrayMemoryToNeonPixelArrayMemory() {
      for( uint8_t y = 0; y < MatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < MatrixWidth; x++) 
        {
          NeonPixelScreenMemoryMatrixMono[y][x] = LED_Array_Get_Pixel_Value(y,x);
        }
      }
    }

    //
    // Write one bit into monochrome Neon Pixel Array memory
    //
    void Neon_Pixel_Array_Matrix_Mono_Write(uint8_t y, uint8_t x, bool value) {
      NeonPixelScreenMemoryMatrixMono[y][x] = value;
    }
    //
    // Read one bit from monochrome LED Array memory
    //
    bool Neon_Pixel_Array_Matrix_Mono_Read(uint8_t y, uint8_t x) {
      return (NeonPixelScreenMemoryMatrixMono[y][x]);
    }
    //
    // Display Monochrome screen memory array on Neon Pixel array
    //
    void Neon_Pixel_Array_Matrix_Mono_Display() {
      uint8_t LEDPixelPosition = 0;
      uint8_t PixelBrightness = 0;
      for( uint8_t y = 0; y < MatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < MatrixWidth; x++) 
        {
          LEDPixelPosition = NeonScreenPixelPosition2DLUT [y][x];
          if (NeonPixelScreenMemoryMatrixMono[y][x])
          {
            PixelBrightness = NeonPixelBrightnessOn;
          }
          else 
          {
            PixelBrightness = NeonPixelBrightnessOff;
          }
          neonPixelMatrix.drawPixelin1DArray(LEDPixelPosition, PixelBrightness);
        }
      }
      neonPixelMatrix.display();
    }
  /*
    void Neon_Pixel_Array_Binary_Display() {
      uint8_t LEDPixelPosition = 0;
      for ( uint8_t ScreenPixel = 0; ScreenPixel < NUM_LEDS_C64; ScreenPixel++ ) {
        // Convert from screen position to LED array position 
        LEDPixelPosition = ScreenPixelPositionBinaryLUT [ScreenPixel];
        // Turn on or off the corresponding LED
        bool bitval = (LedArrayMemoryBinary >> ScreenPixel) & 0x0000000000000001 ;
        if ( bitval ) {
          leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
        }
        else {
          leds[LEDPixelPosition] = 0;
        }
      }
      Neon_Pixel_Array_Auto_Brightness();
      FastLED.show();
    }

    void Neon_Pixel_Array_Binary_Write(uint64_t BinaryValue){
      LedArrayMemoryBinary = BinaryValue;
    }

    uint64_t Neon_Pixel_Array_Binary_Read(){
      return (LedArrayMemoryBinary);
    }
  */
    void Neon_Pixel_Array_String_Write(uint8_t bit, bool value) {
      NeonPixelArrayMemoryString [bit] = value;
    }

    void Neon_Pixel_Array_Matrix_String_Display() {
      uint8_t NeonPixelPosition = 0;
      uint8_t PixelBrightness = 0;
      for ( uint8_t ScreenPixel = 0; ScreenPixel < NUM_PIXELS; ScreenPixel++ )
      {
        NeonPixelPosition = NeonPixelPosition1DLUT [ScreenPixel];
        if ( NeonPixelArrayMemoryString [ScreenPixel] ) 
        {
          PixelBrightness = NeonPixelBrightnessOn;
        }
        else 
        {
          PixelBrightness = NeonPixelBrightnessOff;
        }
        neonPixelMatrix.drawPixelin1DArray(NeonPixelPosition, PixelBrightness);
      }
      neonPixelMatrix.display();
    }

    void Neon_Pixel_Array_Binary_Write_Default() {
      NeonPixelArrayMemoryBinary = NeonPixelArrayMemoryBinaryDefault;
    }

    void Neon_Pixel_Array_Binary_To_Matrix_Mono() {        // TO DO: There is something wrong with Bit 31 (4,0) math because it is always set on if anything above it is on
      bool bitValue;
      uint8_t pixelPosition;
      uint8_t bitPosition = 0;
      uint64_t testValue;
      for( uint8_t y = 0; y < MatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < MatrixWidth; x++) 
        {
          pixelPosition = (y*8)+x; // row 0, column 0: 0 * 8 + 0 = 0
          bitPosition = 63 - pixelPosition; // Bit position 0 is lower right, Pixel position 0 (or 0,0) is upper left
          //if (bitPosition < 32)
          if (pixelPosition > 31)
          {
            testValue = 1 << bitPosition;       // This "1 << bitPosition" doesn't work beyond 32 bits in Arduino-land...
            bitValue = (NeonPixelArrayMemoryBinary & testValue);
            // TO DO: For bit 31 (4,0) the bitValue above calculates to 1 if anything above bit position 31 is set, even if 31 is zero.
            // So I added this special case handling. Need to figure out what is going wrong when 1<<31 should be 0b1000000000000000
            if ((y==4)&&(x==0)) {bitValue = (NeonPixelArrayMemoryBinary & 0b1000000000000000);}
          }
          else
          {
            testValue = 1 << (bitPosition-32);  // ...so the functionality is split at the 32 bit line...
            bitValue = ( (NeonPixelArrayMemoryBinary>>32) & testValue); // ...and it turns out >>32 shift works on a 64 bit number
          }
          Neon_Pixel_Array_Matrix_Mono_Write(y, x, bitValue);
        }
      }
    }

  /*
    void Neon_Pixel_Array_Matrix_Mono_to_Binary() {        
      uint64_t bitValue;
      uint8_t pixelPosition;
      uint8_t bitPosition = 0;
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          pixelPosition = (y*8)+x; // row 0, column 0: 0 * 8 + 0 = 0
          bitPosition = 63 - pixelPosition; // Bit position 0 is lower right, Pixel position 0 (or 0,0) is upper left
          bitValue = Neon_Pixel_Array_Matrix_Mono_Read(y, x);
          LedArrayMemoryBinary = (LedArrayMemoryBinary | (bitValue <<bitPosition));
        }
      }
    }

    void Neon_Pixel_Array_Test_Count_Binary() {
        static uint64_t BinaryValue = 0; // Tested to see what happens after 32 bits and 63 bits and it rolls over as expected.
        Neon_Pixel_Array_Monochrome_Set_Color(135,255,255);
        Neon_Pixel_Array_Memory_Clear();
        Neon_Pixel_Array_Binary_Write(BinaryValue);
        Neon_Pixel_Array_Binary_Display();
        BinaryValue++;
    }

    void Neon_Pixel_Array_Test_Pixel_String() {
        static uint8_t stringPos = 0;
        static unsigned long StringUpdatePeriodms = 50;  
        static unsigned long StringNowTime = 0;
        static unsigned long StringUpdateTimer = 0;
        StringNowTime = millis();
        if ((StringNowTime - StringUpdateTimer) >= StringUpdatePeriodms)
        {
          StringUpdateTimer = StringNowTime;
          Neon_Pixel_Array_Memory_Clear();
          Neon_Pixel_Array_String_Write(stringPos, 1);
          Neon_Pixel_Array_String_Display();
          stringPos++;
          if (stringPos>63) {stringPos=0;}
          #ifdef MONOCHROMECOLORCHANGER
            static uint8_t MonochromeColorChange = 0;
            Neon_Pixel_Array_Monochrome_Set_Color(MonochromeColorChange, 255, 255);
            MonochromeColorChange ++;
          #endif
        }
    }

    void Neon_Pixel_Array_Test_Pixel_Matrix_Mono() {
        static uint8_t Sequence = 0;
        static bool SequenceUpnDown = 1;
        static unsigned long UpdatePeriodms = 100;  
        static unsigned long NowTime = 0;
        static unsigned long UpdateTimer = 0;
        NowTime = millis();
        if ((NowTime - UpdateTimer) >= UpdatePeriodms)
        {
          UpdateTimer = NowTime;
          Neon_Pixel_Array_Memory_Clear();
            for(uint8_t y=0; y<8; y++)
              { 
              if(y == Sequence)
                {
                for(uint8_t x=0; x<8; x++)
                  {
                  Neon_Pixel_Array_Matrix_Mono_Write(Sequence, x, 1);
                  }
                }
                Neon_Pixel_Array_Matrix_Mono_Write(y, Sequence, 1);
              }
          // Neon_Pixel_Array_Matrix_Mono_Write(1, 1, 1); // works
          Neon_Pixel_Array_Matrix_Mono_Display();
          if (SequenceUpnDown)
          {
            Sequence++;
            if (Sequence==8)
            {
              SequenceUpnDown=0;
              Sequence=6;
            } 
          }
          else
          {
            Sequence--;
            if (Sequence==255)
            {
              SequenceUpnDown=1;
              Sequence=1;
            } 
          }
          #ifdef MONOCHROMECOLORCHANGER
            static uint8_t MonochromeColorChange = 0;
            Neon_Pixel_Array_Monochrome_Set_Color(MonochromeColorChange, 255, 255);
            MonochromeColorChange ++;
          #endif
       }
     }
  */

    void Neon_Pixel_Array_Init() {
      Serial.println("\n  Neon_Pixel_Array_Init started.");
      // Minimum configuration to display single pixels.
      neonPixelMatrix.setDisplayPixelSize(8,8); // Set the size of the viewable display area.
      neonPixelMatrix.setViewOrigin(0,8);
      neonPixelMatrix.begin();
      neonPixelMatrix.display();
    }

  #else // NEON_PIXEL_ARRAY
    void Neon_Pixel_Array_Binary_Write_Default() { }
    void Neon_Pixel_Array_Matrix_Mono_Display() { }
    void Neon_Pixel_Array_Matrix_String_Display() { }
    void Neon_Pixel_Array_Matrix_Mono_Write(uint8_t y, uint8_t x, bool value) { }  
    void Neon_Pixel_Array_String_Write(uint8_t bit, bool value) { }
    bool Neon_Pixel_Array_Matrix_Mono_Read(uint8_t y, uint8_t x) { }
    void CopyCoreMemoryToMonochromeNeonPixelArrayMemory() { }
    void Neon_Pixel_Array_Init() {
      Serial.println("\n  Neon_Pixel_Array_Init not started because NEON_PIXEL_ARRAY is not #DEFINED.");
    }

  #endif // NEON_PIXEL_ARRAY

#elif defined MCU_TYPE_RP2040

#endif
