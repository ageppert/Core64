#include <stdint.h>
// #include <stdbool.h> // Not required because FastLED library redefines bool.

#if (ARDUINO >= 100)
  #include <Arduino.h>
#else
  #include <WProgram.h>
#endif

#include "LED_Array_HAL.h"

#include "Config/HardwareIOMap.h"

#include "Hal/Core_HAL.h"       // ToDo This core_api shouldn't be directly accessed from this files. interaction should be through higher level application
#include "Config/CharacterMap.h"
#include "SubSystems/Ambient_Light_Sensor.h"

#if defined USE_FASTLED_LIBRARY
  #define FASTLED_ALLOW_INTERRUPTS 0    // include before #include FastLED.h to disable inetrrupts during writes.
  #include <FastLED.h>
  #include "Config/FastLED_Config.h"           // Core 64 Custom config file for FastLED library
  // FastLED.show() takes a little less than 2ms (measured) to update 64 LEDs. Good to have it delay 1/2 ms after core matrix twiddles the data pin.
#elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
  #include <Adafruit_NeoPixel.h>
  #include "Config/Adafruit_NeoPixel_Config.h"           // Core 64 Custom config file for Adafruit NeoPixel library
  #include "math.h"
#endif 

#define MONOCHROMECOLORCHANGER 1

static volatile bool LedArrayInitialized = false;
static uint8_t StartUpSymbolIndex = 0;
 const uint8_t SymbolSequenceArraySize = 3;
 const uint8_t SymbolSequenceArray[SymbolSequenceArraySize] = {7,8,9};
 const uint32_t StartUpSymbolSequenceUpdatePeriodms = 750;  

// LED Array Memory Buffers for user representations of the LED Array.
// Interaction with the abstract memory buffers which define the LED Array for the user to view:
// BINARY [64 bit data word, monochrome]
  // 0 is lower right (LSb), 63 is upper left (MSb), counting right to left, then up to the next row. Each row up is a higher Byte.
  static uint64_t LedArrayMemoryBinary = 0;
  static uint64_t LedArrayMemoryBinaryDefault = 0xDEADBEEFC0D3C4FE; // 0x8142241818244281; // "X" //  0xDEADBEEF and 0xC0D3C4FE
  static uint16_t LedArrayMemoryBinary16bit = 0;
  static uint16_t LedArrayMemoryBinaryDefault16bit = 0xC0D3C4FE; // 0x8142241818244281; // "X" //  0xDEADBEEF and 0xC0D3C4FE
// STRING [1D 64 pixel string, monochrome]
  // 0 is upper left, 63 is lower right, counting left to right, then down to next row
  static bool LedArrayMemoryString [64];  // for Core64
  static bool LedArrayMemoryString16bit [16];  // for Core16  
// MATRIX MONOCHROME [2D 8x8 pixel matrix, monochrome]
  // order y,x : 0,0 is upper left, 7,7 is lower right, counting left to right and top to bottom.
  static bool LedScreenMemoryMatrixMono [8][8];      
// MATRIX COLOR [2D 8x8 pixel matrix, 1 byte HUE and 1 optional SAT in HSV color space]
  static bool LedScreenMemoryMatrixColor [8][8];      
  // order y,x : 0,0 is upper left, 7,7 is lower right, counting left to right and top to bottom.
  // Exception for HSV color encoding is 0 which is interpreted as off, which will be substituted when displayed.
  // Exception for HSV color encoding is 255 which is interpreted as White, which will be substituted when displayed.
  static uint8_t LedScreenMemoryMatrixHue [8][8];
  static uint8_t LedScreenMemoryMatrixSat [8][8];

#if defined USE_FASTLED_LIBRARY
  // nothing to do here
#elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
  // These parameters are set in Adafruit_Neopizel_Config.h and from HardwareIOMap.h
  // Declare our NeoPixel strip object:
  Adafruit_NeoPixel strip(NUM_LEDS_C64, Pin_RGB_LED_Array, COLOR_ORDER + NEO_FREQUENCY);
  // Argument 1 = Number of pixels in NeoPixel strip
  // Argument 2 = Arduino pin number (most are valid)
  // Argument 3 = Pixel type flags, add together as needed:
  //   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
  //   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
  //   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
  //   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
  //   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
#endif

