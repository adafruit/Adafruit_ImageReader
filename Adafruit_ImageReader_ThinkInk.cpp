#include "Adafruit_ImageReader_ThinkInk.h"

#ifdef __AVR__
#define BUFPIXELS 24 ///<  24 * 5 =  120 bytes
#else
#define BUFPIXELS 200 ///< 200 * 5 = 1000 bytes
#endif

// Palettes are weird but will be needed for error diffusion dithering later.
// Also, the quantize function references the tricolor palette.
static const uint8_t
    palette_mono[][4] = {{47 >> 3, 36 >> 2, 41 >> 3, EPD_BLACK},
                         {242 >> 3, 244 >> 2, 239 >> 3, EPD_WHITE}},
    palette_tricolor[][4] = {{47 >> 3, 36 >> 2, 41 >> 3, EPD_BLACK},
                             {242 >> 3, 244 >> 2, 239 >> 3, EPD_WHITE},
                             {215 >> 3, 38 >> 2, 39 >> 3, EPD_RED}},
    palette_grayscale4[][4] = {{47 >> 3, 36 >> 2, 41 >> 3, EPD_BLACK},
                               {112 >> 3, 105 >> 2, 107 >> 3, EPD_DARK},
                               {177 >> 3, 175 >> 2, 173 >> 3, EPD_LIGHT},
                               {242 >> 3, 244 >> 2, 239 >> 3, EPD_WHITE}};

// Convert RGB565 color to closest match for EPD display type

// Should probably return the palette index but not the EPD color.
// That way the calling function can look up the RGB color for
// error diffusion.
static uint8_t quantize(uint16_t rgb, thinkinkmode_t mode) {
  uint8_t r = rgb >> 11;
  uint8_t g = (rgb >> 5) & 0x3F;
  uint8_t b = rgb & 0x1F;
  if (mode == THINKINK_MONO) {
    return (uint8_t)((r * 631 + g * 611 + b * 241) >> 15); // 0 or 1
  } else if (mode == THINKINK_GRAYSCALE4) {
    return (uint8_t)((r * 631 + g * 611 + b * 241) >> 14); // 0 to 3
  } else { // THINKINK_TRICOLOR
    // For the moment, doing a brute-force compare against each color
    // in the tricolor palette, returning the index of the closest match
    // (using distance in linear RGB space for comparison).
    uint8_t i, closest_index = 0;
    uint32_t closest_dist = 0xFFFFFFFF;
    for (i = 0; i < sizeof palette_tricolor / sizeof palette_tricolor[0]; i++) {
      int8_t dr = (r - palette_tricolor[i][0]) * 2; // Red dist
      int8_t dg = g - palette_tricolor[i][1];       // Green dist
      int8_t db = (b - palette_tricolor[i][2]) * 2; // Blue dist
      uint32_t dist = dr * dr + dg * dg + db * db;  // Dist^2
      if (dist < closest_dist) { // No sqrt needed, because relative compare
        closest_dist = dist;
        closest_index = i;
      }
    }
    return closest_index;
  }
}

// Draw span of pixels from source buffer to EPD display, handling
// quantization and dithering as requested. Clipping is already handled in
// calling function; coordinates can safely be assumed fully in-image at
// this point. This ONLY does a horizontal span, not a 2D rect. Input data
// will ALWAYS be 16-bit RGB565 at this point (bitmaps have been expandex).

static void span(uint16_t *src, Adafruit_EPD *epd, int16_t x, int16_t y,
                 int16_t width, thinkinkmode_t mode, dither_t dither) {

  uint8_t *palette;
  if (mode == THINKINK_MONO) {
    palette = (uint8_t *)palette_mono;
  } else if (mode == THINKINK_GRAYSCALE4) {
    palette = (uint8_t *)palette_grayscale4;
  } else {
    palette = (uint8_t *)palette_tricolor;
  }
  if (dither == DITHER_NONE) {
    while (width--) {
      epd->drawPixel(x++, y, palette[quantize(*src++, mode) * 4 + 3]);
    }
  } else if (dither == DITHER_PATTERN) {
  } else {
  }
}

