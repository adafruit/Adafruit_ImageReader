# Adafruit_ImageReader [![Build Status](https://travis-ci.com/adafruit/Adafruit_ImageReader.svg?branch=master)](https://travis-ci.com/adafruit/Adafruit_ImageReader)

Companion library for Adafruit_GFX to load images from SD card.

Requires Adafruit_GFX library and one of the SPI color graphic display libraries, e.g. Adafruit_ILI9341.

**IMPORTANT NOTE: version 2.0 is a "breaking change"** from the 1.X releases of this library. Existing code WILL NOT COMPILE without revision. Adafruit_ImageReader now relies on the Adafruit_SPIFlash and SdFat libraries, and the Adafruit_ImageReader constructor call has changed (other functions remain the same). See the examples for reference. Very sorry about that but it brings some helpful speed and feature benefits (like loading from SPI/QSPI flash).
