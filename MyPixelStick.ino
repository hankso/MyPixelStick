/*
Written by Hank to DIY a pixelstick
Date: 2017.11.8

It can read images from SD card or just receive pixel data via blueteeth

* SD card attached as follows:
** MOSI - pin 11
** MISO - pin 12
** CLK - pin 13
** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

PixelStick Data Pin 6

On Mega, BT softserial should use another pin because 8, 9 won't work

Txt file containing data must have a header like this:
    ##filename.txt##
    index r g b
    index r g b
    ......
or
    ##filename.txt#
    index,r,g,b
    index,r,g,b
    ......
*/

#include <Adafruit_NeoPixel.h>
#include <SD.h>
//#include <SoftwareSerial.h>

#define buttonBT   2
#define buttonNext 3
#define SDPin      4
#define PixelPin   6
#define n_LEDs     30

#define BT_baud    115200
// connect RX0 to BT_TX and TX1 to BT_RX
// remember to set BT baudrate by AT commands
#define myport Serial 
// bluetooth softwareserial - RX 8, TX 9
//SoftwareSerial myport(8, 9);

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(n_LEDs, PixelPin, NEO_KHZ800 + NEO_RGB);

File data; // current data file
File root; // root dir of SD card

// TODO: acceleration sensor such as 'MMA7631'
// to change the t_Step(time step)
// i.e. horizontal pixel interval of an image
int t_Step = 50;
int n_Files = 0; // total txt file numbers in SD card
int c_Index = 0; // current file index

bool RUN = true; // soft on-off switch

// this string storage all txt file names
// used for turning to next file
// because I dont know how to create an
// realtime appendable list of String in C++
// (maybe something like "char *stringList" ?)
String FileNames;


// Declaration
void init_SD();
int listFiles(File, int);
String getFilename(int*);
void display();
void receiveFile(bool);
void handleSerial();
void rainbow(uint8_t);
uint32_t Wheel(byte);
void bar(float);
void HELP();

void setup(){
    //Serial.begin(115200);
    //Serial.write('#');
    // blueteeth baudrate set to 115200
    myport.begin(BT_baud);
    myport.println("\nInitializing...");

    pinMode(buttonBT,    INPUT);
    pinMode(buttonNext,  INPUT);
    pinMode(10,          OUTPUT);
    pinMode(13,          OUTPUT);

    pixel.begin();
    pixel.show();

    init_SD();
    if (!root) RUN = false;
    
    myport.println("type 'help' for more information");
}

void loop(){
    handleSerial();
    if (RUN){
        display();
    }
}




void init_SD(){
    myport.print("SD card loading... ");
    if (SD.begin(SDPin)){
        myport.println("Done!");
        root = SD.open("/");
        n_Files = listFiles(root, 0);
    }
    myport.println("failed, maybe not inserted.");
}

