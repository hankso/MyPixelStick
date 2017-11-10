
/*******************************************************************************
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
*******************************************************************************/

#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <SoftwareSerial.h>

#define buttonBT   2
#define buttonNext 3
#define SDPin      4
#define PixelPin   6
#define n_LEDs     30

SoftwareSerial myport(8, 9); // blueteeth softwareserial - TX 8, RX 9
Adafruit_NeoPixel pixel(n_LEDs, PixelPin, NEO_KHZ800 + NEO_RGB);
File data; // current data file
File root; // root dir of SD card
int n_Files; // total txt file numbers in SD card
int c_Index = 0; // current file index
bool RUN = true; // soft on-off switch

// TODO: acceleration sensor such as 'MMA7631'
// to change the t_Step(time step)
// i.e. horizontal pixel interval of an image
int t_Step = 100;

// this string storage all txt file names
// used for turning to next file
// because I dont know how to create an
// realtime appendable list of String in C++
// (maybe something like "char *stringList" ?)
String FileNames;

String HELP = "
PixelStick
Version: 1.0.0  https://github.com/hankso/pixelstick

Usage: [options ...]

Options:
    start:     start displaying photos
    stop:      stop displaying and keep main-thread rest
    stream:    indicator for transmitting data via blueteeth
    time+:     timeInterval plus
    time-:     timeInterval minus
    listfiles: print a tree structure of files in SD card
    next:      display next photo in SD card
    prev:      display previous photo
    help:      show this message

Txt files in SD card and data sent to blueteeth must have a header,
which looks like this:
    #filename.txt#
    index r g b
    index r g b
    ......
or
    index,r,g,b
    index,r,g,b
    ......
";



void setup(){
    //Serial.begin(9600);
    // blueteeth baudrate set to 115200
    myport.begin(115200);
    myport.println("Initializing...");

    pinMode(2,  INPUT);
    pinMode(3,  INPUT);
    pinMode(10, OUTPUT);
    pinMode(13, OUTPUT);

    pixel.begin();
    pixel.show();

    root = init_SD();
    if (root){
        myport.println("Done!\n");
        n_Files = listFiles();
    }
    else{
        myport.println("Initialized, but SD card unfound.\n");
        RUN = false;
    }
}

void loop(){
    handleSerial();
    if (RUN){
        display();
    }
    else{
        // breath effect
    }
}



int listFiles(File root, int numTabs = 0){
    // list all files in SD card and return numbers of all
    int count = 0;
    File f;

    while (true){
        f = root.openNextFile();
        if (!f) break;
        for (int i = 0; i < numTabs; i++){
            myport.print("\t");
        }
        if (f.isDirectory()){
            myport.println(f.name() + "/");
            listFiles(f, numTabs + 1);
        }
        else{
            if (f.name().endsWith("txt")){
                // record file into FileNames with '#' indicator
                FileNames += "#" + String(count) + root.name() + "/" + f.name();
                if (c_Index == count){
                    myport.print("  >>%2d %s\t", count, f.name());
                }
                else{
                    myport.print("\t%2d %s\t", count, f.name());
                }
                myport.println(f.size(), DEC);
                count++;
            }
            else{
                myport.println("\t   %s", f.name());
            }
        }
        f.close();
    }
    root.rewindDirectory();
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
    // c_Index should be in [0, n_Files]
    data = SD.open(getFilename(&c_Index));

    if (!data){
        myport.println("Error when trying to open file %s", data.name());
        return;
    }
    // loading effect
    //for (float i = 0; i <= 1; i+=0.1) bar(i)

    data.readStringUntil('#');
    data.readStringUntil('#');

    //int r, g, b;
    int i;
    while (data.available()){
        // r = data.parseInt();
        // g = data.parseInt();
        // b = data.parseInt();
        while ((i = data.parseInt()) > n_LEDs){
            data.parseInt();
            data.parseInt();
            data.parseInt();
        }
        pixel.setPixelColor(i,
                            data.parseInt(),
                            data.parseInt(),
                            data.parseInt());
        if (i == n_LEDs){
            pixel.show();
            delay(t_Step);
        }
    }
}


void bar(float rate){
    // showing effect of progress bar
    // or music amptitude
    if (rate > 1) return;
    int n = n_LEDs * rate;
    pixel.setPixelColor(n, 10, 200, 10);
    while (n > 0)
        pixel.setPixelColor(--n, 128, 128, 128);
    pixel.show();
    delay(100);
}


File init_SD(){
    if (!SD.begin(SDPin)){
        myport.println("Card failed, or not present");
        return;
    }
    myport.println("card initialized.");
    return SD.open("/");
}


void receiveFile(bool writeSD = false){
    // param writeSD used for debugging
    // I'd like to test whether realtime uploading data works
    // I'm afraid serial is a little bit slow to realtime display
    // let's have a try

    // in case SD is not inserted
    if (!root) writeSD = false;

    while (myport.read()!='#'){}
    String filename = myport.readStringUntil('#');
    if (writeSD) File f = SD.open(filename, FILE_WRITE);
    myport.println("Receiving %s...", filename);

    int i, r, g, b;
    while (myport.available()){
        i = myport.parseInt();
        r = myport.parseInt();
        g = myport.parseInt();
        b = myport.parseInt();

        // display
        if (i <= n_LEDs) pixel.setPixelColor(i, r, g, b);
        // write in SD
        if (writeSD) f.println("%d %d %d %d", i, r, g, b);

        if (i == n_LEDs){
            pixel.show();
            delay(t_Step);
        }
    }

    if (writeSD){
        f.close();
        n_Files = listFiles(root);
    }
}


void handleSerial(){
    if (myport.available() == 0){
        delay(1000);
        return
    }

    String msg;
    char c;
    while (myport.available()){
        c = myport.read();
        if (c == '\n') break;
        msg + String(c);
    }
    switch (msg) {
        case "start":
            if (!root){
                root = init_SD();
                if (!root)
                    myport.println("SD card not found. Keep in rest.");
                    break;
                n_Files = listfiles();
            RUN = true;
            break;
        case "stop":
            RUN = false;
            break;
        case "stream":
            receiveFile();
            break;
        case "listfiles":
            listFiles();
            break;
        case "time+":
            myport.println("t_Step: %d", t_Step+=10);
            break;
        case "time-":
            myport.println("t_Step: %d", t_Step-=10);
            break;
        case "next":
            myport.println(getFilename(&++c_Index));
            break;
        case "prev":
            myport.println(getFilename(&--c_Index));
            break;
        case "help":
            myport.println(HELP);
            break;
        // case "": ;
        // case "": ;
    }
}
