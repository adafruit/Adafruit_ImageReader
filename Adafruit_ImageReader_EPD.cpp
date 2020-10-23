#include "Adafruit_ImageReader_EPD.h"

#ifdef __AVR__
#define BUFPIXELS 24 ///<  24 * 5 =  120 bytes
#else
#define BUFPIXELS 200 ///< 200 * 5 = 1000 bytes
#endif

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
  if (format == IMAGE_1) {
    uint8_t *buffer = canvas.canvas1->getBuffer();
    uint8_t i, c;
    while (row < y + canvas.canvas1->height()) {
      for (i = 0; i < 8; i++) {
        if ((*buffer & (0x80 >> i)) > 0) {
          c = EPD_BLACK; // try to infer black
        } else {
          c = EPD_WHITE;
        }
        epd.writePixel(col, row, c);

        col++;
      }
      if (col >= x + canvas.canvas1->width()) {
        col = x;
        row++;
      }
      buffer++;
    };
  } else if (format == IMAGE_8) {
  } else if (format == IMAGE_16) {
    uint16_t *buffer = canvas.canvas16->getBuffer();
    while (row < y + canvas.canvas16->height()) {
      // RGB in 565 format
      uint8_t r = (*buffer & 0xf800) >> 8;
      uint8_t g = (*buffer & 0x07e0) >> 3;
      uint8_t b = (*buffer & 0x001f) << 3;

      uint8_t c = 0;
      if ((r < 0x60) && (g < 0x60) && (b < 0x60)) {
        c = EPD_BLACK; // try to infer black
      } else if ((r < 0x80) && (g < 0x80) && (b < 0x80)) {
        c = EPD_DARK; // try to infer dark gray
      } else if ((r < 0xD0) && (g < 0xD0) && (b < 0xD0)) {
        c = EPD_LIGHT; // try to infer light gray
      } else if ((r >= 0x80) && (g >= 0x80) && (b >= 0x80)) {
        c = EPD_WHITE;
      } else if (r >= 0x80) {
        c = EPD_RED; // try to infer red color
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

// ADAFRUIT_IMAGEREADER_EPD CLASS **********************************************
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
Adafruit_ImageReader_EPD::Adafruit_ImageReader_EPD(FatFileSystem &fs)
    : Adafruit_ImageReader(fs) {}

/*!
    @brief   Loads BMP image file from SD card directly to Adafruit_EPD screen.
    @param   filename
             Name of BMP image file to load.
    @param   epd
             Screen to draw to (any Adafruit_EPD-derived class).
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
ImageReturnCode Adafruit_ImageReader_EPD::drawBMP(char *filename,
                                                  Adafruit_EPD &epd, int16_t x,
                                                  int16_t y, boolean transact) {
  uint16_t epdbuf[BUFPIXELS]; // Temp space for buffering EPD data
  // Call core BMP-reading function, passing address to EPD object,
  // EPD working buffer, and X & Y position of top-left corner (image
  // will be cropped on load if necessary). Image pointer is NULL when
  // reading to EPD, and transact argument is passed through.
  return coreBMP(filename, &epd, epdbuf, x, y, NULL, transact);
}

/*!
    @brief   BMP-reading function common both to the draw function (to EPD)
             and load function (to canvas object in RAM). BMP code has been
             centralized here so if/when more BMP format variants are added
             in the future, it doesn't need to be implemented, debugged and
             kept in sync in two places.
    @param   filename
             Name of BMP image file to load.
    @param   epd
             Screen to draw to (any Adafruit_EPD-derived class). if loading to
             screen, else NULL.
    @param   dest
             Working buffer for loading 16-bit TFT pixel data, if loading to
             screen, else NULL.
    @param   x
             Horizontal offset in pixels (if loading to screen).
    @param   y
             Vertical offset in pixels (if loading to screen).
    @param   img
             Pointer to Adafruit_Image_EPD object, if loading to RAM (or NULL
             if loading to screen).
    @param   transact
             Use SPI transactions; 'true' is needed only if loading to screen
             and it's on the same SPI bus as the SD card. Other situations
             can use 'false'.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader_EPD::coreBMP(
    char *filename,    // SD file to load
    Adafruit_EPD *epd, // Pointer to TFT object, or NULL if to image
    uint16_t *dest,    // EPD working buffer, or NULL if to canvas
    int16_t x,         // Position if loading to EPD (else ignored)
    int16_t y,
    Adafruit_Image_EPD *img, // NULL if load-to-screen
    boolean transact) {      // SD & EPD sharing bus, use transactions

  ImageReturnCode status = IMAGE_ERR_FORMAT; // IMAGE_SUCCESS on valid file
  uint32_t offset;                           // Start of image data in file
  uint32_t headerSize;                       // Indicates BMP version
  int bmpWidth, bmpHeight;                   // BMP width & height in pixels
  uint8_t planes;                            // BMP planes
  uint8_t depth;                             // BMP bit depth
  uint32_t compression = 0;                  // BMP compression mode
  uint32_t colors = 0;                       // Number of colors in palette
  uint16_t *quantized = NULL;                // EPD Color palette
  uint32_t rowSize;                          // >bmpWidth if scanline padding
  uint8_t sdbuf[3 * BUFPIXELS];              // BMP read buf (R+G+B/pixel)
  int16_t epd_col, epd_row;
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
  uint8_t r, g, b, color;    // Current pixel color
  uint8_t bitIn = 0;         // Bit number for 1-bit data in
  uint8_t bitOut = 0;        // Column mask for 1-bit data out

  // If an Adafruit_Image object is passed and currently contains anything,
  // free its contents as it's about to be overwritten with new stuff.
  if (img)
    img->dealloc();

  // If BMP is being drawn off the right or bottom edge of the screen,
  // nothing to do here. NOT an error, just a trivial clip operation.
  if (epd && ((x >= epd->width()) || (y >= epd->height())))
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
    if (epd) {
      // Crop area to be loaded (if destination is EPD)
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
      if ((x + loadWidth) > epd->width())
        loadWidth = epd->width() - x;
      if ((y + loadHeight) > epd->height())
        loadHeight = epd->height() - y;
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
            if (epd) {
              epd->startWrite(); // Start SPI (regardless of transact)
              epd_col = x;
              epd_row = y;
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
                  color = 0;
                  if ((r < 0x60) && (g < 0x60) && (b < 0x60)) {
                    color = EPD_BLACK; // try to infer black
                  } else if ((r < 0x80) && (g < 0x80) && (b < 0x80)) {
                    color = EPD_DARK; // try to infer dark gray
                  } else if ((r < 0xD0) && (g < 0xD0) && (b < 0xD0)) {
                    color = EPD_LIGHT; // try to infer light gray
                  } else if ((r >= 0x80) && (g >= 0x80) && (b >= 0x80)) {
                    color = EPD_WHITE;
                  } else if (r >= 0x80) {
                    color = EPD_RED; // try to infer red color
                  }
                  quantized[c] = color;
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
                  if (img) {
                    destidx = ((bmpWidth + 7) / 8) * row;
                  }
                }
                if (file.position() != bmpPos) { // Need seek?
                  if (transact) {
                    epd->endWrite(); // End EPD SPI transaction
                  }
                  file.seek(bmpPos);     // Seek = SD transaction
                  srcidx = sizeof sdbuf; // Force buffer reload
                }
                for (col = 0; col < loadWidth; col++) { // For each pixel...
                  if (srcidx >= sizeof sdbuf) {         // Time to load more?
                    if (epd) {                          // Drawing to TFT?
                      if (transact) {
                        epd->endWrite(); // End EPD SPI transact
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
                        epd->startWrite(); // Start EPD SPI transact
                      if (destidx) {       // If buffered EPD data
                        // Non-blocking writes (DMA) have been temporarily
                        // disabled until this can be rewritten with two
                        // alternating 'dest' buffers (else the nonblocking
                        // data out is overwritten in the dest[] write below).
                        uint16_t index = 0;
                        while (index < destidx && epd_row < y + loadHeight) {
                          epd->writePixel(epd_col, epd_row, dest[index]);
                          epd_col++;
                          if (epd_col == x + loadWidth) {
                            epd_col = x;
                            epd_row++;
                          }
                          index++;
                        };
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

                    color = 0;
                    if ((r < 0x60) && (g < 0x60) && (b < 0x60)) {
                      color = EPD_BLACK; // try to infer black
                    } else if ((r < 0x80) && (g < 0x80) && (b < 0x80)) {
                      color = EPD_DARK; // try to infer dark gray
                    } else if ((r < 0xD0) && (g < 0xD0) && (b < 0xD0)) {
                      color = EPD_LIGHT; // try to infer light gray
                    } else if ((r >= 0x80) && (g >= 0x80) && (b >= 0x80)) {
                      color = EPD_WHITE;
                    } else if (r >= 0x80) {
                      color = EPD_RED; // try to infer red color
                    }
                    dest[destidx++] = color;
                  } else {
                    // Extract 1-bit color index
                    uint8_t n = (sdbuf[srcidx] >> bitIn) & 1;
                    if (!bitIn) {
                      srcidx++;
                      bitIn = 7;
                    } else {
                      bitIn--;
                    }
                    if (epd) {
                      // Look up in palette, store in epd dest buf
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
                if (epd) {       // Drawing to TFT?
                  if (destidx) { // Any remainders?
                    uint16_t index = 0;
                    while (index < destidx && epd_row < y + loadHeight) {
                      epd->writePixel(epd_col, epd_row, dest[index]);
                      epd_col++;
                      if (epd_col == x + loadWidth) {
                        epd_col = x;
                        epd_row++;
                      }
                      index++;
                    };
                    destidx = 0; // and reset dest index
                  }
                  epd->endWrite(); // End TFT (regardless of transact)
                }
              } // end scanline loop

              if (quantized) {
                if (epd)
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