int listFiles(File r = root, int numTabs = 0){
    if (!r){
        myport.println("Listfiles error: dir unreadable, terminated.");
        return 0;
    }
    // list all files in SD card and return numbers of all
    int count = 0;
    File f;

    while (true){
        f = r.openNextFile();
        if (!f) break;
        for (int i = 0; i < numTabs; i++){
            myport.print("\t");
        }
        if (f.isDirectory()){
            myport.println(String(f.name()) + "/");
            listFiles(f, numTabs + 1);
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

String getFilename(int* index){
    // FileNames : "#1filename#2filename......#nfilename"
    // FileNames.indexOf("#n") --> index of "#n"
    // FileNames.indexOf("#", index) --> index of "#n+1"
    *index = *index > n_Files ?
              n_Files : (*index < 0 ? 0 : *index);
    int i = FileNames.indexOf("#" + String(*index));
    int j = FileNames.indexOf("#", i);
    return FileNames.substring(i, j);
}

void display(){
    if (!root) rainbow(5);
    else{

        String filename = getFilename(&c_Index);
        int l = filename.length();
        char t[l];
        filename.toCharArray(t, l);
        data = SD.open(t);
        if (data){
            data.readStringUntil('#');
            data.readStringUntil('#');
            //int i, r, g, b;
            int i;
            while (data.available()){
                while (i = data.parseInt() >= n_LEDs){
                    data.readStringUntil('\n');
                }
                // r = data.parseInt();
                // g = data.parseInt();
                // b = data.parseInt();

                pixel.setPixelColor(i,
                                    data.parseInt(),
                                    data.parseInt(),
                                    data.parseInt());
                if (i == n_LEDs - 1){
                    pixel.show();
                    delay(t_Step);
                }
            }
        }
        else
            myport.println("Error when trying to open file " +
                           String(data.name()));

    }
}

void receiveFile(bool writeSD = false, bool echo = true){
    // param writeSD used for debugging
    // I'd like to test whether realtime uploading data works
    // I'm afraid serial is a little bit slow to realtime display
    // let's have a try
    
    myport.println("Waiting for data...");

    // in case SD is not inserted
    if (!root) writeSD = false;
    //   i   r   g   b
    // 999 255 255 255
    if (echo) myport.println("  i   r   g   b");

    while (myport.read()!='#'){}
    // 'readStringUntil' has a relatively short timeout
    // we need time to choose which file to send etc.
    //myport.readStringUntil('#');

    String filename = myport.readStringUntil('#');
    File f;
    if (writeSD){
        // I was totally shocked!!!!!!!!!
        // "SD.open(const char*, uint8_t)" only accept char*
        // So I can not use "SD.open(String filename)"!!!
        // What a fucking function, I HATE strongly-typed language
        // It really teach me how to be a man
        int l = filename.length();
        char t[l];
        filename.toCharArray(t, l);
        f = SD.open(t, FILE_WRITE);
    }
    myport.println("Receiving " + String(filename));
    delay(200);

    int i, r, g, b;
    while (true){
        i = myport.parseInt();
        r = myport.parseInt();
        g = myport.parseInt();
        b = myport.parseInt();

        // display
        if (i < n_LEDs) pixel.setPixelColor(i, r, g, b);

        // write in SD
        // There doesn't exist printf("%d,%d,%d,%d",int,int,int,int) in arduino...
        // Ahhhhh...you really beat me
        // I add some code to $ARDUINO_DIR/hardware/arduino/cores/Print.h
        // more details at "h ttp://forum.arduino.cc/index.php?topic=149785.0"
        if (writeSD)   f.printf("%d %d %d %d\n", i, r, g, b);
        if (echo) myport.printf("%3d %3d %3d %3d\n", i, r, g, b);

        if (i == n_LEDs - 1){
            pixel.show();
            delay(t_Step);
        }
        
        if (i == 255){
            if (myport.read() == '#'     && 
                myport.parseInt() == 255 &&
                myport.read() == '#')
            {
                myport.println("Terminate.");
                break;
            }
            else myport.readStringUntil('\n');
        }
    }

    if (writeSD){
        f.close();
        n_Files = listFiles(root);
    }
}

void handleSerial(){
    if (myport.available()){
        //myport.println(myport.readStringUntil('\n'));
        String msg = myport.readStringUntil('\n');
        //myport.println("You sent " + msg);
        myport.println(">>> " + msg);
        if (msg.endsWith(String('\r'))) msg = msg.substring(0, msg.length()-1);
       	
        
        if (msg.startsWith("time") && msg.length() > 4){
            t_Step += msg.substring(4).toInt();
            if (t_Step < 0) t_Step = 0;
            myport.println("t_Step set to " + String(t_Step));
        }
        
        else if (msg == "run") {
            if (!root){
                init_SD();
                if (root) RUN = true;
                else myport.println("Keep in rest.");
            }
            else RUN = true;
            if (RUN) myport.println("RUN state: " + String(RUN));
        }
        
        else if (msg == "stop"){
            RUN = false;
            myport.println("RUN state: " + String(RUN));
        }
        else if (msg == "clear"){
            for (int i = 0; i < n_LEDs; i++)
                pixel.setPixelColor(i, 0, 0, 0);
            pixel.show();
        }
        else if (msg == "pixel"){
            while (myport.available() < 7){}
            int i = myport.parseInt();
            int r = myport.parseInt();
            int g = myport.parseInt();
            int b = myport.parseInt();
            myport.printf("%d %d %d %d",i, r, g, b);
            pixel.setPixelColor(i, r, g, b);
            pixel.show();
        }
        else if (msg == "push")
            receiveFile();
        else if (msg == "ls")
            listFiles();
        else if (msg == "next")
            myport.println("current file: " + getFilename(&++c_Index));
        else if (msg == "prev")
            myport.println("current file: " + getFilename(&--c_Index));
        else if (msg == "help"){
            HELP();
        }
        
        else myport.println(msg + ": command not found\nTry 'help' for more infomation.");
    }
    else{
        //Serial.println(millis());
        delay(200);
    }
}


// showing effect of progress bar or music amptitude
void bar(float rate){
    if (rate <= 1){
        int n = n_LEDs * rate;
        pixel.setPixelColor(n, 10, 200, 10);
        while (n > 0)
            pixel.setPixelColor(--n, 128, 128, 128);
        pixel.show();
        delay(100);
    }
}

// rainbow effect from example of library NeoPixel.
void rainbow(uint8_t wait){
    uint16_t i, j;

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

void HELP(){
    // No more info output unless you know how to avoid
    // crash caused by 'too many' serial output.
    
    ///*
    // uncomment this part for nano
    myport.println("PixelStick by hank");
    myport.println("Version: 1.0.0 https://github.com/hankso/pixelstick");
    myport.println("Available commands: [run|stop|next|prev|push|ls|pixel|clear]");
    //*/
    
    /*
    // uncomment this part for uno
    myport.println("PixelStick by hank https://github.com/hankso/pixelstick");
    myport.println("Version: 1.0.0");
    myport.println("Usage:");
    myport.println("    help:   show this message");
    myport.println("    run:    start displaying");
    myport.println("    stop:   stop displaying");
    myport.println("    next:   display next photo(only works with SD card)");
    myport.println("    prev:   display previous photo(only works with SD card)");
    myport.println("    push:   start sending data");
    myport.println("    ls:     list files");
    myport.println("    pixel:  control a single pixel with latter input (i r g b)");
    myport.println("    clear:  set all pixels to black(0, 0, 0)");
    myport.println("    time+n: plus or minus a number to time-duration(e.g.'time-10'|'time+50'|)");
    */
    
}
