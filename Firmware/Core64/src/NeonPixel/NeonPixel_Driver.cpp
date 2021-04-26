
// #include "HardwareIOMap.h"
#include <Adafruit_GFX.h>
#include "NeonPixel_Driver.h"
#include <SPI.h>

// Teensy 3.2
#define CLOCKPIN        13
#define DATA_OUT        11
#define DATA_IN         12
#define CHIP_SELECT      8 // already available as Pin_SPI_LCD_CS in HardwareIOMap

static const int spiClk =  480000; 

NeonPixelMatrix::NeonPixelMatrix(int16_t w, int16_t h) : 
    Adafruit_GFX(w, h) {

    pinMode(CHIP_SELECT, OUTPUT);

    SPI.setMOSI(DATA_OUT);
    SPI.setMISO(DATA_IN);
    SPI.setSCK(CLOCKPIN);
    SPI.begin();                  //   <<<--- THE MISSING KEY TO MAKING THE setCLK assignment work!!!

    Serial.print("\nInitializing Neon Pixel Matrix...");

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

    SPI.setSCK(CLOCKPIN);
    SPI.begin();                  //   <<<--- THE MISSING KEY TO MAKING THE setCLK assignment work!!!

    digitalWriteFast(CHIP_SELECT, 0);
    SPI.beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));   
    for(int x=viewOriginX; x<viewOriginX+pixelWidth; x++) {
        for(int y=viewOriginY; y<viewOriginY+pixelHeight; y++){
          dataToSend = frameBuffer[((y%HEIGHT)*WIDTH) + (x%WIDTH)];
          SPI.transfer(dataToSend);
        }
    }
    SPI.endTransaction();
    digitalWriteFast(CHIP_SELECT, 1);
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