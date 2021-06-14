/*!
 * @file Adafruit_ImageReader.cpp
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
 * Written by Phil "PaintYourDragon" Burgess for Adafruit Industries.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 */

#include "Adafruit_ImageReader.h"

// Buffers in BMP draw function (to screen) require 5 bytes/pixel: 3 bytes
// for each BMP pixel (R+G+B), 2 bytes for each TFT pixel (565 color).
// Buffers in BMP load (to canvas) require 3 bytes/pixel (R+G+B from BMP),
// no interim 16-bit buffer as data goes straight to the canvas buffer.
// Because buffers are flushed at the end of each scanline (to allow for
// cropping, vertical flip, scanline padding, etc.), no point in any of
// these pixel counts being more than the screen width.
// (Maybe to do: make non-AVR loader dynamically allocate buffer based
// on screen or image size.)

#ifdef __AVR__
#define BUFPIXELS 24 ///<  24 * 5 =  120 bytes
#else
#define BUFPIXELS 200 ///< 200 * 5 = 1000 bytes
#endif

// ADAFRUIT_IMAGE CLASS ****************************************************
// This has been created as a class here rather than in Adafruit_GFX because
// it's a new type returned specifically by the Adafruit_ImageReader class
// and needs certain flexibility not present in the latter's GFXcanvas*
// classes (having been designed for flash-resident bitmaps).

/*!
    @brief   Constructor.
    @return  'Empty' Adafruit_Image object.
*/
Adafruit_Image::Adafruit_Image(void)
    : mask(NULL), palette(NULL), format(IMAGE_NONE) {
  canvas.canvas1 = NULL;
}

/*!
    @brief   Destructor.
    @return  None (void).
*/
Adafruit_Image::~Adafruit_Image(void) { dealloc(); }

/*!
    @brief   Deallocates memory associated with Adafruit_Image object
             and resets member variables to 'empty' state.
    @return  None (void).
*/
void Adafruit_Image::dealloc(void) {
  if (format == IMAGE_1) {
    if (canvas.canvas1) {
      delete canvas.canvas1;
      canvas.canvas1 = NULL;
    }
  } else if (format == IMAGE_8) {
    if (canvas.canvas8) {
      delete canvas.canvas8;
      canvas.canvas8 = NULL;
    }
  } else if (format == IMAGE_16) {
    if (canvas.canvas16) {
      delete canvas.canvas16;
      canvas.canvas16 = NULL;
    }
  }
  if (mask) {
    delete mask;
    mask = NULL;
  }
  if (palette) {
    delete[] palette;
    palette = NULL;
  }
  format = IMAGE_NONE;
}

/*!
    @brief   Get width of Adafruit_Image object.
    @return  Width in pixels, or 0 if no image loaded.
*/
int16_t Adafruit_Image::width(void) const {
  if (format != IMAGE_NONE) { // Image allocated?
    if (format == IMAGE_1)
      return canvas.canvas1->width();
    else if (format == IMAGE_8)
      return canvas.canvas8->width();
    else if (format == IMAGE_16)
      return canvas.canvas16->width();
  }
  return 0;
}

/*!
    @brief   Get height of Adafruit_Image object.
    @return  Height in pixels, or 0 if no image loaded.
*/
int16_t Adafruit_Image::height(void) const {
  if (format != IMAGE_NONE) { // Image allocated?
    if (format == IMAGE_1)
      return canvas.canvas1->height();
    else if (format == IMAGE_8)
      return canvas.canvas8->height();
    else if (format == IMAGE_16)
      return canvas.canvas16->height();
  }
  return 0;
}

