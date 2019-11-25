/*!
 * @file Adafruit_ImageReader_EPD.cpp
 *
 * @mainpage Companion library for Adafruit_GFX to load images from SD card.
 *           Load-to-display and load-to-RAM are supported.
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's ImageReader library for the
 * Arduino platform. It is designed to work in conjunction with Adafruit_GFX
 * and a display-specific library (e.g. Adafruit_ILI9341).
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on
 * <a href="https://github.com/adafruit/Adafruit_GFX">Adafruit_GFX</a>
 * plus a display device-specific library such as
 * <a href="https://github.com/adafruit/Adafruit_ILI9341">Adafruit_ILI9341</a>
 * or other subclasses of SPITFT. Filesystem reading is handled through the
 * <a href="https://github.com/adafruit/Adafruit_SPIFlash">Adafruit_SPIFlash</a>
 * library, which in turn relies on
 * <a href="https://github.com/adafruit/SdFat">SdFat</a>.
 * Please make sure you have installed the latest versions before
 * using this library.
 *
 * @section author Author
 *
 * Written by Melissa LeBlanc-Williams for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 */

#include "Adafruit_ImageReader_EPD.h"

/*!
    @brief   Draw image to an Adafruit ePaper-type display.
    @param   epd
             Screen to draw to (any Adafruit_EPD-derived class).
    @param   x
             Horizontal offset in pixels; left edge = 0, positive = right.
             Value is signed, image will be clipped if all or part is off
             the screen edges. Screen rotation setting is observed.
    @param   y
             Vertical offset in pixels; top edge = 0, positive = down.
    @return  None (void).
*/
void Adafruit_Image_EPD::draw(Adafruit_EPD &epd, int16_t x, int16_t y) {
  int16_t col = x, row = y;
  if(format == IMAGE_1 ) {
    uint8_t *buffer = canvas.canvas1->getBuffer();
    uint8_t i, c;
    while(row < y + canvas.canvas1->height()) {
        for (i = 0; i < 8; i++) {
            if ((*buffer & (0x80 >> i)) > 0) {
               c = EPD_BLACK; // try to infer black
            } else {
               c = EPD_WHITE;
            }
            epd.writePixel(col, row, c);
            
            col++;
            if (col == x + canvas.canvas1->width()) {
              col = x;
              row++;
            }
        }
        buffer++;
    };
  } else if(format == IMAGE_8 ) {
  } else if(format == IMAGE_16) {
    uint16_t *buffer = canvas.canvas16->getBuffer();
    while(row < y + canvas.canvas16->height()) {
        // RGB in 565 format
        uint8_t r = (*buffer & 0xf800) >> 8;
        uint8_t g = (*buffer & 0x07e0) >> 3;
        uint8_t b = (*buffer & 0x001f) << 3;
      
        uint8_t c = 0;
        if ((r < 0x80) && (g < 0x80) && (b < 0x80)) {
           c = EPD_BLACK; // try to infer black
        } else if ((r >= 0x80) && (g >= 0x80) && (b >= 0x80)) {
           c = EPD_WHITE;
        } else if (r >= 0x80) {
           c = EPD_RED; //try to infer red color
        }
        
        epd.writePixel(col, row, c);
        col++;
        if (col == x + canvas.canvas16->width()) {
          col = x;
          row++;
        }
        buffer++;
    };
  }
}
