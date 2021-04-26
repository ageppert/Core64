
#include <Adafruit_GFX.h>


class NeonPixelMatrix : public Adafruit_GFX {
    public:
        NeonPixelMatrix(int16_t w, int16_t h);
        boolean begin();
        void drawPixel(int16_t x, int16_t y, uint16_t color);
        void drawPixelin1DArray(int16_t position, uint16_t color);
        void clear(void);
        uint8_t *getBuffer(void);
        void display(void);
        void setDisplayPixelSize(int16_t w, int16_t h);
        void setViewOrigin(int16_t x, int16_t y);

        ~NeonPixelMatrix(void);
    
    private:
        uint8_t *frameBuffer;
        uint8_t *displayBuffer;
        uint16_t pixelWidth = 8;
        uint16_t pixelHeight = 8;
        uint16_t viewOriginX = 0;
        uint16_t viewOriginY = 0;
};