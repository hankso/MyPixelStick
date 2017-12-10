/*
# MyPixelStick by hank
Date: 2017.11.8

In this version it can read commands from SD card or receive color data via blueteeth

### Pin define
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

### Data format
-Command file should have header and end like this:
    - #file.txt#               // '#' + filename + '#'
    - uint32_t data
    - uint32_t data
    - ......
    - -1-1-1-1...              // more than two -1 for ensurance
- serial.parseInt() will return '0' if timeout, which may be
misunderstanded as '0 0 0 0', so use '-1' as an indicator of end.

### printf
There doesn't exist printf("%d",int) in arduino...
In order not to use:
    sprintf (buffer, "you send %d something here", foo);
    Serial.print (buffer);
I add some code to $ARDUINO_DIR/hardware/arduino/cores/Print.h
more details at 'http://playground.arduino.cc/Main/Printf'

-------------------------------------------
TODO: acceleration sensor such as 'MMA7631'
to change the t_Step(time step)
i.e. horizontal pixel interval of an image
*/

#include <Adafruit_NeoPixel.h>
#include <SD.h>
//#include <SoftwareSerial.h>

#define SDPin      4
#define PixelPin   6
#define BT_baud    115200
#define myport     Serial

File data;              // current data file
File root;              // root dir of SD card
uint8_t  n_Files = 0;   // total txt file numbers in SD card
uint8_t  c_Index = 0;   // current file index
uint16_t n_LEDs  = 144; // pixel number
uint16_t t_Step  = 10;  // time interval, this controls displaying speed
bool RUN = false;       // soft on-off switch
String FileNames;       // this string storage all txt file names
// About String FileNames:
// I dont know how to create an appendable list of String in C++
// (like "file_list.append('filename')" in python),
// so I storage all filenames into this variable
// FileNames = "#1file#2file#3file......#nfile"
// Maybe something like "char *stringList" will be more elegant

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(n_LEDs, PixelPin, NEO_KHZ800 + NEO_RGB);

void HELP();                           //
void init_SD();                        //
void display();                        //
void listen_serial();                  // handle all serial info
void clear_pixel();                    // turn all pixel off
void pixel_cmd();                      // interactive mode
void bar(float);                       // showing effect of progress bar or music amptitude
void rainbow(uint8_t);                 // embedded beautiful color pattern
void receive_file(bool);               // receive commands from serial
uint8_t list_file(File, uint8_t);      // list all files in SD card and return numbers of all
uint32_t Wheel(byte);                  // work with "void rainbow(uint8_t)""
String get_filename(uint8_t*, String); // return current cmd file name


