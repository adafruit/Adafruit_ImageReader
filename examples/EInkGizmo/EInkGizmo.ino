// Adafruit_ImageReader test for Adafruit E-Ink Gizmo for CircuitPlayground.
// Demonstrates loading images to the screen, to RAM, and how to query
// image file dimensions.
// Requires BMP file in root directory of QSPI Flash:
// blinka.bmp.

#include "Adafruit_ThinkInk.h"    // Hardware-specific library for EPD
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader_EPD.h> // Image-reading functions

#define EPD_CS      0
#define EPD_DC      1
#define SRAM_CS     -1
#define EPD_RESET   PIN_A3
#define EPD_BUSY    -1

// 1.54" 152x152 Tricolor EPD with ILI0373 chipset
//ThinkInk_154_Tricolor_Z17 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
// 1.54" 200x200 Tricolor EPD with SSD1681 chipset
ThinkInk_154_Tricolor_Z90 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

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

Adafruit_Image_EPD   img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;

void setup(void) {
  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(115200);
  //while(!Serial) delay(10);           // Wait for Serial Monitor before continuing

  display.begin(THINKINK_TRICOLOR);
  display.setRotation(3);  

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatVolume object. This MUST
  // BE INITIALIZED before using any of the image reader functions!
  Serial.print(F("Initializing filesystem..."));
  // SPI or QSPI flash requires two steps, one to access the bare flash
  // memory itself, then the second to access the filesystem within...
  if(!flash.begin()) {
    errorEPD("Flash begin() failed");
  }
  if(!filesys.begin(&flash)) {
    errorEPD("filesys begin() failed");
  }

  Serial.println(F("OK!"));

  // Load full-screen BMP file 'blinka.bmp' at position (0,0) (top left).
  // Notice the 'reader' object performs this, with 'epd' as an argument.
  Serial.print(F("Loading blinka.bmp to canvas..."));
  stat = reader.drawBMP((char *)"/blinka.bmp", display, 0, 0);
  reader.printStatus(stat); // How'd we do?
  if (stat != IMAGE_SUCCESS) {
    errorEPD("Unable to draw image");
  }
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
  for(int r=0; r<4; r++) { // For each of 4 rotations...
    display.setRotation(r);    // Set rotation
    display.fillScreen(0);     // and clear screen
    display.clearBuffer();
    img.draw(display, 0, 0);
    display.display();
    delay(30 * 1000); // Pause 30 sec.
  }
}

void errorEPD(const char *errormsg) {
  display.fillScreen(0);     // clear screen
  display.clearBuffer();
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.setTextColor(EPD_BLACK);
  display.print(errormsg); 
  display.display();

  while (1) {
    delay(10);
  }
}