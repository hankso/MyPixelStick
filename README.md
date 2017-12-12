# Pixelstick
- my pixelstick using arduino and library NeoPixel
- in this version it can read commands from SD card or receive color data via blueteeth

# Pin define
- SD card attached as follows:
    - MOSI - pin 11
    - MISO - pin 12
    - CLK - pin 13
    - CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)


- PixelStick Data Pin 6


- Bluetooth
    - RX --> Arduino TX1
    - TX --> Arduino RX0
    - baudrate: 115200(default)
    - soft serial performs not so well at 115200, so use hard serial please

# Arduino Interface
- PixelStick
- Version: 1.0.2 https://github.com/hankso/MyPixelStick
- Usage: [options ...]
- Options:
    - run:    start displaying photos from command files if SD is inserted
    - stop:   stop displaying
    - push:   start sending data via serial (HardwareSerial | Bluetooth)
    - time:   set time-duration (e.g. 'time-10'|'time+50'|'time=20')
    - ls:     print a tree structure of files in SD card
    - ips:    interactive pixelstick. control pixels in format (i r g b)
    - next:   run next cmd file in SD card
    - prev:   run previous cmd file
    - clear:  set all pixels off
    - pixel:  set num of pixels (e.g. 'pixel 30'|'pixel 50')

![arduino interface](https://github.com/hankso/MyPixelStick/blob/master/Screenshot%20from%202017-11-24%2023:30:17.png)

(latest version test result is in file "Serial Monitor")

### Data format
- Command file should have header and end like this:
    - #file.txt#               // '#' + filename + '#'
    - uint32_t data
    - uint32_t data
    - ......
    - -1-1-1-1...              // more than two -1 for ensurance

- serial.parseInt() will return '0' if timeout, which may be misunderstanded as '0 0 0 0', so use '-1' as an indicator of end.

### class Adafruit_NeoPixel
- void
    - [x] begin(void)
    - [x] show(void)
    - setPin(uint8_t p)
    - [x] setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
    - setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    - setPixelColor(uint16_t n, uint32_t c)
    - [ ] setBrightness(uint8_t)
    - [x] clear()
    - [x] updateLength(uint16_t n)
    - updateType(neoPixelType t)
- uint8_t
    - getPixels(void) const,
    - getBrightness(void) const;
- int8_t
    - getPin(void) { return pin; };
- uint16_t
    - numPixels(void) const;
- static uint32_t
    - Color(uint8_t r, uint8_t g, uint8_t b),
    - Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
- uint32_t
    - getPixelColor(uint16_t n) const;

### printf
There doesn't exist printf("%d",int) in arduino...
In order not to use:
```C++
sprintf (buffer, "you send %d something here", foo);
Serial.print (buffer);
```
I add some code to $ARDUINO_DIR/hardware/arduino/cores/Print.h.

More details at http://playground.arduino.cc/Main/Printf

-------------------------------------------
TODO: acceleration sensor such as 'MMA7631' to change the t_Step(time step)

i.e. horizontal pixel interval of an image