#ifdef CORE64_LED_MATRIX  // For Core64 LED Matrix layout (consecutive rows)
  // Look up tables to translate the 1D and 2D user representations of the array to the LED positions used by the LED Array Driver, FastLED.
  const uint8_t ScreenPixelPositionBinaryLUT [64] = { // Maps Screen Pixel Position to LED Binary Display position.
    63,62,61,60,59,58,57,56, 
    55,54,53,52,51,50,49,48, 
    47,46,45,44,43,42,41,40,
    39,38,37,36,35,34,33,32,
    31,30,29,28,27,26,25,24,
    23,22,21,20,19,18,17,16,
    15,14,13,12,11,10, 9, 8,
     7, 6, 5, 4, 3, 2, 1, 0  
    };
  const uint8_t ScreenPixelPosition1DLUT [64] = { // Maps Screen Pixel Position to LED 1D array position.
     0, 1, 2, 3, 4, 5, 6, 7,
     8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,
    24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,
    48,49,50,51,52,53,54,55,
    56,57,58,59,60,61,62,63
    };
  const uint8_t ScreenPixelPosition2DLUT [8][8] = { // Maps Screen Pixel Position to LED 2D array position.
    { 0, 1, 2, 3, 4, 5, 6, 7},
    { 8, 9,10,11,12,13,14,15},
    {16,17,18,19,20,21,22,23},
    {24,25,26,27,28,29,30,31},
    {32,33,34,35,36,37,38,39},
    {40,41,42,43,44,45,46,47},
    {48,49,50,51,52,53,54,55},
    {56,57,58,59,60,61,62,63}
    };
  // The following three are for Core16
  const uint8_t ScreenPixelPositionBinaryLUT16bit [16] = { // Maps Screen Pixel Position to LED Binary Display position.
    15,14,13,12,
    11,10, 9, 8,
     7, 6, 5, 4,
     3, 2, 1, 0  
    };
  const uint8_t ScreenPixelPosition1DLUT16bit [16] = { // Maps Screen Pixel Position to LED 1D array position.
     0, 1, 2, 3,
     4, 5, 6, 7,
     8, 9,10,11,
     12,13,14,15
    };
  const uint8_t ScreenPixelPosition2DLUT16bit [4][4] = { // Maps Screen Pixel Position to LED 2D array position.
    { 0, 1, 2, 3},
    { 4, 5, 6, 7},
    { 8, 9,10,11},
    {12,13,14,15}
    };
#else // For Pimoroni Unicorn Hat layout (serpentine)
  // Look up tables to translate the 1D and 2D user representations of the array to the LED positions used by the LED Array Driver, FastLED.
  const uint8_t ScreenPixelPositionBinaryLUT [64] = { // Maps Screen Pixel Position to LED Binary Display position.
    63,62,61,60,59,58,57,56, 
    48,49,50,51,52,53,54,55, 
    47,46,45,44,43,42,41,40,
    32,33,34,35,36,37,38,39,
    31,30,29,28,27,26,25,24,
    16,17,18,19,20,21,22,23,
    15,14,13,12,11,10, 9, 8,
     0, 1, 2, 3, 4, 5, 6, 7  
    };
  const uint8_t ScreenPixelPosition1DLUT [64] = { // Maps Screen Pixel Position to LED 1D array position.
     7, 6, 5, 4, 3, 2, 1, 0,
     8, 9,10,11,12,13,14,15,
    23,22,21,20,19,18,17,16,
    24,25,26,27,28,29,30,31,
    39,38,37,36,35,34,33,32,
    40,41,42,43,44,45,46,47,
    55,54,53,52,51,50,49,48,
    56,57,58,59,60,61,62,63
    };
  const uint8_t ScreenPixelPosition2DLUT [8][8] = { // Maps Screen Pixel Position to LED 2D array position.
    { 7, 6, 5, 4, 3, 2, 1, 0},
    { 8, 9,10,11,12,13,14,15},
    {23,22,21,20,19,18,17,16},
    {24,25,26,27,28,29,30,31},
    {39,38,37,36,35,34,33,32},
    {40,41,42,43,44,45,46,47},
    {55,54,53,52,51,50,49,48},
    {56,57,58,59,60,61,62,63}
    };
#endif

// Default monochrome color (135,255,255 = OLED aqua)
static uint8_t LEDArrayMonochromeColorHSV  [3] = {DEFAULTLEDArrayMonochromeColorH,DEFAULTLEDArrayMonochromeColorS,DEFAULTLEDArrayMonochromeColorV};             // Hue, Saturation, Value. Allowable range 0-255.
static uint8_t LEDArrayBrightness = BRIGHTNESS;

