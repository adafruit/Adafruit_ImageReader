// Adafruit_ImageReader test for 2.4" TFT FeatherWing.
// Requires three BMP files in root directory of SD card:
// purple.bmp, parrot.bmp and test.bmp.

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ILI9341.h>     // Hardware-specific library
#include <Adafruit_ImageReader.h> // Image-reading functions

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
#elif defined(ARDUINO_NRF52_FEATHER) /* BSP 0.6.5 and higher! */
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

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_ImageReader reader;   // Class w/image-reading functions
Adafruit_Image       img; // Contains dynamically-allocated canvas object

void setup(void) {

  ImageReturnCode stat;              // Status from image-reading functions
  int32_t         width=0, height=0; // BMP image dimensions

  Serial.begin(9600);
#if !defined(ESP32)
  while(!Serial); // Wait for Serial Monitor before continuing
#endif

  tft.begin();

  Serial.print(F("Initializing SD card..."));
  if(!SD.begin(SD_CS)) {
    Serial.println("failed!");
    for(;;); // Loop here forever
  }
  Serial.println(F("OK!"));

  // Fill screen blue just to show that we're talking to screen OK
  tft.fillScreen(ILI9341_BLUE);

  // Load BMP file 'purple.bmp' at position (0, 0) (top left corner)
  Serial.print(F("Loading purple.bmp to screen..."));
  stat = reader.drawBMP("/purple.bmp", tft, 0, 0);
  // (Absolute path isn't necessary on most devices, but something
  // with the ESP32 SD library seems to require it.)
  printStatus(stat); // How'd we do?

  Serial.print("Querying parrot.bmp image size...");
  stat = reader.bmpDimensions("/parrot.bmp", &width, &height);
  printStatus(stat); // How'd we do?
  if(stat == IMAGE_SUCCESS) { // If it worked...
    Serial.print(F("Image dimensions: "));
    Serial.print(width);
    Serial.write('x');
    Serial.println(height);
    // Draw 4 parrots, edges are clipped as necessary...
    for(int i=0; i<4; i++) {
      reader.drawBMP("/parrot.bmp", tft,
        (tft.width()  * i / 3) - (width  / 2),
        (tft.height() * i / 3) - (height / 2));
    }
  }

  // Load BMP 'test.bmp' into a GFX canvas in RAM.
  // This won't work on AVR and other small devices.
  Serial.print("Loading test.bmp to canvas...");
  stat = reader.loadBMP("/test.bmp", img);
  printStatus(stat); // How'd we do?
}

void loop() {
//  if(img.canvas.canvas16) { // If second BMP successfully loaded in RAM...
  if(1) {
    img.draw(tft,
      (int16_t)random(-img.width() , tft.width()) , // Horiz pos.
      (int16_t)random(-img.height(), tft.height())); // Vert pos
  }
}

void printStatus(ImageReturnCode stat) {
  if(stat == IMAGE_SUCCESS)
    Serial.println(F("Success!"));
  else if(stat == IMAGE_ERR_FILE_NOT_FOUND)
    Serial.println(F("File not found."));
  else if(stat == IMAGE_ERR_FORMAT)
    Serial.println(F("Not a supported BMP variant."));
  else if(stat == IMAGE_ERR_MALLOC)
    Serial.println(F("Malloc failed (insufficient RAM)."));
}
