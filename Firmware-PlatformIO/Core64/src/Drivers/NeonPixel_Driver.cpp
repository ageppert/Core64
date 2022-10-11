
#include "Config/HardwareIOMap.h"
#if defined BOARD_CORE64_TEENSY_32

    #include "Drivers/NeonPixel_Driver.h"
    #include <Adafruit_GFX.h>
    #include <SPI.h>

    // Teensy 3.2
    #define CLOCKPIN                  13
    #define DATA_OUT                  11 
    #define DATA_IN                   12
    #define CHIP_SELECT                8 // already available as Pin_SPI_LCD_CS in HardwareIOMap.
    #define NEON_PIXEL_SPI_BIT_MODE   MSBFIRST
    #define NEON_PIXEL_SPI_POLARITY   SPI_MODE0
    static const int spiClk        =  380000; 
    /* Neon Pixel Notes
        Only DATA OUT and CLOCK are used to control the Neon Pixels.
        SPI DATA IN is not used because there is no return line from the Neon Pixels.
        CHIP SELECT is not used because it doesn't exist on the Neon Pixels.
        There is no incoming SPI buffer reset - it is always alive so ANY movement of the SPI CLK will cause the pixels to interpret SPI DAT as useful data.
        They should work reliably at 380kHz.
        MSB first. First byte of data is the last pixel in the string. Last byte of data is the first pixel in the string.
        Full brightness is 0x7F or greater.
        2022-10-07 Troubleshooting and FIX!
            Andy's Neon Pixel configuration (Core64 LB V0.4 w/ Teensy 3.2) is (and always has been) showing signs of the data being shifted 
            so that the pixel left of the commanded pixel is lighting up to various degrees.
            Two problems fixed:
            1) The LED on the Teensy is shared with the SPI CLK and was used for heart beat. During setup of heart beat, the the SPI CLK is
               toggled and the Neon Pixels take that in as data. But the incoming data buffer is not reset [after a timeout] so the next
               "real" ends up being offset from there. Disable the LED Heart Beat if Teensy is used!
            2) There was an extra NULL byte I was sending in the FOR loop that sends the SPI data out. That was causing the Neon Pixel to the
               left to be addressed instead of the intended one.  
    */

    NeonPixelMatrix::NeonPixelMatrix(int16_t w, int16_t h) : 
        Adafruit_GFX(w, h) {
        pinMode(CHIP_SELECT, OUTPUT);
        SPI.setMOSI(DATA_OUT);
        SPI.setMISO(DATA_IN);
        SPI.setSCK(CLOCKPIN);
        SPI.begin();                  //   <<<--- THE MISSING KEY TO MAKING THE setCLK assignment work!!!
        Serial.println("Initialized Neon Pixel Matrix.");
    }

    boolean NeonPixelMatrix::begin() {
        if((!frameBuffer) && !(frameBuffer = (uint8_t *)malloc(WIDTH * HEIGHT))) {
            return false;
        }

        if((!displayBuffer) && !(displayBuffer = (uint8_t *)malloc(pixelWidth * pixelHeight))) {
            return false;
        }

        clear();
        display();

        return true;
    }

    void NeonPixelMatrix::setDisplayPixelSize(int16_t w, int16_t h){
        pixelHeight = h;
        pixelWidth = w;
    }

    void NeonPixelMatrix::setViewOrigin(int16_t x, int16_t y){
        viewOriginX = x;
        viewOriginY = y;
    }

    void NeonPixelMatrix::clear(void) {
        for (int i=0; i<(WIDTH * HEIGHT); i++) {
            frameBuffer[i] = 0x00;
        }
    }

    void NeonPixelMatrix::drawPixel(int16_t x, int16_t y, uint16_t color) {
        // Serial.printf("x=%d, y=%d, c=%d\n", x, y, color);
        if (x>=WIDTH || y>=HEIGHT) return;
        if (x<0 || y<0) return;
        frameBuffer[ (y*WIDTH)+x ] = color;
    }

    void NeonPixelMatrix::drawPixelin1DArray(int16_t position, uint16_t color) {
        if (position >= (WIDTH*HEIGHT)) return;
        if (position < 0) return;
        frameBuffer[ position ] = color;

    }

    void NeonPixelMatrix::display() {
        uint16_t i=0;
        uint8_t  dataToSend;
        SPI.beginTransaction(SPISettings(spiClk, NEON_PIXEL_SPI_BIT_MODE, NEON_PIXEL_SPI_POLARITY));   
        for(int x=viewOriginX; x<viewOriginX+pixelWidth; x++) {
            for(int y=viewOriginY; y<viewOriginY+pixelHeight; y++){
              dataToSend = frameBuffer[((y%HEIGHT)*WIDTH) + (x%WIDTH)];
              SPI.transfer(dataToSend);
            }
        }
        SPI.endTransaction();
    }

    uint8_t *NeonPixelMatrix::getBuffer() { return frameBuffer; }

    NeonPixelMatrix::~NeonPixelMatrix(void) {
        if(frameBuffer) {
            free(frameBuffer);
            frameBuffer = NULL;
        }
        if (displayBuffer) {
            free(displayBuffer);
            displayBuffer = NULL;
        }
    }

#elif defined BOARD_CORE64C_RASPI_PICO

#endif
