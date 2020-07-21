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
  friend class Adafruit_ImageReader_EPD; ///< Loading occurs here
};

/*!
   @brief  An optional adjunct to Adafruit_EPD that reads RGB BMP
           images (maybe others in the future) from a flash filesystem
           (SD card or SPI/QSPI flash). It's purposefully been made an
           entirely separate class (rather than part of SPITFT or GFX
           classes) so that Arduino code that uses GFX or SPITFT *without*
           image loading does not need to incur the RAM overhead and
           additional dependencies of the Adafruit_SPIFlash library by
           its mere inclusion. The syntaxes can therefore be a bit
           bizarre (passing display object as an argument), see examples
           for use.
*/
class Adafruit_ImageReader_EPD : public Adafruit_ImageReader {
public:
  Adafruit_ImageReader_EPD(FatFileSystem &fs);
  ImageReturnCode drawBMP(char *filename, Adafruit_EPD &epd, int16_t x,
                          int16_t y, boolean transact = true);

private:
  ImageReturnCode coreBMP(char *filename, Adafruit_EPD *epd, uint16_t *dest,
                          int16_t x, int16_t y, Adafruit_Image_EPD *img,
                          boolean transact);
};

#endif // __ADAFRUIT_IMAGE_READER_EPD_H__
