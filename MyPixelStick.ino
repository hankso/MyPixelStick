/*
# MyPixelStick by hank
Date: 2017.11.8

In this version it can read commands from SD card or receive color data via blueteeth

### Pin define
- Bluetooth
    - RX --> Arduino TX1
    - TX --> Arduino RX0
    - baudrate: 115200(default)
    - soft serial performs not so well at 115200, so use hard serial please

- PixelStick Data Pin 8

- Touch detection Pin 10

- SD card attached as follows:
    - CS   - pin 9
    - MOSI - pin 11
    - MISO - pin 12
    - CLK  - pin 13

### Data format
-Command file should have header and end like this:
    - #file.txt#               // '#' + filename + '#'
    - uint32_t data
    - uint32_t data
    - ......
    - -1-1-1-1...              // more than two -1 for ensurance
- serial.parseInt() will return '0' if timeout, which may be
misunderstanded as '0 0 0 0', so use '-1' as an indicator of end.

### class <Adafruit_NeoPixel>
- void
    -[x] begin(void)
    -[x] show(void)
    - setPin(uint8_t p)
    -[x] setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
    - setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    - setPixelColor(uint16_t n, uint32_t c)
    -[ ] setBrightness(uint8_t)
    -[ ] clear()
    -[x] updateLength(uint16_t n)
    - updateType(neoPixelType t)
- uint8_t
    - *getPixels(void) const,
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
    sprintf (buffer, "you send %d something here", foo);
    Serial.print (buffer);
I add some code to $ARDUINO_DIR/hardware/arduino/cores/Print.h
more details at 'http://playground.arduino.cc/Main/Printf'

-------------------------------------------
TODO: acceleration sensor such as 'MMA7631'
to change tStep(time step)
i.e. horizontal pixel interval of a frame
*/

#include <Adafruit_NeoPixel.h>
#include <SD.h>

#define Lighterval 255/5
#define pixelPin   8
#define SDPin      9
#define touchPin   10
#define BT_baud    115200
#define myport     Serial


// variables
File
    data,                            // current data file
    root;                            // root dir of SD card
uint8_t
    nFiles = 0,                      // total txt file numbers in SD card
    cIndex = 0,                      // current file index
    LIGHT  = 0;                      // light level
uint16_t
    nLEDs  = 80,                     // pixel number
    tStep  = 10;                     // time interval, this controls displaying speed
bool
    STATE  = false,                  // on-off state of running
    TOUCH  = false;                  // on-off state of touch detection
char*
    fileList[32];                    // storage all cmd file names
/*
now that char * is a pointer variable -- it is a int variable which storages an index pointing to a string -- we have:
char* a = "hello";
a --> 0x00000123456
*a --> 'h'
*(a+1) --> 'e'
"%s",a --> "hello"
so we can use char *a[] to storage a list of pointers which point to many strings' first char.
Usually cmd files will be no more than 32, if needed you can give it more space ;-)
*/
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(nLEDs, pixelPin, NEO_KHZ800 + NEO_GRB);


// functions
void
    HELP(),                           //
    initSD(),                         //
    display(),                        //
    listenSerial(),                   // handle all serial info
    allPixel(uint8_t c=0),            // turn all pixel off
    pixelCmd(),                       // interactive mode
    bar(float),                       // showing effect of progress bar or music amptitude
    rainbow(uint8_t),                 // embedded beautiful color pattern
    receiveFile(bool writeSD=false);  // receive commands from serial
uint32_t
    Wheel(byte);                      // work with "void rainbow(uint8_t)"
File
    currentFile(File, char *);        // find current cmd file with correspond filename and return this file
uint8_t
    touchDetect(int pin=touchPin),    // return int 1-10 if pin is been touched else 0
    listFile(File, uint8_t, uint8_t); // list all files in SD card