/*!
    @brief   Draw an image in RAM to an Adafruit ThinkInk display.
    @param   epd     Screen to draw to (any Adafruit_EPD-derived class).
    @param   x       Horizontal offset in pixels; left edge = 0,
                     positive = right. Value is signed, image will be clipped
                     if all or part is off the screen edges. Screen rotation
                     setting is observed.
    @param   y       Vertical offset in pixels; top edge = 0, positive = down.
    @param   mode    One of the thinkinkmode_t types enumerated in
                     Adafruit_EPD library (e.g. THINKINK_MONO).
    @param   dither  One of the dither_t values enumerated in header -
                     DITHER_NONE, DITHER_ORDERED or DITHER_DIFFUSION.
    @return  None (void).
*/
void Adafruit_Image_ThinkInk::draw(Adafruit_EPD &epd, int16_t x, int16_t y,
                                   thinkinkmode_t mode, dither_t dither) {
  if ((x >= epd.width()) || (y >= epd.height())) {
    return; // Reject off right/bottom
  }

  if (format == IMAGE_1) {
    int x2 = x + canvas.canvas1->width() - 1;
    int y2 = y + canvas.canvas1->height() - 1;
    if ((x2 < 0) || (y2 < 0)) {
      return; // Reject off left/top
    }
    // Vertical clipping is achieved with an offset into the canvas buffer.
    uint8_t *buffer = canvas.canvas1->getBuffer();
    if (y < 0) { // Top clip
      buffer -= y * ((canvas.canvas1->width() + 7) / 8);
      y = 0;
    }
    if (y2 >= epd.height()) { // Bottom clip
      y2 = epd.height() - 1;
    }
    // Horizontal clipping is peculiar by comparison...because source data
    // is bit packed, need to keep track of the initial byte offset (at the
    // start of each row) and bit index of the first pixel...
    uint16_t initial_offset;
    uint8_t initial_bit;
    if (x < 0) { // Left clip
      initial_offset = -x / 8;
      initial_bit = 7 - (-x & 7); // 7 to 0
      x = 0;
    } else {
      initial_offset = initial_bit = 0;
    }
    if (x2 >= epd.width()) { // Right clip
      x2 = epd.width() - 1;
    }

    epd.startWrite();

    uint16_t epdbuf[BUFPIXELS]; // Temp space for buffering EPD data
    uint16_t destidx = 0;
    for (; y <= y2; y++) { // For each row...
      uint16_t offset = initial_offset;
      uint8_t bit = initial_bit;
      int span_x = x;
      for (int16_t col = x; col <= x2; col++) {
        epdbuf[destidx++] = palette[(buffer[offset] >> bit) & 1];
        if (bit) {
          bit--;
        } else {
          bit = 7;
          offset++;
        }
        // If last pixel of row, or if epdbuf is full...
        if ((col >= x2) || (destidx >= BUFPIXELS)) {
          // Pass epdbuf data to span-drawing function...
          span(epdbuf, &epd, span_x, y, destidx, mode, dither);
          span_x += destidx; // Next span starts here
          destidx = 0;       // Reset epdbuf
        }
      }
      buffer += (canvas.canvas1->width() + 7) / 8; // Offset to next row
    }
  } else if (format == IMAGE_8) {
    // BMP reader doesn't currently handle palettized images
  } else if (format == IMAGE_16) {
    int x2 = x + canvas.canvas16->width() - 1;
    int y2 = y + canvas.canvas16->height() - 1;
    if ((x2 < 0) || (y2 < 0)) {
      return; // Reject off left/top
    }
    uint16_t *buffer = canvas.canvas16->getBuffer();
    if (y < 0) {                              // Top clip
      buffer -= y * canvas.canvas16->width(); // Offset to first scanline
      y = 0;
    }
    if (y2 >= epd.height()) { // Bottom clip
      y2 = epd.height() - 1;
    }
    if (x < 0) {   // Left clip
      buffer += x; // Offset to first column
      x = 0;
    }
    if (x2 >= epd.width()) { // Right clip
      x2 = epd.width() - 1;
    }

    epd.startWrite();

    int16_t width = x2 - x + 1;
    for (; y <= y2; y++) { // For each row...
      // Call span function, passing pointer into RGB565 image
      span(buffer, &epd, x, y, width, mode, dither);
      buffer += canvas.canvas16->width(); // Offset to next scanline
    }
  } // end IMAGE_16
  epd.endWrite();
}

