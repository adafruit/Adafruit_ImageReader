/*!
 * @file Adafruit_ImageReader.h
 *
 * This is part of Adafruit's ImageReader library for Arduino, designed to
 * work with Adafruit_GFX plus a display device-specific library.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Phil "PaintYourDragon" Burgess for Adafruit Industries.
 *
 * BSD license, all text here must be included in any redistribution.
 */

#include "Adafruit_SPITFT.h"

/** Status codes returned by drawBMP() and loadBMP() */
enum ImageReturnCode {
  IMAGE_SUCCESS,            // Successful load (or image clipped off screen)
  IMAGE_ERR_FILE_NOT_FOUND, // Could not open file
  IMAGE_ERR_FORMAT,         // Not a supported image format
  IMAGE_ERR_MALLOC          // Could not allocate image (loadBMP() only)
};

/** Image formats returned by loadBMP() */
enum ImageFormat {
  IMAGE_NONE,               // No image was loaded; IMAGE_ERR_* condition
  IMAGE_CANVAS1,            // GFXcanvas1 result (NOT YET SUPPORTED)
  IMAGE_CANVAS8,            // GFXcanvas8 result (NOT YET SUPPORTED)
  IMAGE_CANVAS16            // GFXcanvas16 result (SUPPORTED)
};

/*!
   @brief  An optional adjunct to Adafruit_SPITFT that reads RGB BMP
            images (maybe others in the future) from an SD card. It's
            purposefully been made an entirely separate class (rather than
            part of SPITFT or GFX classes) so that Arduino code that uses
            GFX or SPITFT *without* image loading does not need to incur
            the significant RAM overhead of the SD library by its mere
            inclusion. The syntaxes can therefore be a bit bizarre, see
            examples for use.
*/
class Adafruit_ImageReader {
  public:
    Adafruit_ImageReader(void);
    ~Adafruit_ImageReader(void);
    ImageReturnCode drawBMP(char *filename, Adafruit_SPITFT &display,
                      int16_t x, int16_t y);
    ImageReturnCode loadBMP(char *filename, void **data, ImageFormat *fmt);
  private:
    File            file;
    uint16_t        readLE16(void);
    uint32_t        readLE32(void);
};