void HELP(){
    // it would be better keep 'No more info output' to avoid RAM crash caused by too many string.
    ///*
    // uncomment this part for nano | uno
    myport.println(F("PixelStick by hank"));
    myport.println(F("Version: 1.1.0 https://github.com/hankso/MyPixelStick"));
    myport.println(F("Usage: [state|next|prev|push|ls|ips|pixel|touch|clear|time|bar|light]"));
    //*/
    /*
    // uncomment this part for mega2560
    myport.println(F("PixelStick by hank https://github.com/hankso/MyPixelStick"));
    myport.println(F("Version: 1.1.0"));
    myport.println(F("Usage:"));
    myport.println(F("    help:   show this message"));
    myport.println(F("    state:  set running state with 'state on'|'state off'"));
    myport.println(F("    next:   display next photo(only works with SD card)"));
    myport.println(F("    prev:   display prev photo(only works with SD card)"));
    myport.println(F("    push:   start sending data from serial"));
    myport.println(F("    ls:     list files"));
    myport.println(F("    ips:    REPL mode. ips(interactive pixelstick). control pixels as format (i r g b)"));
    myport.println(F("    pixel:  set num of pixels(e.g. 'pixel 30'|'pixel 50')"));
    myport.println(F("    touch:  open or close touch-detect function at pin 10(default) with 'touch on'|'touch off'"))
    myport.println(F("    clear:  turn off all pixels"));
    myport.println(F("    time:   set time-duration(e.g. 'time-10'|'time+50'|'time=20'|'time 50')"));
    myport.println(F("    bar:    show as process bar or music amptitude"));
    myport.println(F("    light:  turn on all pixels as light(e.g. 'light 10'|'light 255')"));
    */
}

void allPixel(uint8_t c){
    for (int i = 0; i < nLEDs; i++)
        pixel.setPixelColor(i, c, c, c);
    pixel.show();
}

void initSD(){
    myport.print("Loading SD card: ");
    if (!SD.begin(SDPin)){
        myport.println("failed, maybe not inserted.");
        return;
    }
    myport.println("done!");

    root = SD.open("/");
    nFiles = listFile(root, 0, 0);
    myport.printf("ls finished, found %d cmd files, current file: %s\n", nFiles, fileList[cIndex]);
    // for(int i = 0; i < nFiles; i++) myport.printf("%s\n",fileList[i]);
}

uint8_t listFile(File r, uint8_t count, uint8_t numTabs) {
    // please use '    ' instead of '\t' because in some cases length of '\t' is uncertainty
    if (!r) {
        myport.println(F("\nlistFile error: dir unreadable, terminated.\n"));
        return count;
    }
    delay(200);
    if (numTabs == 0) myport.println(r.name());
    while (true) {
        File file = r.openNextFile();
        if (!file) break;
        delay(200);
        for (uint8_t i = 0; i < numTabs; i++) myport.print("|   ");
        if (file.isDirectory()) {
            myport.printf("|---%s/\n", file.name());
            count = listFile(file ,count, numTabs + 1);
        }
        else{
            String temp = file.name();
            temp.toLowerCase();
            if (temp.endsWith(".cmd")) {
                if (cIndex == count)
                    myport.printf("| > %2d ", count);
                else
                    myport.printf("|   %2d ", count);
                myport.println(temp + "  " + String(file.size()/1024.0) + "KB");
                // TODO debug this
                // file_list[count] = const_cast<char*>(temp.c_str());
                count++;
            }
            else myport.println("|   " + temp);
        }
        file.close();
    }
    return count;
}

void pixelCmd() {
    int i;
    uint8_t r, g, b;
    while (true) {
        myport.print("ips> ");
        /*
        To avoid parseInt() return 0 because of '\r','\n'
        especially when receving "\r\n", set threshold to 3.
        so that it can handle both "1\r\n2\r\n 3\r\n 4\r\n"
        and "1\n2\n3\n4\n"
        */
        while (myport.available() < 3) {delay(100);}
        i = myport.parseInt();
        if (i == -1) break;
        myport.printf("%3d ", i);
        while (myport.available() < 3) {delay(100);}
        r = myport.parseInt();
        myport.printf("%3d ", r);
        while (myport.available() < 3) {delay(100);}
        g = myport.parseInt();
        myport.printf("%3d ", g);
        while (myport.available() < 3) {delay(100);}
        b = myport.parseInt();
        myport.printf("%3d \n", b);
        pixel.setPixelColor(i, r, g, b);
        pixel.show();
    }
    myport.println("terminated('-1' detected)");
    // "-1 -1 -1 -1\r\n" clean serial buff
    while (myport.available()) {myport.read();}
    allPixel();
}

void bar(float rate) {
    if (rate <= 1 && rate >= 0) {
        uint8_t n = nLEDs * rate;
        pixel.setPixelColor(n, 10, 200, 10);
        while (n > 0)
            pixel.setPixelColor(--n, 128, 128, 128);
        pixel.show();
        delay(1000);
        allPixel();
    }
}