void LED_Array_Auto_Brightness() {
  #if defined  MCU_TYPE_MK20DX256_TEENSY_32
    if ( (LogicBoardTypeGet() == eLBT_CORE64_T32 ) && (HardwareVersionMajor >= 0) && (HardwareVersionMinor >= 4) )
  #elif defined MCU_TYPE_RP2040
  if (
    ( (LogicBoardTypeGet() == eLBT_CORE64_PICO ) && (HardwareVersionMajor >= 0) && (HardwareVersionMinor >= 7) ) ||
    ( (LogicBoardTypeGet() == eLBT_CORE64C_PICO ) && (HardwareVersionMajor >= 0) && (HardwareVersionMinor >= 2) )
  )  
  #endif
    {
      if(AmbientLightAvaible()==0) {LEDArrayBrightness = BRIGHTNESS;}
      else {LEDArrayBrightness = GetAmbientLightLevel8BIT();}
      if(LEDArrayBrightness < BRIGHTNESS_MIN) {LEDArrayBrightness = BRIGHTNESS_MIN;}
      if(LEDArrayBrightness > BRIGHTNESS_MAX) {LEDArrayBrightness = BRIGHTNESS_MAX;}
    }

    if ( (LogicBoardTypeGet() == eLBT_CORE16_PICO ) && (HardwareVersionMajor >= 0) && (HardwareVersionMinor >= 1) ) {
      LEDArrayBrightness = C16P_BRIGHTNESS_DEFAULT;
    }

    #if defined USE_FASTLED_LIBRARY
      FastLED.setBrightness( LEDArrayBrightness );
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      LEDArrayMonochromeColorHSV [2] = LEDArrayBrightness; // Neopixels use VALUE to change brightness through HSV
      strip.show();
    #endif

    // Serial.println(LEDArrayBrightness);
}

  void LED_Array_Set_Brightness(uint8_t brightness) {
      #if defined USE_FASTLED_LIBRARY
        FastLED.setBrightness( brightness );
      #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
        LEDArrayMonochromeColorHSV [2] = brightness; // Neopixels use VALUE to change brightness through HSV
        strip.show();
      #endif
  }

  void LED_Array_Memory_Clear() {
    uint8_t loopLength = 0;
    if (LogicBoardTypeGet() == eLBT_CORE16_PICO ) { loopLength = NUM_LEDS_C16; }
    else { loopLength = NUM_LEDS_C64; }

    LedArrayMemoryBinary = 0;
    for( uint8_t i = 0; i < loopLength; i++) {
      LedArrayMemoryString[i] = 0;
      LedArrayMemoryString16bit[i] = 0;
    }
    for( uint8_t y = 0; y < kMatrixHeight; y++) 
    {
      for( uint8_t x = 0; x < kMatrixWidth; x++) 
      {
        LedScreenMemoryMatrixMono[y][x] = 0;
        LedScreenMemoryMatrixHue[y][x] = 0;
      }
    }
  }

  uint8_t LED_Array_Get_Pixel_Value(uint8_t y, uint8_t x) {
    return LedScreenMemoryMatrixMono[y][x];
  }

  void LED_Array_Monochrome_Set_Color(uint8_t hue, uint8_t saturation, uint8_t value) {
    LEDArrayMonochromeColorHSV[0] = hue;
    LEDArrayMonochromeColorHSV[1] = saturation;
    #if defined USE_FASTLED_LIBRARY
      LEDArrayMonochromeColorHSV[2] = value;
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      LEDArrayMonochromeColorHSV [2] = LEDArrayBrightness; // Neopixels use VALUE to change brightness through HSV
    #endif 
  }

  void LED_Array_Monochrome_Increment_Color(uint8_t HueIncrement) {
    LEDArrayMonochromeColorHSV[0] = LEDArrayMonochromeColorHSV[0] + HueIncrement;
  }

  uint16_t XY( uint8_t x, uint8_t y)
  {
    uint16_t i;
    
    if( kMatrixSerpentineLayout == false) {
      i = (y * kMatrixWidth) + x;
    }

    if( kMatrixSerpentineLayout == true) {
      if( y & 0x01) {
        // Odd rows run backwards
        uint8_t reverseX = (kMatrixWidth - 1) - x;
        i = (y * kMatrixWidth) + reverseX;
      } else {
        // Even rows run forwards
        i = (y * kMatrixWidth) + x;
      }
    }
    
    return i;
  }

  uint16_t YX( uint8_t y, uint8_t x)
  {
    uint16_t i;
    
    if( kMatrixSerpentineLayout == false) {
      i = (y * kMatrixWidth) + x;
    }

    if( kMatrixSerpentineLayout == true) {
      if( y & 0x01) {
        // Odd rows run backwards
        uint8_t reverseX = (kMatrixWidth - 1) - x;
        i = (y * kMatrixWidth) + reverseX;
      } else {
        // Even rows run forwards
        i = (y * kMatrixWidth) + x;
      }
    }
    
    return i;
  }

  uint16_t XYsafe( uint8_t x, uint8_t y)
  {
    if( x >= kMatrixWidth) return -1;
    if( y >= kMatrixHeight) return -1;
    return XY(x,y);
  }

  void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
  {
    byte lineStartHue = startHue8;
    for( byte y = 0; y < kMatrixHeight; y++) {
      lineStartHue += yHueDelta8;
      byte pixelHue = lineStartHue;      
      for( byte x = 0; x < kMatrixWidth; x++) {
        pixelHue += xHueDelta8;
      #if defined USE_FASTLED_LIBRARY
        leds[ XY(x, y)]  = CHSV( pixelHue, 255, 255);
      #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
        strip.setPixelColor( (XY(x, y)), strip.ColorHSV( (int32_t)(pixelHue*256),255,LEDArrayBrightness)) ;  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
        //Serial.println(pixelHue);
      #endif 
      }
    }
  }

  void LED_Array_Test_Rainbow_Demo() {
    #if defined USE_FASTLED_LIBRARY
      uint32_t ms = millis();
      int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / kMatrixWidth));
      int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / kMatrixHeight));
      // Serial.print(xHueDelta32);
      // Serial.print(", ");
      // Serial.println(yHueDelta32);
      DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
      if( ms < 5000 ) {
        FastLED.setBrightness( scale8( BRIGHTNESS, (ms * 256) / 5000));
      } else {
        FastLED.setBrightness(BRIGHTNESS);
      }
      FastLED.show();

    // TO DO: This is broken and does not work right. Need to make it smooth like the FASTLED Rainbow Demo
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      uint32_t ms = millis();
      int32_t yHueDelta32 = 1565535*cos(ms/220);
      int32_t xHueDelta32 = 1532768*cos(ms/290);
      // Serial.print(xHueDelta32);
      // Serial.print(", ");
      // Serial.println(yHueDelta32);
      DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
      if( ms < 5000 ) {
        strip.setBrightness( ( BRIGHTNESS * ms / 8) );
      } 
      else {
        strip.setBrightness(BRIGHTNESS);
      }
      strip.show();
    #endif 
  }

  void LED_Array_Test_All_RGB(uint8_t color) {
    LED_Array_Monochrome_Set_Color(color,255,255);
    LED_Array_Memory_Clear();
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      for( uint8_t i = 0; i <= 15; i++) {
        LED_Array_String_Write(i,1);
      }
    }
    else {
      for( uint8_t i = 0; i <= 63; i++) {
        LED_Array_String_Write(i,1);
      }
    }
    LED_Array_String_Display();
  }

  //
  // Copy Core Memory Array bits into monochrome LED Array memory
  //
    void CopyCoreMemoryToMonochromeLEDArrayMemory() {
      if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
          for( uint8_t y = 0; y < 4; y++) {
            for( uint8_t x = 0; x < 4; x++) {
              LedScreenMemoryMatrixMono[y][x] = CoreArrayMemory[y][x];
              LedScreenMemoryMatrixMono[y][x] = CoreArrayMemory[y][x];
            }
          }
      }
      else {
          for( uint8_t y = 0; y < kMatrixHeight; y++) {
            for( uint8_t x = 0; x < kMatrixWidth; x++) {
              LedScreenMemoryMatrixMono[y][x] = CoreArrayMemory[y][x];
              LedScreenMemoryMatrixMono[y][x] = CoreArrayMemory[y][x];
            }
          }
      }
    }

  //
  // Copy Color Font Symbol into monochrome LED Array memory
  //
    void WriteCharacterMapToCoreMemoryArrayMemory() {
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          CoreArrayMemory[y][x] = ColorFontSymbols[0][y][x];
        }
      }
    }

  //
  // Copy Color Font Symbol into Color HSV LED Array memory
  //
    void WriteColorFontSymbolToLedScreenMemoryMatrixHue(uint8_t SymbolNumber) {
      if (LogicBoardTypeGet()==eLBT_CORE16_PICO) { 
        for( uint8_t y = 0; y < 4; y++) 
        {
          for( uint8_t x = 0; x < 4; x++) 
          {
            LedScreenMemoryMatrixHue[y][x] = ColorFontSymbols16bit[SymbolNumber][y][x];
          }
        }
      }
      else {
        for( uint8_t y = 0; y < kMatrixHeight; y++) 
        {
          for( uint8_t x = 0; x < kMatrixWidth; x++) 
          {
            LedScreenMemoryMatrixHue[y][x] = ColorFontSymbols[SymbolNumber][y][x];
          }
        }
      }
    }

  //
  // Copy Color Symbol into Color HSV LED Array memory
  //
    void WriteGameSnakeSymbol(uint8_t SymbolNumber){
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          LedScreenMemoryMatrixHue[y][x] = GameSnakeSymbols[SymbolNumber][y][x];
        }
      }
    }

  //
  // Copy Color Symbol into Color HSV LED Array memory
  //
    void WriteGamePongSymbol(uint8_t SymbolNumber){
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          LedScreenMemoryMatrixHue[y][x] = GamePongSymbols[SymbolNumber][y][x];
        }
      }
    }

  //
  // Copy Color Symbol into Color HSV LED Array memory
  //
    void WriteAppPaintSymbol(uint8_t SymbolNumber){
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          LedScreenMemoryMatrixHue[y][x] = AppPaintSymbolsHue[SymbolNumber][y][x];
          LedScreenMemoryMatrixSat[y][x] = AppPaintSymbolsSat[SymbolNumber][y][x];
        }
      }
    }

  //
  // Copy Color Symbol into Color HSV LED Array memory
  //
    void WriteUtilFluxSymbol(uint8_t SymbolNumber){
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          LedScreenMemoryMatrixHue[y][x] = UtilFluxSymbols[SymbolNumber][y][x];
        }
      }
    }

  //
  // Write the color Palette into the HSV LED Array Memory
  //
    void WriteAppPaintPalette(bool TopNBottom){
      if (TopNBottom){
        for( uint8_t y = 0; y < 2; y++) 
        {
          for( uint8_t x = 0; x < kMatrixWidth; x++) 
          {
            LedScreenMemoryMatrixHue[y][x] = AppPaintSymbolsHue[2][y][x];
            LedScreenMemoryMatrixSat[y][x] = AppPaintSymbolsSat[2][y][x];
          }
        }
      }
      else {
        for( uint8_t y = 6; y < 8; y++) 
        {
          for( uint8_t x = 0; x < kMatrixWidth; x++) 
          {
            LedScreenMemoryMatrixHue[y][x] = AppPaintSymbolsHue[1][y][x];
            LedScreenMemoryMatrixSat[y][x] = AppPaintSymbolsSat[1][y][x];
          }
        }
      }
    }

  //
  // Write one bit into monochrome LED Array memory
  //
  void LED_Array_Matrix_Mono_Write(uint8_t y, uint8_t x, bool value) {
    if ((x<kMatrixWidth)&&(y<kMatrixHeight)) {
      LedScreenMemoryMatrixMono[y][x] = value;
    }
    else {
      Serial.print("LED_Array_Matrix_Mono_Write overloaded. Y,X: ");
      Serial.print(y);
      Serial.print(",");
      Serial.print(x);
      Serial.println();
    }
  }

  //
  // Read one bit from monochrome LED Array memory
  //
  bool LED_Array_Matrix_Mono_Read(uint8_t y, uint8_t x) {
    return (LedScreenMemoryMatrixMono[y][x]);
  }

  //
  // Write one HUE COLOR bit into color LED Array memory
  //
  void LED_Array_Matrix_Color_Hue_Write(uint8_t y, uint8_t x, uint8_t hue) {
    if ((x<kMatrixWidth)&&(y<kMatrixHeight)) {
      LedScreenMemoryMatrixHue[y][x] = hue;
    }
    else {
      Serial.print("LED_Array_Matrix_Color_Hue_Write overloaded. Y,X: ");
      Serial.print(y);
      Serial.print(",");
      Serial.print(x);
      Serial.println();
    }
  }

  //
  // Write one HUE and one SAT COLOR bit into color LED Array memory
  //
  void LED_Array_Matrix_Color_Hue_Sat_Write(uint8_t y, uint8_t x, uint8_t hue, uint8_t sat) {
    if ((x<kMatrixWidth)&&(y<kMatrixHeight)) {
      LedScreenMemoryMatrixHue[y][x] = hue;
      LedScreenMemoryMatrixSat[y][x] = sat;
    }
    else {
      Serial.print("LED_Array_Matrix_Color_Hue_Sat_Write overloaded. Y,X: ");
      Serial.print(y);
      Serial.print(",");
      Serial.print(x);
      Serial.println();
    }
  }

  //
  // Write one COLOR bit into color LED Array memory using hue and saturation
  //
  void LED_Array_Matrix_Color_Write(uint8_t y, uint8_t x, uint8_t hue, uint8_t sat) {
    if ((x<kMatrixWidth)&&(y<kMatrixHeight)) {
      LedScreenMemoryMatrixColor[y][x] = hue;
    }
    else {
      Serial.print("LED_Array_Matrix_Color_Write overloaded. Y,X: ");
      Serial.print(y);
      Serial.print(",");
      Serial.print(x);
      Serial.println();
    }
  }

  void LED_Array_Matrix_Mono_Display() {
    uint8_t LEDPixelPosition = 0;
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      for( uint8_t y = 0; y < 4; y++) 
      {
        for( uint8_t x = 0; x < 4; x++) 
        {
          LEDPixelPosition = ScreenPixelPosition2DLUT16bit [y][x];
          // LED HUE COLOR
          if ( ((LedScreenMemoryMatrixMono [y][x]) >= 1) || ((LedScreenMemoryMatrixMono [y][x]) <= 255) ) {
            #if defined USE_FASTLED_LIBRARY
              leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
            #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( (LEDArrayMonochromeColorHSV[0]*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
            #endif
          }
          // LED OFF
          if ((LedScreenMemoryMatrixMono [y][x]) == 0) {
            #if defined USE_FASTLED_LIBRARY
              leds[LEDPixelPosition] = 0;
            #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
              strip.setPixelColor( LEDPixelPosition, strip.Color( 0, 0, 0) );
            #endif
          }
        }
      }
    }
    else {
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          LEDPixelPosition = ScreenPixelPosition2DLUT [y][x];
          // LED HUE COLOR
          if ( ((LedScreenMemoryMatrixMono [y][x]) >= 1) || ((LedScreenMemoryMatrixMono [y][x]) <= 255) ) {
            #if defined USE_FASTLED_LIBRARY
              leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
            #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( (LEDArrayMonochromeColorHSV[0]*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
            #endif
          }
          // LED OFF
          if ((LedScreenMemoryMatrixMono [y][x]) == 0) {
            #if defined USE_FASTLED_LIBRARY
              leds[LEDPixelPosition] = 0;
            #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
              strip.setPixelColor( LEDPixelPosition, strip.Color( 0, 0, 0) );
            #endif
          }
        }
      }
    }
    LED_Array_Auto_Brightness();
    #if defined USE_FASTLED_LIBRARY
      FastLED.show();
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      strip.show();
    #endif
  }

  void LED_Array_Color_Display(bool HuenHueSat) {
    uint8_t LEDPixelPosition = 0;
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      for( uint8_t y = 0; y < 4; y++) 
      {
        for( uint8_t x = 0; x < 4; x++) 
        {
          LEDPixelPosition = ScreenPixelPosition2DLUT16bit [y][x];          
          #if defined USE_FASTLED_LIBRARY
            leds[LEDPixelPosition] = CHSV(LedScreenMemoryMatrixHue [y][x],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);          // leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
            // LED WHITE special case
            if ((LedScreenMemoryMatrixHue [y][x]) == 255) {
              leds[LEDPixelPosition] = CHSV(LedScreenMemoryMatrixHue [y][x], 0 ,LEDArrayMonochromeColorHSV[2]);
            }
          #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
            if (HuenHueSat) {
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( ((LedScreenMemoryMatrixHue [y][x])*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
            }
            else {
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( ((LedScreenMemoryMatrixHue [y][x])*256),(LedScreenMemoryMatrixSat [y][x]),LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
            }
            // LED WHITE special case
            if ((LedScreenMemoryMatrixHue [y][x]) == 255) {
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( ((LedScreenMemoryMatrixHue [y][x])*256),0,LEDArrayMonochromeColorHSV[2]) );
            }
          #endif
          // Exception is color of 0 which is implemented as pixel OFF, and not the 0 color in HSV space.
          if(LedScreenMemoryMatrixHue [y][x]==0)
          {
            #if defined USE_FASTLED_LIBRARY
              leds[LEDPixelPosition] = 0;
            #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( 0, 0, 0) );
            #endif
          }
        }
      }
    }
    else {
      for( uint8_t y = 0; y < kMatrixHeight; y++) 
      {
        for( uint8_t x = 0; x < kMatrixWidth; x++) 
        {
          LEDPixelPosition = ScreenPixelPosition2DLUT [y][x];
          #if defined USE_FASTLED_LIBRARY
            leds[LEDPixelPosition] = CHSV(LedScreenMemoryMatrixHue [y][x],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);          // leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
            // LED WHITE special case
            if ((LedScreenMemoryMatrixHue [y][x]) == 255) {
              leds[LEDPixelPosition] = CHSV(LedScreenMemoryMatrixHue [y][x], 0 ,LEDArrayMonochromeColorHSV[2]);
            }
          #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
            if (HuenHueSat) {
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( ((LedScreenMemoryMatrixHue [y][x])*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
            }
            else {
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( ((LedScreenMemoryMatrixHue [y][x])*256),(LedScreenMemoryMatrixSat [y][x]),LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
            }
            // LED WHITE special case
            if ((LedScreenMemoryMatrixHue [y][x]) == 255) {
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( ((LedScreenMemoryMatrixHue [y][x])*256),0,LEDArrayMonochromeColorHSV[2]) );
            }
          #endif
          // Exception is color of 0 which is implemented as pixel OFF, and not the 0 color in HSV space.
          if(LedScreenMemoryMatrixHue [y][x]==0)
          {
            #if defined USE_FASTLED_LIBRARY
              leds[LEDPixelPosition] = 0;
            #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
              strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( 0, 0, 0) );
            #endif
          }
        }
      }
    }
    LED_Array_Auto_Brightness();
    #if defined USE_FASTLED_LIBRARY
      FastLED.show();
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      strip.show();
    #endif
  }

  void LED_Array_Binary_Display() {
    uint8_t LEDPixelPosition = 0;
    for ( uint8_t ScreenPixel = 0; ScreenPixel < NUM_LEDS_C64; ScreenPixel++ ) {
      // Convert from screen position to LED array position 
      LEDPixelPosition = ScreenPixelPositionBinaryLUT [ScreenPixel];
      // Turn on or off the corresponding LED
      bool bitval = (LedArrayMemoryBinary >> ScreenPixel) & 0x0000000000000001 ;
      if ( bitval ) {
        #if defined USE_FASTLED_LIBRARY
          leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
        #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
          strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( (LEDArrayMonochromeColorHSV[0]*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
        #endif
      }
      else {
        #if defined USE_FASTLED_LIBRARY
          leds[LEDPixelPosition] = 0;
        #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
          strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( 0, 0, 0) );
        #endif
      }
    }
    LED_Array_Auto_Brightness();
    #if defined USE_FASTLED_LIBRARY
      FastLED.show();
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      strip.show();
    #endif
  }

  void LED_Array_Binary_Write(uint64_t BinaryValue){
    LedArrayMemoryBinary = BinaryValue;
  }

  uint64_t LED_Array_Binary_Read(){
    return (LedArrayMemoryBinary);
  }

  void LED_Array_String_Write(uint8_t bit, bool value) {
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      LedArrayMemoryString16bit [bit] = value;
    }
    else {
      LedArrayMemoryString [bit] = value;
    }
  }

  void LED_Array_String_Display() {
    uint8_t LEDPixelPosition = 0;
    if (LogicBoardTypeGet()==eLBT_CORE16_PICO) {
      for ( uint8_t ScreenPixel = 0; ScreenPixel < 16; ScreenPixel++ ) {
        // Convert from screen position to LED array position 
        LEDPixelPosition = ScreenPixelPosition1DLUT16bit [ScreenPixel];
        // Turn on or off the corresponding LED
        if ( LedArrayMemoryString16bit [ScreenPixel] ) {
          #if defined USE_FASTLED_LIBRARY
            leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
          #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
            strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( (LEDArrayMonochromeColorHSV[0]*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
          #endif
        }
        else {
          #if defined USE_FASTLED_LIBRARY
            leds[LEDPixelPosition] = 0;
          #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
            strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( 0, 0, 0) );
          #endif
        }
      }
    }
    else {
      for ( uint8_t ScreenPixel = 0; ScreenPixel < NUM_LEDS_C64; ScreenPixel++ ) {
        // Convert from screen position to LED array position 
        LEDPixelPosition = ScreenPixelPosition1DLUT [ScreenPixel];
        // Turn on or off the corresponding LED
        if ( LedArrayMemoryString [ScreenPixel] ) {
          #if defined USE_FASTLED_LIBRARY
            leds[LEDPixelPosition] = CHSV(LEDArrayMonochromeColorHSV[0],LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]);
          #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
            strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( (LEDArrayMonochromeColorHSV[0]*256),LEDArrayMonochromeColorHSV[1],LEDArrayMonochromeColorHSV[2]) );  //  Set pixel's color (in RAM) pixel #, hue, saturation, brightness
          #endif
        }
        else {
          #if defined USE_FASTLED_LIBRARY
            leds[LEDPixelPosition] = 0;
          #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
            strip.setPixelColor( LEDPixelPosition, strip.ColorHSV( 0, 0, 0) );
          #endif
        }
      }
    }
    // LED_Array_Auto_Brightness();
    #if defined USE_FASTLED_LIBRARY
      FastLED.show();
    #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
      strip.show();
    #endif
  }

  void LED_Array_Binary_Write_Default() {
    LedArrayMemoryBinary = LedArrayMemoryBinaryDefault;
  }

  void LED_Array_Binary_To_Matrix_Mono() {        // TO DO: There is something wrong with Bit 31 (4,0) math because it is always set on if anything above it is on
    bool bitValue;
    uint8_t pixelPosition;
    uint8_t bitPosition = 0;
    uint64_t testValue;
    for( uint8_t y = 0; y < kMatrixHeight; y++) 
    {
      for( uint8_t x = 0; x < kMatrixWidth; x++) 
      {
        pixelPosition = (y*8)+x; // row 0, column 0: 0 * 8 + 0 = 0
        bitPosition = 63 - pixelPosition; // Bit position 0 is lower right, Pixel position 0 (or 0,0) is upper left
        //if (bitPosition < 32)
        if (pixelPosition > 31)
        {
          testValue = 1 << bitPosition;       // This "1 << bitPosition" doesn't work beyond 32 bits in Arduino-land...
          bitValue = (LedArrayMemoryBinary & testValue);
          // TO DO: For bit 31 (4,0) the bitValue above calculates to 1 if anything above bit position 31 is set, even if 31 is zero.
          // So I added this special case handling. Need to figure out what is going wrong when 1<<31 should be 0b1000000000000000
          if ((y==4)&&(x==0)) {bitValue = (LedArrayMemoryBinary & 0b1000000000000000);}
        }
        else
        {
          testValue = 1 << (bitPosition-32);  // ...so the functionality is split at the 32 bit line...
          bitValue = ( (LedArrayMemoryBinary>>32) & testValue); // ...and it turns out >>32 shift works on a 64 bit number
        }
        LED_Array_Matrix_Mono_Write(y, x, bitValue);
      }
    }
  }

  void LED_Array_Matrix_Mono_to_Binary() {        
    uint64_t bitValue;
    uint8_t pixelPosition;
    uint8_t bitPosition = 0;
    for( uint8_t y = 0; y < kMatrixHeight; y++) 
    {
      for( uint8_t x = 0; x < kMatrixWidth; x++) 
      {
        pixelPosition = (y*8)+x; // row 0, column 0: 0 * 8 + 0 = 0
        bitPosition = 63 - pixelPosition; // Bit position 0 is lower right, Pixel position 0 (or 0,0) is upper left
        bitValue = LED_Array_Matrix_Mono_Read(y, x);
        LedArrayMemoryBinary = (LedArrayMemoryBinary | (bitValue <<bitPosition));
      }
    }
  }

  void LED_Array_Test_Count_Binary() {
      static uint64_t BinaryValue = 0; // Tested to see what happens after 32 bits and 63 bits and it rolls over as expected.
      LED_Array_Monochrome_Set_Color(135,255,255);
      LED_Array_Memory_Clear();
      LED_Array_Binary_Write(BinaryValue);
      LED_Array_Binary_Display();
      BinaryValue++;
  }

  void LED_Array_Test_Pixel_String() {
      static uint8_t stringPos = 0;
      static uint8_t stringLength = 0;
      static unsigned long StringUpdatePeriodms = 50;  
      static unsigned long StringNowTime = 0;
      static unsigned long StringUpdateTimer = 0;
      StringNowTime = millis();
      if (LogicBoardTypeGet()==eLBT_CORE16_PICO) { stringLength = 16; }
      else { stringLength = 64; }
      if ((StringNowTime - StringUpdateTimer) >= StringUpdatePeriodms)
      {
        StringUpdateTimer = StringNowTime;
        LED_Array_Memory_Clear();
        LED_Array_String_Write(stringPos, 1);
        LED_Array_String_Display();
        stringPos++;
        if (stringPos>=stringLength) {stringPos=0;}
        #ifdef MONOCHROMECOLORCHANGER
          static uint8_t MonochromeColorChange = 0;
          LED_Array_Monochrome_Set_Color(MonochromeColorChange, 255, 255);
          MonochromeColorChange ++;
        #endif
      }
  }

  void LED_Array_Test_Pixel_Matrix_Mono() {
      static uint8_t Sequence = 0;
      static bool SequenceUpnDown = 1;
      static unsigned long UpdatePeriodms = 100;  
      static unsigned long NowTime = 0;
      static unsigned long UpdateTimer = 0;
      NowTime = millis();
      if ((NowTime - UpdateTimer) >= UpdatePeriodms)
      {
        UpdateTimer = NowTime;
        LED_Array_Memory_Clear();
          for(uint8_t y=0; y<8; y++)
            { 
            if(y == Sequence)
              {
              for(uint8_t x=0; x<8; x++)
                {
                LED_Array_Matrix_Mono_Write(Sequence, x, 1);
                }
              }
              LED_Array_Matrix_Mono_Write(y, Sequence, 1);
            }
        // LED_Array_Matrix_Mono_Write(1, 1, 1); // works
        LED_Array_Matrix_Mono_Display();
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
          LED_Array_Monochrome_Set_Color(MonochromeColorChange, 255, 255);
          MonochromeColorChange ++;
        #endif
     }
   }


  // Set-up first symbol to cycle from in start-up mode.
  void LED_Array_Start_Up_Symbol_Loop_Begin() {
      StartUpSymbolIndex = 0;
      LED_Array_Memory_Clear();
      WriteColorFontSymbolToLedScreenMemoryMatrixHue(SymbolSequenceArray[StartUpSymbolIndex]);
      LED_Array_Color_Display(1);
  }

  // Cycles through symbols dedicated to start-up mode.
  void LED_Array_Start_Up_Symbol_Loop_Continue() {
      static unsigned long NowTime = 0;
      static unsigned long UpdateTimer = 0;
      NowTime = millis();
      if ((NowTime - UpdateTimer) >= StartUpSymbolSequenceUpdatePeriodms)
      {
        UpdateTimer = NowTime;
        LED_Array_Memory_Clear();
        WriteColorFontSymbolToLedScreenMemoryMatrixHue(SymbolSequenceArray[StartUpSymbolIndex]);
        LED_Array_Color_Display(1);
        StartUpSymbolIndex++;
        if(StartUpSymbolIndex >= SymbolSequenceArraySize){StartUpSymbolIndex=0;}
      }
  }

  // Cycles through available multi-color font symbols
  void LED_Array_Test_Pixel_Matrix_Color() {
      static uint8_t FontSymbolNumber = 7;
      static unsigned long UpdatePeriodms = 1500;  
      static unsigned long NowTime = 0;
      static unsigned long UpdateTimer = 0;
      NowTime = millis();
      if ((NowTime - UpdateTimer) >= UpdatePeriodms)
      {
        UpdateTimer = NowTime;
        LED_Array_Memory_Clear();
        WriteColorFontSymbolToLedScreenMemoryMatrixHue(FontSymbolNumber);
        LED_Array_Color_Display(1);
        FontSymbolNumber++;
        if(FontSymbolNumber>15){FontSymbolNumber=7;}
      }
  }

  void LED_Array_Init() {
    if (LedArrayInitialized == false) {
      #if defined USE_FASTLED_LIBRARY
        // These parameters are set in FastLED_Config.h and from HardwareIOMap.h
        FastLED.addLeds<CHIPSET, Pin_RGB_LED_Array, COLOR_ORDER>(leds, NUM_LEDS_C64).setCorrection(TypicalSMD5050);
        FastLED.setBrightness( BRIGHTNESS );
      #elif defined USE_ADAFRUIT_NEOPIXEL_LIBRARY
        strip.begin();
        strip.setBrightness( 255 ); // Neopixel brightness set to 255 once in set up, and dimming is done via HSV with the VALUE during usage
        if ( (LogicBoardTypeGet() == eLBT_CORE16_PICO ) && (HardwareVersionMajor >= 0) && (HardwareVersionMinor >= 1) ) {
          LEDArrayBrightness = C16P_BRIGHTNESS_DEFAULT;
          LEDArrayMonochromeColorHSV [2] = LEDArrayBrightness; // Neopixels use VALUE to change brightness through HSV
        }
        strip.show();
      #endif
      LedArrayInitialized = true;
    }
    LED_Array_Memory_Clear();
    delay(25);
    LED_Array_Color_Display(1);
  }