/*!
    @brief   Return pointer to image's GFX canvas object.
    @return  void* pointer, must be type-converted to a GFX canvas type
             consistent with the image's format (e.g. GFXcanvas16* if
             image format is IMAGE_16 -- use image.format() to determine
             the image format). Returns NULL if no canvas allocated.
    @note    Calling function must type-convert the result to one of the
             supported canvas object types, and must act accordingly with
             regard to calling functions on this object (e.g. doing the
             right thing with an 8- or 16-bit canvas, each has distinct
             drawing functions, things like that). This is here mostly to
             allow more advanced applications to get directly into an
             image's canvas object (and, in turn, its raw graphics buffer
             via canvas->getBuffer()) to move data in or out. Potential
             for a lot of mayhem here if used wrong.
*/
void *Adafruit_Image::getCanvas(void) const {
  if (format != IMAGE_NONE) { // Image allocated?
    if (format == IMAGE_1)
      return (void *)canvas.canvas1;
    else if (format == IMAGE_8)
      return (void *)canvas.canvas8;
    else if (format == IMAGE_16)
      return (void *)canvas.canvas16;
  }
  return NULL;
}

/*!
    @brief   Draw image to an Adafruit_SPITFT-type display.
    @param   tft
             Screen to draw to (any Adafruit_SPITFT-derived class).
    @param   x
             Horizontal offset in pixels; left edge = 0, positive = right.
             Value is signed, image will be clipped if all or part is off
             the screen edges. Screen rotation setting is observed.
    @param   y
             Vertical offset in pixels; top edge = 0, positive = down.
    @return  None (void).
*/
void Adafruit_Image::draw(Adafruit_SPITFT &tft, int16_t x, int16_t y) {
  if (format == IMAGE_1) {
    uint16_t foreground, background;
    if (palette) {
      foreground = palette[1];
      background = palette[0];
    } else {
      foreground = 0xFFFF;
      background = 0x0000;
    }
    tft.drawBitmap(x, y, canvas.canvas1->getBuffer(), canvas.canvas1->width(),
                   canvas.canvas1->height(), foreground, background);
  } else if (format == IMAGE_8) {
  } else if (format == IMAGE_16) {
    tft.drawRGBBitmap(x, y, canvas.canvas16->getBuffer(),
                      canvas.canvas16->width(), canvas.canvas16->height());
  }
}

// ADAFRUIT_IMAGEREADER CLASS **********************************************
// Loads images from SD card to screen or RAM.

/*!
    @brief   Constructor.
    @return  Adafruit_ImageReader object.
    @param   fs
             FAT filesystem associated with this Adafruit_ImageReader
             instance. Any images to load will come from this filesystem;
             if multiple filesystems are required, each will require its
             own Adafruit_ImageReader object. The filesystem does NOT need
             to be initialized yet when passed in here (since this will
             often be in pre-setup() declaration, but DOES need initializing
             before any of the image loading or size functions are called!
*/
Adafruit_ImageReader::Adafruit_ImageReader(FatFileSystem &fs) { filesys = &fs; }