void rainbow(uint8_t wait) {
    int i, j;
    for(j = 0; j < 256; j++) {
        for(i = 0; i < nLEDs; i++)
            pixel.setPixelColor(i, Wheel((i + j) & 255));
        pixel.show();
        delay(wait);
    }
}

uint32_t Wheel(byte WheelPos) {
      WheelPos = 255 - WheelPos;
      if (WheelPos < 85)
          return pixel.Color((255 - WheelPos*3)/3, 0, (WheelPos*3)/3);
      if (WheelPos < 170){
          WheelPos -= 85;
          return pixel.Color(0, (WheelPos*3)/3, (255 - WheelPos*3)/3);
      }
      else{
          WheelPos -= 170;
          return pixel.Color((WheelPos*3)/3, (255 - WheelPos*3)/3, 0);
      }
}

uint8_t touchDetect(int pin) {
    byte bitmask = digitalPinToBitMask(pin);
    volatile uint8_t* PORT = portOutputRegister(digitalPinToPort(pin));
    volatile uint8_t* PIN  = portInputRegister(digitalPinToPort(pin));
    volatile uint8_t* DDR  = portModeRegister(digitalPinToPort(pin));
    *DDR |= bitmask; *PORT &= ~(bitmask);
    delay(1);
    *DDR &= ~(bitmask); *PORT |= bitmask;
    uint8_t times = 0; while (!(*PIN & bitmask)) {times++;}
    *DDR |= bitmask; *PORT &= ~(bitmask);
    return times;
}

File currentFile(File r, char * filename) {
    File null;
    if (!r) {return null;}
    while (true) {
        File file = r.openNextFile();
        if (!file) break;
        if (file.isDirectory()) {
            File next = currentFile(file, filename);
            if (next) return next;
        }
        else {
            if (String(file.name())==String(filename))
                return file;
        }
        file.close();
    }
    return null;
}

void display() {
    if (!root) rainbow(5);
    else {
        data = currentFile(root, fileList[cIndex]);
        if (!data) {
            myport.printf("Error when trying to open file %s\n", data.name());
            data.close();
        }
        else {
            data.readStringUntil('#');
            data.readStringUntil('#');
            // int i, r, g, b;
            uint8_t i;
            uint32_t temp;
            while (data.available()) {
                while ( (i = (temp = data.parseInt())>>24) >= nLEDs ) {} //doing_nothing
                /*
                r = data.parseInt();
                g = data.parseInt();
                b = data.parseInt();
                pixel.setPixelColor(i,
                                    data.parseInt(),
                                    data.parseInt(),
                                    data.parseInt());
                */
                pixel.setPixelColor(i, temp & 0xffffff);
                if (i == nLEDs - 1) {
                    pixel.show();
                    myport.println(millis());
                    delay(tStep);
                }
            }
            allPixel();
            data.close();
        }
        STATE = false;
    }
}

void receiveFile(bool writeSD){
    /*
    Param writeSD used for debugging, I'd like to test whether
    realtime uploading data works: I'm afraid serial is a little
    bit slow to realtime display, let's have a try...

    I insist to use 'index r g b' instead of just 'r g b' because
    it can
    1. not only prevent chaos pixel color result from packet loss
    2. but also skip unnecessary color change
       (e.g. data '5 100 100 100' will not be transferred if pixel
        5 has already been set to '100 100 100')

    It seems that realtime performs not so good, but i got an idea:
    Inspired by "Adafruit_NeoPixel.h/uint32_t color()", we can use
    uint32_t to storage info like this:
        uint32_t 85207100
      = bin 00000101000101000010100000111100
      = bin 00000101<<24|00010100<<16|00101000<<8|00111100
      = uint8_t    5<<24|      20<<16|      40<<8|      60
      = i:5 r:20 g:40 b:60
    So we only need one number(uint32_t) to set color of a pixel,
    which means using "parseInt" only one time. That saves time.

    However, index takes 8 bit space of arduino and if
    nLEDs > 255, eh...... try uint64_t please  _(:3 」∠)_
    */
    if (writeSD && !root) writeSD = false; // in case SD is not inserted
    /*
    'readStringUntil' has a relatively short timeout
    we need time to choose which file to send, etc
    so use while(){} to let stick wait
    */
    while (myport.read()!='#') {}
    String filename = myport.readStringUntil('#');
    myport.println("Receiving " + String(filename));
    File f;
    if (writeSD) f = SD.open(filename, FILE_WRITE);
    delay(10);
    uint32_t temp;
    uint8_t i;
    myport.println("Time(ms) i  temp  available");
    while (true){
        temp = myport.parseInt();
        i = temp >> 24;
        // myport.printf("%d %d %d %d\n", millis(), i, temp, myport.available());
        myport.printf("%6ld %3d ", millis(), i);
        myport.printf("%3d %3d %3d\n", (temp>>16)&0xff, (temp>>8)&0xff, temp&0xff);
        if (i < nLEDs) pixel.setPixelColor(i, temp & 0xffffff);
        if (writeSD) f.println(temp);
            // f.printf("%d %d %d %d\n", i, r, g, b);
        if (i == nLEDs - 1){
            pixel.show();
            myport.println(millis());
            delay(tStep);
        }
        if (temp == 4294967295) // uint32_t storage 4294967295 as -1(-1 means end)
        {
            if (myport.parseInt() == -1) // double check
            {
                myport.println("terminate");
                while(myport.available()) myport.read(); // clear buff
                break;
            }
        }
    }
    if (writeSD){
        f.println("-1-1-1-1");
        f.close();
        nFiles = listFile(root, 0, 0);
    }
    delay(1000);
    allPixel();
}

