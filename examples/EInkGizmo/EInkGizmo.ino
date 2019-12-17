// Adafruit_ImageReader test for Adafruit E-Ink Gizmo for CircuitPlayground.
// Demonstrates loading images to the screen, to RAM, and how to query
// image file dimensions.
// Requires BMP file in root directory of QSPI Flash:
// blinka.bmp.

#include <Adafruit_GFX.h>         // Core graphics library
#include "Adafruit_EPD.h"         // Hardware-specific library for EPD
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader_EPD.h> // Image-reading functions

#define EPD_CS      0
#define EPD_DC      1
#define SRAM_CS     -1
#define EPD_RESET   PIN_A3
#define EPD_BUSY    -1

Adafruit_IL0373      display(152, 152, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

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

Adafruit_SPIFlash    flash(&flashTransport);
FatFileSystem        filesys;
Adafruit_ImageReader reader(filesys); // Image-reader, pass in flash filesys

Adafruit_Image_EPD   img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);
  //while(!Serial);           // Wait for Serial Monitor before continuing

  display.begin();
  display.setRotation(3);  

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatFileSystem object. This MUST
  // BE INITIALIZED before using any of the image reader functions!
  Serial.print(F("Initializing filesystem..."));
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

  Serial.println(F("OK!"));

  // Load full-screen BMP file 'tricolor-blinka.bmp' at position (0,0) (top left).
  // Notice the 'reader' object performs this, with 'epd' as an argument.
  Serial.print(F("Loading tricolor-blinka.bmp to canvas..."));
  stat = reader.loadBMP((char *)"/tricolor-blinka.bmp", img);
  reader.printStatus(stat); // How'd we do?

  Serial.print(F("Drawing canvas to EPD..."));
  display.clearBuffer();
  img.draw(display, 0, 0);
  display.display();

  delay(15 * 1000); // Pause 15 seconds before moving on to loop()
}

void loop() {
  for(int r=0; r<4; r++) { // For each of 4 rotations...
    display.setRotation(r);    // Set rotation
    display.fillScreen(0);     // and clear screen
    display.clearBuffer();
    img.draw(display, 0, 0);
    display.display();
    delay(15 * 1000); // Pause 15 sec.
  }
}
