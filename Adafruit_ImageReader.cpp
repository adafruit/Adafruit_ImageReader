#include <SD.h>
#include "Adafruit_ImageReader.h"

// Buffers in BMP draw function (to screen) require 5 bytes/pixel: 3 bytes
// for each BMP pixel (R+G+B), 2 bytes for each TFT pixel (565 color).
// Buffers in BMP load (to canvas) require 3 bytes/pixel (R+G+B from BMP),
// no interim 16-bit buffer as data goes straight to the canvas buffer.
#ifdef __AVR__
 #define DRAWPIXELS  24 ///<  24 * 5 =  120 bytes
 #define LOADPIXELS  32 ///<  32 * 3 =   96 bytes
#else
 #define DRAWPIXELS 200 ///< 200 * 5 = 1000 bytes
 #define LOADPIXELS 320 ///< 320 * 3 =  960 bytes
#endif

/*!
    @brief   Constructor.
    @return  Adafruit_ImageReader object.
*/
Adafruit_ImageReader::Adafruit_ImageReader(void) {
}

/*!
    @brief   Destructor.
    @return  None (void).
*/
Adafruit_ImageReader::~Adafruit_ImageReader(void) {
  if(file) file.close();
}

/*!
    @brief   Loads BMP image file from SD card directly to SPITFT screen.
    @param   filename
             Name of BMP image file to load.
    @param   tft
             Adafruit_SPITFT object (e.g. one of the Adafruit TFT or OLED
             displays that subclass Adafruit_SPITFT).
    @param   x
             Horizontal offset in pixels; left edge = 0, positive = right.
             Value is signed, image will be clipped if all or part is off
             the screen edges. Screen rotation setting is observed.
    @param   y
             Vertical offset in pixels; top edge = 0, positive = down.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader::drawBMP(
  char *filename, Adafruit_SPITFT &tft, int16_t x, int16_t y) {

  ImageReturnCode status = IMAGE_ERR_FORMAT; // IMAGE_SUCCESS on valid header
  uint32_t        offset;                    // Start of image data in file
  int             width, height;             // BMP width & height in pixels
  uint8_t         depth;                     // BMP bit depth
  uint32_t        rowSize;                   // >width if scanline padding
  uint8_t         sdbuf[3*DRAWPIXELS];       // BMP pixel buf (R+G+B per pixel)
  uint16_t        tftbuf[DRAWPIXELS];        // TFT pixel buf (16bpp)
#if ((3*DRAWPIXELS) <= 255)
  uint8_t         bufidx = sizeof sdbuf;     // Current position in sdbuf
#else
  uint16_t        bufidx = sizeof sdbuf;
#endif
#if (DRAWPIXELS <= 255)
  uint8_t         tftidx = 0;
#else
  uint16_t        tftidx = 0;
#endif
  boolean         flip   = true;             // BMP is stored bottom-to-top
  uint32_t        pos    = 0;                // Next pixel position in file
  int             w, h, row, col;            // Region being loaded
  uint8_t         r, g, b;                   // Current pixel color

  // If BMP is being drawn off the right or bottom edge of the screen,
  // nothing to do here. NOT an error, just a trivial clip operation.
  if((x >= tft.width()) || (y >= tft.height())) return IMAGE_SUCCESS;

  // Open requested file on SD card
  if(!(file = SD.open(filename))) return IMAGE_ERR_FILE_NOT_FOUND;

  // Parse BMP header
  if(readLE16() == 0x4D42) { // BMP signature
    (void)readLE32();        // Read & ignore file size
    (void)readLE32();        // Read & ignore creator bytes
    offset = readLE32();     // Start of image data
    // Read DIB header
    (void)readLE32();        // Read & ignore header size
    width  = readLE32();
    height = readLE32();
    if(readLE16() == 1) {    // # planes -- currently must be '1'
      depth = readLE16();    // bits per pixel
      if((depth == 24) && (readLE32() == 0)) { // Uncompressed BGR only

        status = IMAGE_SUCCESS; // Supported BMP format -- proceed!

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (width * 3 + 3) & ~3;

        // If height is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(height < 0) {
          height = -height;
          flip   =  false;
        }

        // Crop area to be loaded
        w = width;
        h = height;
        if(x < 0) {
          w += x;
          x  = 0;
        }
        if(y < 0) {
          h += y;
          y  = 0;
        }
        if((x + w) > tft.width())  w = tft.width()  - x;
        if((y + h) > tft.height()) h = tft.height() - y;
        if((w > 0) && (h > 0)) { // Clip top/left
          tft.startWrite();              // Start new TFT SPI transaction
          tft.setAddrWindow(x, y, w, h); // Window = clipped image bounds

          for(row=0; row<h; row++) { // For each scanline...

            yield(); // Keep ESP8266 happy

            // Seek to start of scan line.  It might seem labor-intensive to
            // be doing this on every line, but this method covers a lot of
            // gritty details like cropping, flip and scanline padding.  Also,
            // the seek only takes place if the file position actually needs
            // to change (avoids a lot of cluster math in SD library).
            if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = offset + (height - 1 - row) * rowSize;
            else     // Bitmap is stored top-to-bottom
              pos = offset + row * rowSize;
            if(file.position() != pos) { // Need seek?
              tft.endWrite();        // End TFT SPI transaction
              file.seek(pos);        // SD transaction
              bufidx = sizeof sdbuf; // Force buffer reload
            }
            for(col=0; col<w; col++) { // For each pixel...
              if(bufidx >= sizeof sdbuf) {      // Time to load more data?
                tft.endWrite();                 // End TFT SPI transaction
                file.read(sdbuf, sizeof sdbuf); // SD transaction
                tft.startWrite();               // Start new TFT SPI transac
                if(tftidx) {                    // If any buffered TFT data
                  tft.writePixels(tftbuf, tftidx); // Write it now and
                  tftidx = 0;                      // reset tft buf index
                }
                bufidx = 0;                     // Reset bmp buf index
              }
              // Convert pixel from BMP to TFT format, save in tft buf
              b = sdbuf[bufidx++];
              g = sdbuf[bufidx++];
              r = sdbuf[bufidx++];
              tftbuf[tftidx++] = tft.color565(r, g, b);
            } // end pixel
            if(tftidx) { // Any remainders?
              tft.writePixels(tftbuf, tftidx);
              tftidx = 0;
            }
            tft.endWrite();
          } // end scanline
        } // end top/left clip
      } // end format
    } // end planes
  } // end signature

  file.close();
  return status;
}

/*!
    @brief   Loads BMP image file from SD card into RAM (as one of the GFX
             canvas object types) for use with the bitmap-drawing functions.
             Not practical for most AVR microcontrollers, but some of the
             more capable 32-bit micros can afford some RAM for this.
    @param   filename
             Name of BMP image file to load.
    @param   data
             A a canvas object vector, which type can be determined from the
             value returned in the third argument. (Currently will return
             only NULL or a GFXcanvas16, cast to a void* pointer).
    @param   fmt
             Pointer to an ImageFormat variable, which will indicate the
             canvas type that resulted from the load operation. Currently
             provides only IMAGE_NONE (load error, data pointer will be
             NULL) or IMAGE_CANVAS16 (success, data pointer can be cast to
             a GFXcanvas16* type, from which the buffer, width and height
             can be queried with other GFX functions).
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader::loadBMP(
  char *filename, void **data, ImageFormat *fmt) {

  GFXcanvas16    *canvas;                    // Result goes here
  ImageReturnCode status = IMAGE_ERR_FORMAT; // IMAGE_SUCCESS on valid header
  uint32_t        offset;                    // Start of image data in file
  int             width, height;             // BMP width & height in pixels
  uint8_t         depth;                     // BMP bit depth
  uint32_t        rowSize;                   // >width if scanline padding
  uint8_t         sdbuf[3*LOADPIXELS];       // Pixel buffer (R+G+B per pixel)
#if ((3*LOADPIXELS) <= 255)
  uint8_t         bufidx = sizeof sdbuf;     // Current position in sdbuf
#else
  uint16_t        bufidx = sizeof sdbuf;
#endif
  boolean         flip   = true;             // BMP is stored bottom-to-top
  uint32_t        pos    = 0;                // Next pixel position in file
  int             row, col;                  // Region being loaded
  uint8_t         r, g, b;                   // Current pixel color
  uint16_t       *image, *ptr;               // 16-bit image data

  *data = NULL;
  *fmt  = IMAGE_NONE;

  // Open requested file on SD card
  if(!(file = SD.open(filename))) return IMAGE_ERR_FILE_NOT_FOUND;

  // Parse BMP header
  if(readLE16() == 0x4D42) { // BMP signature
    (void)readLE32();        // Read & ignore file size
    (void)readLE32();        // Read & ignore creator bytes
    offset = readLE32();     // Start of image data
    // Read DIB header
    (void)readLE32();        // Read & ignore header size
    width  = readLE32();
    height = readLE32();
    if(readLE16() == 1) {    // # planes -- currently must be '1'
      depth = readLE16();    // bits per pixel
      if((depth == 24) && (readLE32() == 0)) { // Uncompressed BGR only

        if((canvas = new GFXcanvas16(width, height))) {

          status = IMAGE_SUCCESS; // Format OK, malloc OK, proceed!
          ptr    = canvas->getBuffer();

          // BMP rows are padded (if needed) to 4-byte boundary
          rowSize = (width * 3 + 3) & ~3;

          // If height is negative, image is in top-down order.
          // This is not canon but has been observed in the wild.
          if(height < 0) {
            height = -height;
            flip   =  false;
          }

          for(row=0; row<height; row++) { // For each scanline...
            yield(); // Keep ESP8266 happy
            // Seek to start of scan line.  It might seem labor-intensive to
            // be doing this on every line, but this method covers details
            // like flip and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change (avoids a
            // lot of cluster math in SD library).
            if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = offset + (height - 1 - row) * rowSize;
            else     // Bitmap is stored top-to-bottom
              pos = offset + row * rowSize;
            if(file.position() != pos) { // Need seek?
              file.seek(pos);        // SD transaction
              bufidx = sizeof sdbuf; // Force buffer reload
            }
            for(col=0; col<width; col++) { // For each pixel...
              if(bufidx >= sizeof sdbuf) {      // Time to load more data?
                file.read(sdbuf, sizeof sdbuf); // Load data
                bufidx = 0;                     // Reset bmp buf index
              }
              // Convert pixel from BMP to 565 format, store in RAM
              b      = sdbuf[bufidx++];
              g      = sdbuf[bufidx++];
              r      = sdbuf[bufidx++];
              *ptr++ = ((r & 0xF8) << 8) |
                       ((g & 0xFC) << 3) |
                       ((b & 0xF8) >> 3);
            } // end pixel
          } // end scanline
          *data = canvas;         // Successful read
          *fmt  = IMAGE_CANVAS16; // Is a GFX 16-bit canvas type
        } else {
          status = IMAGE_ERR_MALLOC;
        } // end alloc
      } // end format
    } // end planes
  } // end signature

  file.close();
  return status;
}

/*!
    @brief   Reads a little-endian 16-bit unsigned value from currently-
             open File, converting if necessary to the microcontroller's
             native endianism. (BMP files use little-endian values.)
    @return  Unsigned 16-bit value, native endianism.
*/
uint16_t Adafruit_ImageReader::readLE16(void) {
#if !defined(ESP32) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  // Read directly into result -- BMP data and variable both little-endian.
  uint16_t result;
  file.read(&result, sizeof result);
  return result;
#else
  // Big-endian or unknown. Byte-by-byte read will perform reversal if needed.
  return file.read() | ((uint16_t)file.read() << 8);
#endif
}

/*!
    @brief   Reads a little-endian 32-bit unsigned value from currently-
             open File, converting if necessary to the microcontroller's
             native endianism. (BMP files use little-endian values.)
    @return  Unsigned 32-bit value, native endianism.
*/
uint32_t Adafruit_ImageReader::readLE32(void) {
#if !defined(ESP32) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  // Read directly into result -- BMP data and variable both little-endian.
  uint32_t result;
  file.read(&result, sizeof result);
  return result;
#else
  // Big-endian or unknown. Byte-by-byte read will perform reversal if needed.
  return       file.read()        |
    ((uint32_t)file.read() <<  8) |
    ((uint32_t)file.read() << 16) |
    ((uint32_t)file.read() << 24);
#endif
}
