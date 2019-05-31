// Adafruit_NeoMatrix example for single NeoPixel Shield.

//convert images here: https://online-converting.com/image/convert2bmp/
//connect sd-card-reader to arduino mega https://www.arduino.cc/en/Reference/SPI
//this project is made similiar to this: https://learn.adafruit.com/adafruit-gfx-graphics-library/loading-images
//we just added the draw-function with a neomatrix object to the image-reader-library (and the includes)

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_NeoMatrix.h>   
#include <Adafruit_NeoPixel.h>
#include <Adafruit_ImageReader.h> // Image-reading functions



#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define PIN 6

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)


// Example for NeoPixel Shield.  In this application we'd like to use it
// as a 5x8 tall matrix, with the USB port positioned at the top of the
// Arduino.  When held that way, the first pixel is at the top right, and
// lines are arranged in columns, progressive order.  The shield uses
// 800 KHz (v2) pixels that expect GRB color data.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(30, 30, PIN,
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_RGB            + NEO_KHZ800);
  
Adafruit_ImageReader reader; // Class w/image-reading functions
Adafruit_Image img;

ImageReturnCode stat;

void setup() {
  matrix.begin();
  matrix.setBrightness(255);


  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //while (!Serial) {
    //; // wait for serial port to connect. Needed for Leonardo only
  //}
  Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

}

void loop() {

    stat = reader.loadBMP("/bild_1.bmp", img);
    reader.printStatus(stat);
    img.draw(matrix,0,0);
    matrix.show();
    delay(60000);
    
    stat = reader.loadBMP("/bild_2.bmp", img);
    reader.printStatus(stat);
    img.draw(matrix,0,0);
    matrix.show();
    delay(60000);

    stat = reader.loadBMP("/bild_2.bmp", img);
    reader.printStatus(stat);
    img.draw(matrix,0,0);
    matrix.show();
    delay(60000);
}