void listenSerial(){
    if (myport.available()){
        String msg = myport.readStringUntil('\n');
        if (msg.endsWith(String('\r'))) msg = msg.substring(0, msg.length()-1);
        if (msg.length()){
            myport.println(msg);
            delay(200);
            if (msg.startsWith("pixel")) {
                if (msg.length() > 5) {
                    if (msg.substring(5).toInt() > 0){
                        nLEDs = msg.substring(5).toInt();
                        pixel.updateLength(nLEDs);
                    }
                }
                myport.println("nLEDs: " + String(nLEDs));
            }
            else if (msg.startsWith("time")) {
                if (msg.length() > 4) {
                    int n = msg.substring(4).toInt();
                    if ((msg.indexOf('=') > 0 || msg.indexOf('+') == -1)
                        && msg.indexOf('-') == -1) // "time=10"|"time = 10"|"time 10" can be recognized here
                        tStep =  n;
                    else
                        tStep += n;
                    if (tStep < 0) tStep = 0;
                }
                myport.println("tStep: " + String(tStep));
            }
            else if (msg.startsWith("state")) {
                if (msg.length() > 6) {
                    if      (msg.substring(6) == "on")  STATE = true;
                    else if (msg.substring(6) == "off") STATE = false;
                }
                myport.println("state: " + String(STATE));
            }
            else if (msg.startsWith("touch")) {
                if (msg.length() > 6){
                    if      (msg.substring(6) == "on")  TOUCH = true;
                    else if (msg.substring(6) == "off") TOUCH = false;
                }
                myport.println("TOUCH: " + String(TOUCH));
            }
            else if (msg == "push") {
                receiveFile();
                myport.println(F("push done. Thank you"));
            }
            else if (msg == "clear") {
                allPixel();
                myport.println(F("all pixels clear now"));
            }
            else if (msg == "next"){
                cIndex++; listFile(root, 0, 0);
                myport.printf("current file: %s\n", fileList[cIndex]);
            }
            else if (msg == "prev"){
                cIndex--; listFile(root, 0, 0);
                myport.printf("current file: %s\n", fileList[cIndex]);
            }
            else if (msg.startsWith("light")) {
                if (msg.length() > 5) allPixel(LIGHT = msg.substring(5).toInt());
                myport.println("light: " + String(LIGHT));
            }
            else if (msg == "ips")
                pixelCmd();
            else if (msg.startsWith("bar") && msg.length() > 3)
                bar(abs(msg.substring(3).toFloat()));
            else if (msg == "ls")
                nFiles = listFile(root, 0, 0);
            else if (msg == "help")
                HELP();
            else myport.println(msg + ": command not found");
        }
        myport.print("\n>>> ");
    }
    else delay(200);
}


void setup(){
    myport.begin(BT_baud);
    myport.print(F("Initializing"));
    delay(100); myport.print(".");
    delay(100); myport.print(".");
    delay(100); myport.println(".");

    initSD();

    pixel.begin();
    pixel.show();

    allPixel(0);

    myport.print(F("\nTry 'help' for more information.\n>>> "));
}

void loop(){
    listenSerial();

    if (STATE) display();

    if(TOUCH) {
        if (touchDetect(touchPin) > 5){
            allPixel(LIGHT+=Lighterval);
            myport.println("light: " + String(LIGHT));
            delay(3000);
        }
    }
}
