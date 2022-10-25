// Adafruit_ImageReader test for Adafruit TFT Gizmo for CircuitPlayground.
// Demonstrates loading images to the screen, to RAM, and how to query
// image file dimensions.
// Requires three BMP files in root directory of SD card:
// parrot.bmp, miniwoof.bmp and wales.bmp.

#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ST7789.h>      // Hardware-specific library for ST7789
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions

// TFT display and SD card share the hardware SPI interface, using
// 'select' pins for each to identify the active device on the bus.

#define TFT_CS        0
#define TFT_RST       -1 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC        1
#define TFT_BACKLIGHT PIN_A3 // Display backlight pin

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

// TFT SPI interface selection
#if (SPI_INTERFACES_COUNT == 1)
  SPIClass* spi = &SPI;
#else
  SPIClass* spi = &SPI1;
#endif

Adafruit_SPIFlash    flash(&flashTransport);
FatVolume        filesys;
Adafruit_ImageReader reader(filesys); // Image-reader, pass in flash filesys

Adafruit_ST7789      tft    = Adafruit_ST7789(spi, TFT_CS, TFT_DC, TFT_RST);
Adafruit_Image       img;        // An image loaded into RAM
int32_t              width  = 0, // BMP image dimensions
                     height = 0;

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);
  //while(!Serial);           // Wait for Serial Monitor before continuing

  tft.init(240, 240);           // Init ST7789 240x240
  tft.setRotation(2);  
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH); // Backlight on

  // The Adafruit_ImageReader constructor call (above, before setup())
  // accepts an uninitialized SdFat or FatVolume object. This MUST
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

  // Fill screen blue. Not a required step, this just shows that we're
  // successfully communicating with the screen.
  tft.fillScreen(ST77XX_BLUE);

  // Load full-screen BMP file 'adabot240.bmp' at position (0,0) (top left).
  // Notice the 'reader' object performs this, with 'tft' as an argument.
  Serial.print(F("Loading adabot240.bmp to screen..."));
  stat = reader.drawBMP("/adabot240.bmp", tft, 0, 0);
  reader.printStatus(stat);   // How'd we do?

  // Query the dimensions of image 'miniwoof.bmp' WITHOUT loading to screen:
  Serial.print(F("Querying miniwoof.bmp image size..."));
  stat = reader.bmpDimensions("/miniwoof.bmp", &width, &height);
  reader.printStatus(stat);   // How'd we do?
  if(stat == IMAGE_SUCCESS) { // If it worked, print image size...
    Serial.print(F("Image dimensions: "));
    Serial.print(width);
    Serial.write('x');
    Serial.println(height);
  }

  // Load small BMP 'wales.bmp' into a GFX canvas in RAM.
  Serial.print(F("Loading wales.bmp to canvas..."));
  stat = reader.loadBMP("/wales.bmp", img);
  reader.printStatus(stat); // How'd we do?

  delay(2000); // Pause 2 seconds before moving on to loop()
}

void loop() {
  for(int r=0; r<4; r++) { // For each of 4 rotations...
    tft.setRotation(r);    // Set rotation
    tft.fillScreen(0);     // and clear screen

    // Load 4 copies of the 'miniwoof.bmp' image to the screen, some
    // partially off screen edges to demonstrate clipping. Globals
    // 'width' and 'height' were set by bmpDimensions() call in setup().
    for(int i=0; i<4; i++) {
      reader.drawBMP("/miniwoof.bmp", tft,
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
