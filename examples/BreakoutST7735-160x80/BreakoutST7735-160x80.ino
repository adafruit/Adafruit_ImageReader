// Adafruit_ImageReader test for Adafruit Mini TFT Breakout for Arduino.
// Demonstrates loading images from SD card or flash memory to the screen,
// to RAM, and how to query image file dimensions.
// OPEN THE ARDUINO SERIAL MONITOR WINDOW TO START PROGRAM.
// Requires three BMP files in root directory of SD card:
// minibot.bmp, rgbwheel.bmp and wales.bmp.
// As written, this uses the microcontroller's SPI interface for the screen
// (not 'bitbang') and must be wired to specific pins (e.g. for Arduino Uno,
// MOSI = pin 11, MISO = 12, SCK = 13). Other pins are configurable below.

#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ST7735.h>      // Hardware-specific library
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions

// TFT display and SD card share the hardware SPI interface, using
// 'select' pins for each to identify the active device on the bus.

// Comment out the next line to load from SPI/QSPI flash instead of SD card:
#define USE_SD_CARD

#define SD_CS    4 // SD card chip select
#define TFT_CS  10 // TFT select pin
#define TFT_DC   8 // TFT data/command pin
#define TFT_RST  9 // Or set to -1 and connect TFT RST to Arduino reset pin

#if defined(USE_SD_CARD)
  SdFat                SD;         // SD card filesystem
  Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys
#else
  // SPI or QSPI flash filesystem (i.e. CIRCUITPY drive)
  #if defined(__SAMD51__) || defined(NRF52840_XXAA)
    Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS,
      PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
  #else
    #if (SPI_INTERFACES_COUNT == 1)
      Adafruit_FlashTransport_SPI flashTransport(SS, &SPI);
    #else
      Adafruit_FlashTransport_SPI flashTransport(SS1, &SPI1);
    #endif
  #endif
  Adafruit_SPIFlash    flash(&flashTransport);
  FatFileSystem        filesys;
  Adafruit_ImageReader reader(filesys); // Image-reader, pass in flash filesys
#endif

Adafruit_ST7735      tft    = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_Image       img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);
#if !defined(ESP32)
  while(!Serial);       // Wait for Serial Monitor before continuing
#endif

  tft.initR(INITR_MINI160x80); // Initialize screen
  Serial.println(F("TFT initialized."));

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatFileSystem object. This MUST
  // BE INITIALIZED before using any of the image reader functions!
  Serial.print(F("Initializing filesystem..."));
#if defined(USE_SD_CARD)
  // SD card is pretty straightforward, a single call...
  if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
    Serial.println(F("SD begin() failed"));
    for(;;); // Fatal error, do not continue
  }
#else
  // SPI or QSPI flash requires two steps, one to access the bare flash
  // memory itself, then the second to access the filesystem within...
  if(!flash.begin()) {
    Serial.println(F("flash begin() failed"));
    for(;;);
  }
  if(!filesys.begin(&flash)) {
    Serial.println(F("filesys begin() failed"));
    for(;;);
  }
#endif
  Serial.println(F("OK!"));

  // Fill screen blue. Not a required step, this just shows that we're
  // successfully communicating with the screen.
  tft.fillScreen(ST77XX_BLUE);

  // Load full-screen BMP file 'minibot.bmp' at position (0,0) (top left).
  // Notice the 'reader' object performs this, with 'tft' as an argument.
  Serial.print(F("Loading minibot.bmp to screen..."));
  stat = reader.drawBMP("/minibot.bmp", tft, 0, 0);
  // (Absolute path isn't necessary on most devices, but something
  // with the ESP32 SD library seems to require it.)
  reader.printStatus(stat);   // How'd we do?

  // Query the dimensions of image 'rgbwheel.bmp' WITHOUT loading to screen:
  Serial.print(F("Querying rgbwheel.bmp image size..."));
  stat = reader.bmpDimensions("/rgbwheel.bmp", &width, &height);
  reader.printStatus(stat);   // How'd we do?
  if(stat == IMAGE_SUCCESS) { // If it worked, print image size...
    Serial.print(F("Image dimensions: "));
    Serial.print(width);
    Serial.write('x');
    Serial.println(height);
  }

  // Load small BMP 'wales.bmp' into a GFX canvas in RAM. This should
  // fail gracefully on AVR and other small devices, meaning the image
  // will not load, but this won't make the program stop or crash, it
  // just continues on without it.
  Serial.print(F("Loading wales.bmp to canvas..."));
  stat = reader.loadBMP("/wales.bmp", img);
  reader.printStatus(stat); // How'd we do?

  delay(2000); // Pause 2 seconds before moving on to loop()
}

void loop() {
  for(int r=0; r<4; r++) { // For each of 4 rotations...
    tft.setRotation(r);    // Set rotation
    tft.fillScreen(0);     // and clear screen

    // Load 4 copies of the 'rgbwheel.bmp' image to the screen, some
    // partially off screen edges to demonstrate clipping. Globals
    // 'width' and 'height' were set by bmpDimensions() call in setup().
    for(int i=0; i<4; i++) {
      reader.drawBMP("/rgbwheel.bmp", tft,
        (tft.width()  * i / 3) - (width  / 2),
        (tft.height() * i / 3) - (height / 2));
    }

    delay(1000); // Pause 1 sec.

    // Draw 50 Welsh dragon flags in random positions. This has no effect
    // on memory-constrained boards like the Arduino Uno, where the image
    // failed to load due to insufficient RAM, but it's NOT fatal.
    for(int i=0; i<50; i++) {
      // Rather than reader.drawBMP() (which works from SD card),
      // a different function is used for RAM-resident images:
      img.draw(tft,                                    // Pass in tft object
        (int16_t)random(-img.width() , tft.width()) ,  // Horiz pos.
        (int16_t)random(-img.height(), tft.height())); // Vert pos
      // Reiterating a prior point: img.draw() does nothing and returns
      // if the image failed to load. It's unfortunate but not disastrous.
    }

    delay(2000); // Pause 2 sec.
  }
}
