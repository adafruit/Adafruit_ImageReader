
/***************************************************
  This is a example sketch demonstrating bitmap drawing
  capabilities of the SSD1351 library for the 1.5"
  and 1.27" 16-bit Color OLEDs with SSD1351 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1431
  ------> http://www.adafruit.com/products/1673

  If you're using a 1.27" OLED, change SSD1351HEIGHT in Adafruit_SSD1351.h
   to 96 instead of 128

  These displays use SPI to communicate, 4 or 5 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution

  The Adafruit GFX Graphics core library
  https://github.com/adafruit/Adafruit-GFX-Library
  and Adafruit ImageReader library are also required
  https://github.com/adafruit/Adafruit_ImageReader
  Be sure to install them!
 ****************************************************/

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_ImageReader.h> // Image-reading functions

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128

// If we are using the hardware SPI interface, these are the pins (for future ref)
#define SCLK_PIN 13
#define MOSI_PIN 11
#define CS_PIN   5
#define RST_PIN  6
#define DC_PIN   4

// to draw images from the SD card, we will share the hardware SPI interface
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, CS_PIN, DC_PIN, RST_PIN);


// For Arduino Uno/Duemilanove, etc
//  connect the SD card with MOSI going to pin 11, MISO going to pin 12 and SCK going to pin 13 (standard)
//  Then pin 10 goes to CS (or whatever you have set up)
#define SD_CS 10    // Set the chip select line to whatever you use (10 doesnt conflict with the library)

Adafruit_ImageReader reader;     // Class w/image-reading functions

void setup(void) {

  ImageReturnCode stat; // Status from image-reading functions

  Serial.begin(9600);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  // initialize the OLED
  tft.begin();

  Serial.println("init");

  tft.fillScreen(BLUE);

  delay(500);
  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("SD OK!");

  Serial.print(F("Loading /lily128.bmp to screen..."));
  stat = reader.drawBMP("/lily128.bmp", tft, 0, 0);
  reader.printStatus(stat);   // How'd we do?
}

void loop() {
}
