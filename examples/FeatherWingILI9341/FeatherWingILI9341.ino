// Adafruit_ImageReader test for 2.4" TFT FeatherWing. Demonstrates loading
// images from SD card or flash memory to the screen, to RAM, and how to
// query image file dimensions. OPEN THE ARDUINO SERIAL MONITOR WINDOW TO
// START PROGRAM. Requires three BMP files in root directory of SD card or
// flash: purple.bmp, parrot.bmp and wales.bmp.

#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ILI9341.h>     // Hardware-specific library
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions

// Comment out the next line to load from SPI/QSPI flash instead of SD card:
#define USE_SD_CARD

// Pin definitions for 2.4" TFT FeatherWing vary among boards...

#if defined(ESP8266)
  #define TFT_CS   0
  #define TFT_DC   15
  #define SD_CS    2
#elif defined(ESP32)
  #define TFT_CS   15
  #define TFT_DC   33
  #define SD_CS    14
#elif defined(TEENSYDUINO)
  #define TFT_DC   10
  #define TFT_CS   4
  #define SD_CS    8
#elif defined(ARDUINO_STM32_FEATHER)
  #define TFT_DC   PB4
  #define TFT_CS   PA15
  #define SD_CS    PC5
#elif defined(ARDUINO_NRF52_FEATHER) // BSP 0.6.5 and higher!
  #define TFT_DC   11
  #define TFT_CS   31
  #define SD_CS    27
#elif defined(ARDUINO_MAX32620FTHR) || defined(ARDUINO_MAX32630FTHR)
  #define TFT_DC   P5_4
  #define TFT_CS   P5_3
  #define STMPE_CS P3_3
  #define SD_CS    P3_2
#else // Anything else!
  #define TFT_CS   9
  #define TFT_DC   10
  #define SD_CS    5
#endif

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

Adafruit_ILI9341       tft    = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_Image         img;        // An image loaded into RAM
int32_t                width  = 0, // BMP image dimensions
                       height = 0;

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);
#if !defined(ESP32)
  while(!Serial);       // Wait for Serial Monitor before continuing
#endif

  tft.begin();          // Initialize screen

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatFileSystem object. This MUST
  // BE INITIALIZED before using any of the image reader functions!
  Serial.print(F("Initializing filesystem..."));
#if defined(USE_SD_CARD)
  // SD card is pretty straightforward, a single call...
  if(!SD.begin(SD_CS, SD_SCK_MHZ(25))) { // ESP32 requires 25 MHz limit
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
  tft.fillScreen(ILI9341_BLUE);

  // Load full-screen BMP file 'purple.bmp' at position (0,0) (top left).
  // Notice the 'reader' object performs this, with 'tft' as an argument.
  Serial.print(F("Loading purple.bmp to screen..."));
  stat = reader.drawBMP("/purple.bmp", tft, 0, 0);
  // (Absolute path isn't necessary on most devices, but something
  // with the ESP32 SD library seems to require it.)
  reader.printStatus(stat);   // How'd we do?

  // Query the dimensions of image 'parrot.bmp' WITHOUT loading to screen:
  Serial.print(F("Querying parrot.bmp image size..."));
  stat = reader.bmpDimensions("/parrot.bmp", &width, &height);
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

    // Load 4 copies of the 'parrot.bmp' image to the screen, some
    // partially off screen edges to demonstrate clipping. Globals
    // 'width' and 'height' were set by bmpDimensions() call in setup().
    for(int i=0; i<4; i++) {
      reader.drawBMP("/parrot.bmp", tft,
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
