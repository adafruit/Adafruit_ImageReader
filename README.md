# Adafruit ImageReader Arduino Library [![Build Status](https://github.com/adafruit/Adafruit_ImageReader/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/Adafruit_ImageReader/actions)[![Documentation](https://github.com/adafruit/ci-arduino/blob/master/assets/doxygen_badge.svg)](http://adafruit.github.io/Adafruit_ImageReader/html/index.html)

Companion library for Adafruit_GFX to load images from SD card or SPI Flash

Requires Adafruit_GFX library and one of the SPI color graphic display libraries, e.g. Adafruit_ILI9341.

**IMPORTANT NOTE: version 2.0 is a "breaking change"** from the 1.X releases of this library. Existing code WILL NOT COMPILE without revision. Adafruit_ImageReader now relies on the Adafruit_SPIFlash and SdFat libraries, and the Adafruit_ImageReader constructor call has changed (other functions remain the same). See the examples for reference. Very sorry about that but it brings some helpful speed and feature benefits (like loading from SPI/QSPI flash).
