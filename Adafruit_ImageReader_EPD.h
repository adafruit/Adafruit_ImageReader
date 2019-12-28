/*!
 * @file Adafruit_ImageReader_EPD.h
 *
 * This is part of Adafruit's ImageReader library for Arduino, designed to
 * work with Adafruit_GFX plus a display device-specific library.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Melissa LeBlanc-Williams for Adafruit Industries.
 *
 * BSD license, all text here must be included in any redistribution.
 */
#ifndef __ADAFRUIT_IMAGE_READER_EPD_H__
#define __ADAFRUIT_IMAGE_READER_EPD_H__

#include "Adafruit_EPD.h"
#include "Adafruit_ImageReader.h"

/*!
   @brief  Data bundle returned with an image loaded to RAM. Used by
           ImageReader.loadBMP() and Image.draw(), not ImageReader.drawBMP().
*/
class Adafruit_Image_EPD : public Adafruit_Image {
public:
  void draw(Adafruit_EPD &epd, int16_t x, int16_t y);

protected:
  friend class Adafruit_ImageReader; ///< Loading occurs here
};

#endif // __ADAFRUIT_IMAGE_READER_EPD_H__
