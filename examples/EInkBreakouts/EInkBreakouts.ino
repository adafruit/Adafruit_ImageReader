// Adafruit_ImageReader test for Adafruit E-Ink Breakouts.
// Demonstrates loading images from SD card or flash memory to the screen,
// to RAM, and how to query image file dimensions.
// Requires BMP file in root directory of QSPI Flash:
// blinka.bmp.

#include <Adafruit_GFX.h>         // Core graphics library
#include "Adafruit_EPD.h"         // Hardware-specific library for EPD
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader_EPD.h> // Image-reading functions

// Comment out the next line to load from SPI/QSPI flash instead of SD card:
#define USE_SD_CARD

#define EPD_CS     10
#define EPD_DC      9
#define SRAM_CS     8
#define EPD_RESET   5 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    3 // can set to -1 to not use a pin (will wait a fixed delay)
#define SD_CS       4 // SD card chip select

/* Uncomment the following line if you are using 1.54" tricolor EPD */
Adafruit_IL0373 display(152, 152, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

/* Uncomment the following line if you are using 1.54" monochrome EPD */
//Adafruit_SSD1608 display(200, 200, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

/* Uncomment the following line if you are using 2.13" tricolor EPD */
//Adafruit_IL0373 display(212, 104, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
//#define FLEXIBLE_213

/* Uncomment the following line if you are using 2.13" monochrome 250*122 EPD */
//Adafruit_SSD1675 display(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

/* Uncomment the following line if you are using 2.7" tricolor or grayscale EPD */
//Adafruit_IL91874 display(264, 176, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS);

/* Uncomment the following line if you are using 2.9" EPD */
//Adafruit_IL0373 display(296, 128, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
//#define FLEXIBLE_290

/* Uncomment the following line if you are using 4.2" tricolor EPD */
//Adafruit_IL0398 display(300, 400, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#if defined(USE_SD_CARD)
  SdFat                    SD;         // SD card filesystem
  Adafruit_ImageReader_EPD reader(SD); // Image-reader object, pass in SD filesys
#else

// SPI or QSPI flash filesystem (i.e. CIRCUITPY drive)
  #if defined(__SAMD51__) || defined(NRF52840_XXAA)
    Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS,
      PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
  #else
    #if (SPI_INTERFACES_COUNT == 1 || defined(ADAFRUIT_CIRCUITPLAYGROUND_M0))
      Adafruit_FlashTransport_SPI flashTransport(SS, &SPI);
    #else
      Adafruit_FlashTransport_SPI flashTransport(SS1, &SPI1);
    #endif
  #endif
  Adafruit_SPIFlash         flash(&flashTransport);
  FatVolume             filesys;
  Adafruit_ImageReader_EPD  reader(filesys); // Image-reader, pass in flash filesys
#endif

Adafruit_Image_EPD   img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);
  //while(!Serial);           // Wait for Serial Monitor before continuing

  display.begin();

#if defined(FLEXIBLE_213) || defined(FLEXIBLE_290)
  // The flexible displays have different buffers and invert settings!
  display.setBlackBuffer(1, false);
  display.setColorBuffer(1, false);
#endif

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatVolume object. This MUST
  // BE INITIALIZED before using any of the image reader functions!
  Serial.print(F("Initializing filesystem..."));
  // SPI or QSPI flash requires two steps, one to access the bare flash
  // memory itself, then the second to access the filesystem within...
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

  // Load full-screen BMP file 'blinka.bmp' at position (0,0) (top left).
  // Notice the 'reader' object performs this, with 'epd' as an argument.
  Serial.print(F("Loading blinka.bmp to canvas..."));
  stat = reader.drawBMP((char *)"/blinka.bmp", display, 0, 0);
  reader.printStatus(stat); // How'd we do?
  display.display();

  // Query the dimensions of image 'blinka.bmp' WITHOUT loading to screen:
  Serial.print(F("Querying blinka.bmp image size..."));
  stat = reader.bmpDimensions("blinka.bmp", &width, &height);
  reader.printStatus(stat);   // How'd we do?
  if(stat == IMAGE_SUCCESS) { // If it worked, print image size...
    Serial.print(F("Image dimensions: "));
    Serial.print(width);
    Serial.write('x');
    Serial.println(height);
  }

  delay(30 * 1000); // Pause 30 seconds before continuing because it's eInk

  Serial.print(F("Drawing canvas to EPD..."));
  display.clearBuffer();

  // Load small BMP 'blinka.bmp' into a GFX canvas in RAM. This should fail
  // gracefully on Arduino Uno and other small devices, meaning the image
  // will not load, but this won't make the program stop or crash, it just
  // continues on without it. Should work on larger ram boards like M4, etc.
  stat = reader.loadBMP("/blinka.bmp", img);
  reader.printStatus(stat); // How'd we do?
}

void loop() {
  for(int r=0; r<4; r++) {     // For each of 4 rotations...
    display.setRotation(r);    // Set rotation
    display.fillScreen(0);     // and clear screen
    display.clearBuffer();
    img.draw(display, 0, 0);
    display.display();
    delay(30 * 1000); // Pause 30 sec.
  }
}