void setup(){
    myport.begin(BT_baud);
    myport.print("Initializing");
    delay(100); myport.print(".");
    delay(100); myport.print(".");
    delay(100); myport.println(".");

    pinMode(10, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(5,  INPUT);

    pixel.begin();
    pixel.show();

    init_SD();
    myport.print("Try 'help' for more information.\n>>> ");
}

void loop(){
    listen_serial();
    if (RUN) display();

    // use as light
    if (digitalRead(5)){
        for(int i; i < n_LEDs; i++)
            pixel.setPixelColor(i, 128, 128, 128);
        pixel.show();
        delay(1000*10);
    }
}

void HELP(){
    // No more info output unless you know how to avoid
    // crash caused by 'too many' serial output.
    ///*
    // uncomment this part for nano
    myport.println("PixelStick by hank");
    myport.println("Version: 1.0.1 https://github.com/hankso/MyPixelStick");
    myport.println("Usage: [run|stop|next|prev|push|ls|ips|pixel|clear|time]");
    //*/
    /*
    // uncomment this part for uno
    myport.println("PixelStick by hank https://github.com/hankso/MyPixelStick");
    myport.println("Version: 1.0.1");
    myport.println("Usage:");
    myport.println("    help:   show this message");
    myport.println("    run:    start displaying");
    myport.println("    stop:   stop displaying");
    myport.println("    next:   display next photo(only works with SD card)");
    myport.println("    prev:   display prev photo(only works with SD card)");
    myport.println("    push:   start sending data from serial");
    myport.println("    ls:     list files");
    myport.println("    ips:    interactive pixelstick. control pixels in format (i r g b)");
    myport.println("    pixel:  set num of pixels(e.g. 'pixel 30'|'pixel 50')");
    myport.println("    clear:  turn off all pixels");
    myport.println("    time:   set time-duration(e.g. 'time-10'|'time+50'|'time=20')");
    */

}

void init_SD(){
    myport.print("Loading SD card: ");
    if (SD.begin(SDPin)){
        myport.println("done!");
        root = SD.open("/");
        n_Files = list_file(root, 0);
    }
    myport.println("failed, maybe not inserted.");
}

uint8_t list_file(File r = root, uint8_t numTabs = 0){
    if (!r){
        myport.println("list_file error: dir unreadable, terminated.");
        return 0;
    }
    uint8_t count = 0;
    File f;
    while (true){
        f = r.openNextFile();
        if (!f) break;
        for (uint8_t i = 0; i < numTabs; i++){
            myport.print("\t");
        }
        if (f.isDirectory()){
            myport.println(String(f.name()) + "/");
            list_file(f, numTabs + 1);
        }
        else{
            if (String(f.name()).endsWith("txt")){
                // record file into FileNames with '#' indicator
                FileNames += "#" +
                             String(count) +
                             r.name() +
                             "/" + f.name();
                if (c_Index == count){
                    myport.printf("  > %2d ", count);
                    myport.print(String(f.name()) + "\t");
                }
                else{
                    myport.printf("\t%2d ", count);
                    myport.print(String(f.name()) + "\t");
                }
                myport.println(f.size(), DEC);
                count++;
            }
            else{
                myport.println("\t   " +
                               String(f.name()));
            }
        }
        f.close();
    }
    r.rewindDirectory();
    return count;
}

String get_filename(uint8_t* index, String namelist){
    if (!root) return "No SD card.";
    // FileNames : "#1filename#2filename......#nfilename"
    // FileNames.indexOf("#n") --> index of "#n"
    // FileNames.indexOf("#", index) --> index of "#n+1"
    *index = *index > n_Files ?
              n_Files : (*index < 0 ? 0 : *index);
    int i = namelist.indexOf("#" + String(*index));
    int j = namelist.indexOf("#", i);
    return namelist.substring(i, j);
}

void clear_pixel(uint8_t value = 0){
    for (int i = 0; i < n_LEDs; i++)
        pixel.setPixelColor(i, value, value, value);
    pixel.show();
}

void pixel_cmd(){
    int i;
    uint8_t r, g, b;
    while (true){
    	myport.print("ips> ");
    	// To avoid parseInt() return 0 because of '\r','\n'
    	// especially when receving "\r\n", set threshold to 3.
    	// so that it can handle both "1\r\n2\r\n 3\r\n 4\r\n"
    	// and "1\n2\n3\n4\n"
        while (myport.available() < 3){delay(100);}
        i = myport.parseInt();
        if (i == -1) break;
        myport.printf("%3d ", i);
        while (myport.available() < 3){delay(100);}
        r = myport.parseInt();
        myport.printf("%3d ", r);
        while (myport.available() < 3){delay(100);}
        g = myport.parseInt();
        myport.printf("%3d ", g);
        while (myport.available() < 3){delay(100);}
        b = myport.parseInt();
        myport.printf("%3d \n", b);
        pixel.setPixelColor(i, r, g, b);
        pixel.show();
    }
    myport.println("terminated('-1' detected)");
    // "-1 -1 -1 -1\r\n" clean serial buff
    while(myport.available()){myport.read();}
    clear_pixel();
}

void display(){

    if (!root)
        rainbow(5);
    else{
        String filename = get_filename(&c_Index, FileNames);
        char t[filename.length()];
        filename.toCharArray(t, filename.length());
        data = SD.open(t);

        if (!data)
            myport.println("Error when trying to open file " +
                           String(data.name()));
        else{
            data.readStringUntil('#');
            data.readStringUntil('#');
            // int i, r, g, b;
            uint8_t i;
            uint32_t temp;
            while (data.available()){
                while ( (i = (temp = data.parseInt())>>24) >= n_LEDs ){} //doing_nothing
                /*
                r = data.parseInt();
                g = data.parseInt();
                b = data.parseInt();
                pixel.setPixelColor(i,
                                    data.parseInt(),
                                    data.parseInt(),
                                    data.parseInt());*/
                pixel.setPixelColor(i, temp & 0xffffff);
                if (i == n_LEDs - 1){
                    pixel.show();
                    myport.println(millis());
                    delay(t_Step);
                }
            }
            clear_pixel();
        }
    }
}

void receive_file(bool writeSD = false){
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
    n_LEDs > 255, eh...... try uint64_t please  _(:3 」∠)_
    */
    if (!root) writeSD = false; // in case SD is not inserted

    // 'readStringUntil' has a relatively short timeout
    // we need time to choose which file to send, etc
    // so use while(){} to let stick wait
    while (myport.read()!='#'){}
    String filename = myport.readStringUntil('#');
    myport.println("Receiving " + String(filename));
    File f;
    if (writeSD){
        // I was totally shocked!!!!!!!!!
        // "SD.open(const char*, uint8_t)" only accept char list
        // So I can not use "SD.open(String filename)"!!!
        // C++ really teachs me how to be a man
        char t[filename.length()];
        filename.toCharArray(t, filename.length());
        f = SD.open(t, FILE_WRITE);
    }
    delay(10);
    /*
    int i, r, g, b;
    myport.println("  i   r   g   b");
    */
    uint32_t temp;
    uint8_t i;
    while (true){
        temp = myport.parseInt();
        i = temp >> 24;
        myport.printf("%d %d %d %d\n", millis(), i, temp, myport.available());
        // myport.print(String(millis())+" "+String(i)+"\n");
        if (i < n_LEDs) pixel.setPixelColor(i, temp & 0xffffff);
        if (writeSD)
            // f.printf("%d %d %d %d\n", i, r, g, b);
            f.println(temp);
        // myport.printf("%3d %3d %3d %3d\n", i, r, g, b);
        if (i == n_LEDs - 1){
            pixel.show();
            myport.println(millis());
            delay(t_Step);
        }
        if (temp == 4294967295) // uint32_t storage 4294967295 as -1(-1 means end)
        {
            if (myport.parseInt() == -1) // double check
            {
                myport.println("terminate");
                // clear buff
                while(myport.available()){myport.read();}
                break;
            }
        }
    }

    if (writeSD){
        f.println("-1-1-1-1");
        f.close();
        n_Files = list_file(root);
    }
	delay(1000);
    clear_pixel();
}

void listen_serial(){
    if (myport.available()){
        String msg = myport.readStringUntil('\n');
        if (msg.endsWith(String('\r'))) msg = msg.substring(0, msg.length()-1);
        if (msg.length()){
            myport.println(msg);
            delay(200);

            if (msg.startsWith("time")){
                if (msg.length() > 4){
                    int n = msg.substring(4).toInt();
                    //  "time=10"|"time = 10"|"time 10" can be recognized here
                    if ((msg.indexOf('=') > 0 || msg.indexOf('+') == -1)
                        && msg.indexOf('-') == -1)
                        t_Step = n;
                    else
                        t_Step += n;
                    if (t_Step < 0) t_Step = 0;
                }
                myport.println("t_Step: " + String(t_Step));
            }
            else if (msg.startsWith("pixel")){
                myport.println("fixing bug...");
                /*
                if (msg.length() > 5){
                    if (msg.substring(5).toInt() > 0){
                        n_LEDs = msg.substring(5).toInt();
                        Adafruit_NeoPixel pixel = Adafruit_NeoPixel(n_LEDs, PixelPin, NEO_KHZ800 + NEO_RGB);
                        pixel.begin();
                        pixel.show();
                        clear_pixel();
                    }
                }
                */
                myport.println("n_LEDs: " + String(n_LEDs));
            }
            else if (msg == "run") {
                if (!root) init_SD();
                RUN = true;
                myport.println("RUN state: " + String(RUN));
            }
            else if (msg == "stop"){
                RUN = false;
                myport.println("RUN state: " + String(RUN));
            }
            else if (msg == "clear"){
                clear_pixel();
                myport.println("All pixels clear now");
            }
            else if (msg == "ips")
                pixel_cmd();
            else if (msg == "push"){
                receive_file();
                myport.println("push done. Thank you.");
            }
            else if (msg == "ls")
                list_file();
            else if (msg == "next")
                myport.println("current file: " + get_filename(&++c_Index, FileNames));
            else if (msg == "prev")
                myport.println("current file: " + get_filename(&--c_Index, FileNames));
            else if (msg == "help")
                HELP();
            else myport.println(msg + ": command not found");
        }
        myport.print("\n>>> ");
    }
    else{
        //Serial.println(millis());
        delay(200);
    }
}

void bar(float rate){
    if (rate <= 1){
        uint8_t n = n_LEDs * rate;
        pixel.setPixelColor(n, 10, 200, 10);
        while (n > 0)
            pixel.setPixelColor(--n, 128, 128, 128);
        pixel.show();
    }
}

void rainbow(uint8_t wait){
    int i, j;
    for(j = 0; j < 256; j++){
        for(i = 0; i < n_LEDs; i++)
            pixel.setPixelColor(i, Wheel((i + j) & 255));
        pixel.show();
        delay(wait);
  }
}

uint32_t Wheel(byte WheelPos){
      WheelPos = 255 - WheelPos;
      if (WheelPos < 85)
          return pixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
      if (WheelPos < 170){
          WheelPos -= 85;
          return pixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
      else{
          WheelPos -= 170;
          return pixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
      }
}
