#include <Adafruit_GFX.h>         // Core graphics library
#include "Adafruit_ThinkInk.h"
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_ImageReader_EPD.h> // Image-reading functions


#define SD_CS       5
#define SRAM_CS     -1
#define EPD_CS      9
#define EPD_DC      10  
#define EPD_RESET   -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    -1 // can set to -1 to not use a pin (will wait a fixed delay)

ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

SdFat SD;         // SD card filesystem
Adafruit_ImageReader_EPD reader(SD); // Image-reader object, pass in SD filesys


#define FILENAME1 "/adabot_head.bmp"
#define FILENAME2 "/panda_head.bmp"
#define FILENAME3 "/29gray4.bmp"

#define BUTTON1 11
#define BUTTON2 12
#define BUTTON3 13


void setup(void) {
  Serial.begin(9600);
  //while(!Serial);           // Wait for Serial Monitor before continuing

  display.begin();
  display.clearBuffer();
 /*
  Serial.print("Initializing filesystem...");
  display.setTextSize(3);
  display.setCursor(10,10);
  display.setTextColor(EPD_BLACK);
  display.print("SD Card...");
  */
  display.display();

  // SD card is pretty straightforward, a single call...
  if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
    Serial.println(F("SD begin() failed"));
    display.println("failed!");
    display.display();
    for(;;); // Fatal error, do not continue
  }
  Serial.println("OK!");
   /*
  display.println("OK!");
  display.display();

  display.setCursor(10,100);
  display.setTextSize(1);
  display.println("Press buttons to display images!");
  display.display();
*/
  pinMode(BUTTON1, INPUT_PULLUP); 
  pinMode(BUTTON2, INPUT_PULLUP); 
  pinMode(BUTTON3, INPUT_PULLUP);
  delay(10);
}

void loop() {
  char selected_file[80] = {0};
  ImageReturnCode stat; // Status from image-reading functions


  if (!digitalRead(BUTTON1)) {
    strcpy(selected_file, FILENAME1);
    Serial.println("Button 1 pressed");
  }
  if (!digitalRead(BUTTON2)) {
    strcpy(selected_file, FILENAME2);
    Serial.println("Button 2 pressed");
  }
  if (!digitalRead(BUTTON3)) {
    strcpy(selected_file, FILENAME3);
    Serial.println("Button 3 pressed");
  }

  
  if (selected_file[0] != 0) {  
    display.clearBuffer();
    display.display();
    delay(500);
    Serial.print("Loading ");
    Serial.print(selected_file);
    Serial.println(" to canvas...");
    display.clearBuffer();
    stat = reader.drawBMP(selected_file, display, 0, 0);
    reader.printStatus(stat); // How'd we do?    
    display.display();
  }

}