// ADAFRUIT_IMAGEREADER_THINKINK CLASS *************************************
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
Adafruit_ImageReader_ThinkInk::Adafruit_ImageReader_ThinkInk(FatFileSystem &fs)
    : Adafruit_ImageReader(fs) {}

/*!
    @brief   Loads BMP image file from SD card directly to Adafruit_ThinkInk
             display.
    @param   filename  Name of BMP image file to load.
    @param   epd       Screen to draw to (any Adafruit_EPD-derived class).
    @param   x         Horizontal offset in pixels; left edge = 0,
                       positive = right. Value is signed, image will be
                       clipped if all or part is off the screen edges.
                       Screen rotation setting is observed.
    @param   y         Vertical offset in pixels; top edge = 0,
                       positive = down.
    @param   mode      One of the thinkinkmode_t types enumerated in
                       Adafruit_EPD library (e.g. THINKINK_MONO).
    @param   dither    One of the dither_t values enumerated in header -
                       DITHER_NONE, DITHER_ORDERED or DITHER_DIFFUSION.
    @param   transact  Pass 'true' if TFT and SD are on the same SPI bus,
                       in which case SPI transactions are necessary. If
                       separate peripherals, can pass 'false'.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader_ThinkInk::drawBMP(
    char *filename, Adafruit_EPD &epd, int16_t x, int16_t y,
    thinkinkmode_t mode, dither_t dither, boolean transact) {
  uint16_t epdbuf[BUFPIXELS]; // Temp space for buffering EPD data
  // Call core BMP-reading function, passing address to EPD object,
  // EPD working buffer, and X & Y position of top-left corner (image
  // will be cropped on load if necessary). Image pointer is NULL when
  // reading to EPD, and transact argument is passed through.
  return coreBMP(filename, &epd, epdbuf, x, y, NULL, mode, dither, transact);
}

/*!
    @brief   BMP-reading function common both to the draw function (to EPD)
             and load function (to canvas object in RAM). BMP code has been
             centralized here so if/when more BMP format variants are added
             in the future, it doesn't need to be implemented, debugged and
             kept in sync in two places.
    @param   filename  Name of BMP image file to load.
    @param   epd       Screen to draw to (any Adafruit_EPD-derived class)
                       if loading to screen, or NULL otherwise.
    @param   dest      Working buffer for loading 16-bit TFT pixel data,
                       if loading to screen, else NULL.
    @param   x         Horizontal offset in pixels (if loading to screen).
    @param   y         Vertical offset in pixels (if loading to screen).
    @param   img       Pointer to Adafruit_Image_ThinkInk object, if
                       loading to RAM (or NULL if loading to screen).
    @param   mode      One of the thinkinkmode_t types enumerated in
                       Adafruit_EPD library (e.g. THINKINK_MONO).
    @param   dither    One of the dither_t values enumerated in header -
                       DITHER_NONE, DITHER_ORDERED or DITHER_DIFFUSION
                       (IGNORED if loading image to canvas).
    @param   transact  Use SPI transactions; 'true' is needed only if
                       loading to screen and it's on the same SPI bus as
                       the SD card. Other situations can use 'false'.
    @return  One of the ImageReturnCode values (IMAGE_SUCCESS on successful
             completion, other values on failure).
*/
ImageReturnCode Adafruit_ImageReader_ThinkInk::coreBMP(
    char *filename,    // SD file to load
    Adafruit_EPD *epd, // Pointer to TFT object, or NULL if to image
    uint16_t *dest,    // EPD working buffer, or NULL if to canvas
    int16_t x,         // Position if loading to EPD (else ignored)
    int16_t y,
    Adafruit_Image_ThinkInk *img, // NULL if load-to-screen
    thinkinkmode_t mode, dither_t dither,
    boolean transact) { // SD & EPD sharing bus, use transactions

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

      // CURRENTLY ONLY 24-BIT BGR AND 1-BIT BMPS ARE SUPPORTED.
      // 8-bit grayscale, 8- or 4-bit paletted, etc. are NOT HANDLED.

      if ((depth == 24) || (depth == 1)) {

        if (img) {
          // Loading to RAM -- allocate GFX canvas
          status = IMAGE_ERR_MALLOC; // Assume won't fit to start
          // Future: handle other depths.
          if (depth == 24) {
            if ((img->canvas.canvas16 = new GFXcanvas16(bmpWidth, bmpHeight))) {
              dest = img->canvas.canvas16->getBuffer();
            }
          } else {
            if ((img->canvas.canvas1 = new GFXcanvas1(bmpWidth, bmpHeight))) {
              dest1 = img->canvas.canvas1->getBuffer();
            }
          }
        } // else loading to screen -- 'dest' pointer was passed in

        if (dest || dest1) { // Supported format, alloc OK, etc.
          status = IMAGE_SUCCESS;

          if ((loadWidth > 0) && (loadHeight > 0)) { // Clip top/left
            if (epd) {                               // If loading to display...
              epd->startWrite(); // Start SPI (regardless of transact)
              epd_col = x;
              epd_row = y;
            } else {
              // Future: handle other depths.
              if (depth == 1) {
                img->format = IMAGE_1; // Is a GFX 1-bit canvas type
              } else {
                img->format = IMAGE_16; // Is a GFX 16-bit canvas type
              }
            }

            if ((depth >= 16) ||
                (quantized = (uint16_t *)malloc(colors * sizeof(uint16_t)))) {
              if (depth < 16) {
                // Load color table, quantied to RGB565. This is NOT
                // converted to EPD color indices yet -- that's done
                // during draw operation, as there may yet be
                // dithering operations to perform.
                for (uint16_t c = 0; c < colors; c++) {
                  b = file.read();
                  g = file.read();
                  r = file.read();
                  (void)file.read(); // Ignore 4th byte
                  quantized[c] =
                      ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                }
              }

              for (row = 0; row < loadHeight; row++) {

                // EACH SCANLINE -------------------------------------------

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
                  if (img) { // If loading 1-bit image to RAM
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

                int span_x = epd_col;

                for (col = 0; col < loadWidth; col++) {

                  // EACH PIXEL ON SCANLINE --------------------------------

                  if (srcidx >= sizeof sdbuf) { // Time to load more data?
                    if (epd && transact) {
                      epd->endWrite(); // End display SPI transaction
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
                    if (epd && transact) {
                      epd->startWrite(); // Start next display SPI transact
                    }
                    srcidx = 0; // Reset bmp buf index
                  }

                  if (depth == 24) {
                    // Convert RGB pixel to 565 format, save in dest.
                    // Makes no difference if going to screen or canvas.
                    b = sdbuf[srcidx++];
                    g = sdbuf[srcidx++];
                    r = sdbuf[srcidx++];
                    dest[destidx++] =
                        ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                  } else { // 1-bit
                    // Extract bit (color index) from BMP...
                    uint8_t n = (sdbuf[srcidx] >> bitIn) & 1;
                    if (!bitIn) {
                      srcidx++;
                      bitIn = 7;
                    } else {
                      bitIn--;
                    }
                    if (epd) { // Loading 1-bit image to display...
                      // RGB565 color from palette goes in EPD dest buffer...
                      dest[destidx++] = quantized[n];
                      // Even though source image is 1-bit, and display might
                      // be 1-bit, it's not always the case that the source
                      // image is black & white (could be any two RGB colors).
                      // Hence the expansion to RGB565. The span-rendering
                      // function then quantizes and/or dithers this to what
                      // the display can best handle. In some cases that's
                      // fantastically bloaty (converting 1-bit to 16-bit RGB
                      // and then back to 1-bit later in the span function),
                      // but the alternative is a TON of special case code.
                      // Since the EPD is slow to update anyway, we'll just
                      // accept it, the code isn't really the bottleneck.
                    } else { // Loading 1-bit image to RAM...
                      // Store bit in canvas1 buffer
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

                  // If loading to display, and either we're on the last pixel
                  // of the current row, OR if the dest buffer is full...
                  if (epd &&
                      ((col == (loadWidth - 1)) || (destidx >= BUFPIXELS))) {
                    // Issue a span of pixels to the display...
                    span(dest, epd, span_x, epd_row + row, destidx, mode,
                         dither);
                    span_x += destidx; // Next span will start here
                    destidx = 0;       // Reset dest buffer counter
                  }
                } // end pixel (column) loop -------------------------------
              }   // end scanline (row) loop -------------------------------

              if (quantized) {     // Was an RGB565 palette allocated?
                if (epd)           // If image was loaded to display,
                  free(quantized); // palette is no longer needed
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