/*!
    @brief   Destructor.
    @return  None (void).
*/
Adafruit_ImageReader::~Adafruit_ImageReader(void) {
  if (file)
    file.close();
  // filesystem is left as-is
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
    @param   transact
             Pass 'true' if TFT and SD are on the same SPI bus, in which
             case SPI transactions are necessary. If separate peripherals,
             can pass 'false'.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader::drawBMP(const char *filename,
                                              Adafruit_SPITFT &tft, int16_t x,
                                              int16_t y, boolean transact) {
  uint16_t tftbuf[BUFPIXELS]; // Temp space for buffering TFT data
  // Call core BMP-reading function, passing address to TFT object,
  // TFT working buffer, and X & Y position of top-left corner (image
  // will be cropped on load if necessary). Image pointer is NULL when
  // reading to TFT, and transact argument is passed through.
  return coreBMP(filename, &tft, tftbuf, x, y, NULL, transact);
}

/*!
    @brief   Loads BMP image file from SD card into RAM (as one of the GFX
             canvas object types) for use with the bitmap-drawing functions.
             Not practical for most AVR microcontrollers, but some of the
             more capable 32-bit micros can afford some RAM for this.
    @param   filename
             Name of BMP image file to load.
    @param   img
             Adafruit_Image object, contents will be initialized, allocated
             and loaded on success (else cleared).
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader::loadBMP(const char *filename,
                                              Adafruit_Image &img) {
  // Call core BMP-reading function. TFT and working buffer are NULL
  // (unused and allocated in function, respectively), X & Y position are
  // always 0 because full image is loaded (RAM permitting). Adafruit_Image
  // argument is passed through, and SPI transactions are not needed when
  // loading to RAM (bus is not shared during load).
  return coreBMP(filename, NULL, NULL, 0, 0, &img, false);
}

/*!
    @brief   BMP-reading function common both to the draw function (to TFT)
             and load function (to canvas object in RAM). BMP code has been
             centralized here so if/when more BMP format variants are added
             in the future, it doesn't need to be implemented, debugged and
             kept in sync in two places.
    @param   filename
             Name of BMP image file to load.
    @param   tft
             Pointer to TFT object, if loading to screen, else NULL.
    @param   dest
             Working buffer for loading 16-bit TFT pixel data, if loading to
             screen, else NULL.
    @param   x
             Horizontal offset in pixels (if loading to screen).
    @param   y
             Vertical offset in pixels (if loading to screen).
    @param   img
             Pointer to Adafruit_Image object, if loading to RAM (or NULL
             if loading to screen).
    @param   transact
             Use SPI transactions; 'true' is needed only if loading to screen
             and it's on the same SPI bus as the SD card. Other situations
             can use 'false'.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader::coreBMP(
    const char *filename, // SD file to load
    Adafruit_SPITFT *tft, // Pointer to TFT object, or NULL if to image
    uint16_t *dest,       // TFT working buffer, or NULL if to canvas
    int16_t x,            // Position if loading to TFT (else ignored)
    int16_t y,
    Adafruit_Image *img, // NULL if load-to-screen
    boolean transact) {  // SD & TFT sharing bus, use transactions

  ImageReturnCode status = IMAGE_ERR_FORMAT; // IMAGE_SUCCESS on valid file
  uint32_t offset;                           // Start of image data in file
  uint32_t headerSize;                       // Indicates BMP version
  int bmpWidth, bmpHeight;                   // BMP width & height in pixels
  uint8_t planes;                            // BMP planes
  uint8_t depth;                             // BMP bit depth
  uint32_t compression = 0;                  // BMP compression mode
  uint32_t colors = 0;                       // Number of colors in palette
  uint16_t *quantized = NULL;                // 16-bit 5/6/5 color palette
  uint32_t rowSize;                          // >bmpWidth if scanline padding
  uint8_t sdbuf[3 * BUFPIXELS];              // BMP read buf (R+G+B/pixel)
#if ((3 * BUFPIXELS) <= 255)
  uint8_t srcidx = sizeof sdbuf; // Current position in sdbuf
#else
  uint16_t srcidx = sizeof sdbuf;
#endif
  uint32_t destidx = 0;
  uint8_t *dest1 = NULL;     // Dest ptr for 1-bit BMPs to img
  boolean flip = true;       // BMP is stored bottom-to-top
  uint32_t bmpPos = 0;       // Next pixel position in file
  int loadWidth, loadHeight, // Region being loaded (clipped)
      loadX, loadY;          // "
  int row, col;              // Current pixel pos.
  uint8_t r, g, b;           // Current pixel color
  uint8_t bitIn = 0;         // Bit number for 1-bit data in
  uint8_t bitOut = 0;        // Column mask for 1-bit data out

  // If an Adafruit_Image object is passed and currently contains anything,
  // free its contents as it's about to be overwritten with new stuff.
  if (img)
    img->dealloc();

  // If BMP is being drawn off the right or bottom edge of the screen,
  // nothing to do here. NOT an error, just a trivial clip operation.
  if (tft && ((x >= tft->width()) || (y >= tft->height())))
    return IMAGE_SUCCESS;

  // Open requested file on SD card
  if (!(file = filesys->open(filename, FILE_READ))) {
    return IMAGE_ERR_FILE_NOT_FOUND;
  }

  // Parse BMP header. 0x4D42 (ASCII 'BM') is the Windows BMP signature.
  // There are other values possible in a .BMP file but these are super
  // esoteric (e.g. OS/2 struct bitmap array) and NOT supported here!
  if (readLE16() == 0x4D42) { // BMP signature
    (void)readLE32();         // Read & ignore file size
    (void)readLE32();         // Read & ignore creator bytes
    offset = readLE32();      // Start of image data
    // Read DIB header
    headerSize = readLE32();
    bmpWidth = readLE32();
    bmpHeight = readLE32();
    // If bmpHeight is negative, image is in top-down order.
    // This is not canon but has been observed in the wild.
    if (bmpHeight < 0) {
      bmpHeight = -bmpHeight;
      flip = false;
    }
    planes = readLE16();
    depth = readLE16(); // Bits per pixel
    // Compression mode is present in later BMP versions (default = none)
    if (headerSize > 12) {
      compression = readLE32();
      (void)readLE32();    // Raw bitmap data size; ignore
      (void)readLE32();    // Horizontal resolution, ignore
      (void)readLE32();    // Vertical resolution, ignore
      colors = readLE32(); // Number of colors in palette, or 0 for 2^depth
      (void)readLE32();    // Number of colors used (ignore)
      // File position should now be at start of palette (if present)
    }
    if (!colors)
      colors = 1 << depth;

    loadWidth = bmpWidth;
    loadHeight = bmpHeight;
    loadX = 0;
    loadY = 0;
    if (tft) {
      // Crop area to be loaded (if destination is TFT)
      if (x < 0) {
        loadX = -x;
        loadWidth += x;
        x = 0;
      }
      if (y < 0) {
        loadY = -y;
        loadHeight += y;
        y = 0;
      }
      if ((x + loadWidth) > tft->width())
        loadWidth = tft->width() - x;
      if ((y + loadHeight) > tft->height())
        loadHeight = tft->height() - y;
    }

    if ((planes == 1) && (compression == 0)) { // Only uncompressed is handled

      // BMP rows are padded (if needed) to 4-byte boundary
      rowSize = ((depth * bmpWidth + 31) / 32) * 4;

      if ((depth == 24) || (depth == 1)) { // BGR or 1-bit bitmap format

        if (img) {
          // Loading to RAM -- allocate GFX 16-bit canvas type
          status = IMAGE_ERR_MALLOC; // Assume won't fit to start
          if (depth == 24) {
            if ((img->canvas.canvas16 = new GFXcanvas16(bmpWidth, bmpHeight))) {
              dest = img->canvas.canvas16->getBuffer();
            }
          } else {
            if ((img->canvas.canvas1 = new GFXcanvas1(bmpWidth, bmpHeight))) {
              dest1 = img->canvas.canvas1->getBuffer();
            }
          }
          // Future: handle other depths.
        }

        if (dest || dest1) { // Supported format, alloc OK, etc.
          status = IMAGE_SUCCESS;

          if ((loadWidth > 0) && (loadHeight > 0)) { // Clip top/left
            if (tft) {
              tft->startWrite(); // Start SPI (regardless of transact)
              tft->setAddrWindow(x, y, loadWidth, loadHeight);
            } else {
              if (depth == 1) {
                img->format = IMAGE_1; // Is a GFX 1-bit canvas type
              } else {
                img->format = IMAGE_16; // Is a GFX 16-bit canvas type
              }
            }

            if ((depth >= 16) ||
                (quantized = (uint16_t *)malloc(colors * sizeof(uint16_t)))) {
              if (depth < 16) {
                // Load and quantize color table
                for (uint16_t c = 0; c < colors; c++) {
                  b = file.read();
                  g = file.read();
                  r = file.read();
                  (void)file.read(); // Ignore 4th byte
                  quantized[c] =
                      ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }
              }

              for (row = 0; row < loadHeight; row++) { // For each scanline...

                yield(); // Keep ESP8266 happy

                // Seek to start of scan line.  It might seem labor-intensive
                // to be doing this on every line, but this method covers a
                // lot of gritty details like cropping, flip and scanline
                // padding. Also, the seek only takes place if the file
                // position actually needs to change (avoids a lot of cluster
                // math in SD library).
                if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
                  bmpPos = offset + (bmpHeight - 1 - (row + loadY)) * rowSize;
                else // Bitmap is stored top-to-bottom
                  bmpPos = offset + (row + loadY) * rowSize;
                if (depth == 24) {
                  bmpPos += loadX * 3;
                } else {
                  bmpPos += loadX / 8;
                  bitIn = 7 - (loadX & 7);
                  bitOut = 0x80;
                  if (img)
                    destidx = ((bmpWidth + 7) / 8) * row;
                }
                if (file.position() != bmpPos) { // Need seek?
                  if (transact) {
                    tft->dmaWait();
                    tft->endWrite(); // End TFT SPI transaction
                  }
                  file.seek(bmpPos);     // Seek = SD transaction
                  srcidx = sizeof sdbuf; // Force buffer reload
                }
                for (col = 0; col < loadWidth; col++) { // For each pixel...
                  if (srcidx >= sizeof sdbuf) {         // Time to load more?
                    if (tft) {                          // Drawing to TFT?
                      if (transact) {
                        tft->dmaWait();
                        tft->endWrite(); // End TFT SPI transact
                      }
#if defined(ARDUINO_NRF52_ADAFRUIT)
                      // NRF52840 seems to have trouble reading more than 512
                      // bytes across certain boundaries. Workaround for now
                      // is to break the read into smaller chunks...
                      int32_t bytesToGo = sizeof sdbuf, bytesRead = 0,
                              bytesThisPass;
                      while (bytesToGo > 0) {
                        bytesThisPass = min(bytesToGo, 512);
                        file.read(&sdbuf[bytesRead], bytesThisPass);
                        bytesRead += bytesThisPass;
                        bytesToGo -= bytesThisPass;
                      }
#else
                      file.read(sdbuf, sizeof sdbuf); // Load from SD
#endif
                      if (transact)
                        tft->startWrite(); // Start TFT SPI transact
                      if (destidx) {       // If buffered TFT data
                        // Non-blocking writes (DMA) have been temporarily
                        // disabled until this can be rewritten with two
                        // alternating 'dest' buffers (else the nonblocking
                        // data out is overwritten in the dest[] write below).
                        // tft->writePixels(dest, destidx, false); // Write it
                        tft->writePixels(dest, destidx, true); // Write it
                        destidx = 0; // and reset dest index
                      }
                    } else {                          // Canvas is simpler,
                      file.read(sdbuf, sizeof sdbuf); // just load sdbuf
                    }                                 // (destidx never resets)
                    srcidx = 0;                       // Reset bmp buf index
                  }
                  if (depth == 24) {
                    // Convert each pixel from BMP to 565 format, save in dest
                    b = sdbuf[srcidx++];
                    g = sdbuf[srcidx++];
                    r = sdbuf[srcidx++];
                    dest[destidx++] =
                        ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                  } else {
                    // Extract 1-bit color index
                    uint8_t n = (sdbuf[srcidx] >> bitIn) & 1;
                    if (!bitIn) {
                      srcidx++;
                      bitIn = 7;
                    } else {
                      bitIn--;
                    }
                    if (tft) {
                      // Look up in palette, store in tft dest buf
                      dest[destidx++] = quantized[n];
                    } else {
                      // Store bit in canvas1 buffer (ignore palette)
                      if (n)
                        dest1[destidx] |= bitOut;
                      else
                        dest1[destidx] &= ~bitOut;
                      bitOut >>= 1;
                      if (!bitOut) {
                        bitOut = 0x80;
                        destidx++;
                      }
                    }
                  }
                }                // end pixel loop
                if (tft) {       // Drawing to TFT?
                  if (destidx) { // Any remainders?
                    // See notes above re: DMA
                    // tft->writePixels(dest, destidx, false); // Write it
                    tft->writePixels(dest, destidx, true); // Write it
                    destidx = 0; // and reset dest index
                  }
                  tft->dmaWait();
                  tft->endWrite(); // End TFT (regardless of transact)
                }
              } // end scanline loop

              if (quantized) {
                if (tft)
                  free(quantized); // Palette no longer needed
                else
                  img->palette = quantized; // Keep palette with img
              }
            } // end depth>24 or quantized malloc OK
          }   // end top/left clip
        }     // end malloc check
      }       // end depth check
    }         // end planes/compression check
  }           // end signature

  file.close();
  return status;
}

/*!
    @brief   Query pixel dimensions of BMP image file on SD card.
    @param   filename
             Name of BMP image file to query.
    @param   width
             Pointer to int32_t; image width in pixels, returned.
    @param   height
             Pointer to int32_t; image height in pixels, returned.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader::bmpDimensions(const char *filename,
                                                    int32_t *width,
                                                    int32_t *height) {

  ImageReturnCode status = IMAGE_ERR_FILE_NOT_FOUND; // Guilty until innocent

  if ((file = filesys->open(filename, FILE_READ))) { // Open requested file
    status = IMAGE_ERR_FORMAT;  // File's there, might not be BMP tho
    if (readLE16() == 0x4D42) { // BMP signature?
      (void)readLE32();         // Read & ignore file size
      (void)readLE32();         // Read & ignore creator bytes
      (void)readLE32();         // Read & ignore position of image data
      (void)readLE32();         // Read & ignore header size
      if (width)
        *width = readLE32();
      if (height) {
        int32_t h = readLE32(); // Don't abs() this, may be a macro
        if (h < 0)
          h = -h; // Do manually instead
        *height = h;
      }
      status = IMAGE_SUCCESS; // YAY.
    }
  }

  file.close();
  return status;
}

// UTILITY FUNCTIONS *******************************************************

/*!
    @brief   Reads a little-endian 16-bit unsigned value from currently-
             open File, converting if necessary to the microcontroller's
             native endianism. (BMP files use little-endian values.)
    @return  Unsigned 16-bit value, native endianism.
*/
uint16_t Adafruit_ImageReader::readLE16(void) {
#if !defined(ESP32) && !defined(ESP8266) &&                                    \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
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
#if !defined(ESP32) && !defined(ESP8266) &&                                    \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  // Read directly into result -- BMP data and variable both little-endian.
  uint32_t result;
  file.read(&result, sizeof result);
  return result;
#else
  // Big-endian or unknown. Byte-by-byte read will perform reversal if needed.
  return file.read() | ((uint32_t)file.read() << 8) |
         ((uint32_t)file.read() << 16) | ((uint32_t)file.read() << 24);
#endif
}

/*!
    @brief   Print human-readable status message corresponding to an
             ImageReturnCode type.
    @param   stat
             Numeric ImageReturnCode value.
    @param   stream
             Output stream (Serial default if unspecified).
    @return  None (void).
*/
void Adafruit_ImageReader::printStatus(ImageReturnCode stat, Stream &stream) {
  if (stat == IMAGE_SUCCESS)
    stream.println(F("Success!"));
  else if (stat == IMAGE_ERR_FILE_NOT_FOUND)
    stream.println(F("File not found."));
  else if (stat == IMAGE_ERR_FORMAT)
    stream.println(F("Not a supported BMP variant."));
  else if (stat == IMAGE_ERR_MALLOC)
    stream.println(F("Malloc failed (insufficient RAM)."));
}
