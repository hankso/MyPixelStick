/*
This bare system only support transmitting cmd data through serial
-PixelStick Data Pin 8
-Bluetooth
    -RX --> Arduino TX1
    -TX --> Arduino RX0
    -baudrate: 115200(default)
    -soft serial performs not so well at 115200, so use hard serial please
*/
#include <Adafruit_NeoPixel.h>
#define Lighterval  255/5
#define pixelPin    8
#define touchPin    10
#define bt_baudrate 115200
#define myport      Serial

bool
    STATE = false,
    TOUCH = false;
uint8_t
    LIGHT   = 0;
uint16_t
    nLEDs  = 144,
    tStep  = 5;
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(nLEDs, pixelPin, NEO_KHZ800 + NEO_RGB);


void allPixel(uint8_t c = 0) {
    // pixel.clear(); TODO this function not work
    for (int i = 0; i < nLEDs; i++)
        pixel.setPixelColor(i, c, c, c);
    pixel.show();
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

uint8_t touchDetect(int pin = touchPin) {
    // volatile uint8_t*
    //     PORT = portOutputRegister(digitalPinToPort(pin)),
    //     PIN  = portInputRegister(digitalPinToPort(pin)),
    //     DDR  = portModeRegister(digitalPinToPort(pin));
    byte bitmask = digitalPinToBitMask(pin);
    volatile uint8_t* PORT = portOutputRegister(digitalPinToPort(pin));
    volatile uint8_t* PIN  = portInputRegister(digitalPinToPort(pin));
    volatile uint8_t* DDR  = portModeRegister(digitalPinToPort(pin));
    *DDR |= bitmask; *PORT &= ~(bitmask);
    // pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
    delay(1);
    *DDR &= ~(bitmask); *PORT |= bitmask;
    // pinMode(pin, INPUT_PULLUP);
    int times = 0;
    while (!(*PIN & bitmask)) {times++;}
    // while (!digitalRead(pin)) {times++;}
    *DDR |= bitmask; *PORT &= ~(bitmask);
    // pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
    return times;
}

void pixelCmd() {
    int i;
    uint8_t r, g, b;
    while (true) {
    	myport.print("ips> ");
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
    myport.println(F("terminated('-1' detected)"));
    while(myport.available()){myport.read();}
    allPixel();
}

void receiveFile() {
    while (myport.read()!='#') {}
    String filename = myport.readStringUntil('#');
    myport.println("Receiving " + String(filename));
    delay(10);
    uint32_t temp;
    uint8_t i;
    while (true) {
        while (!myport.available()) {}
        temp = myport.parseInt();
        i = temp >> 24;
        // myport.printf("%6ld %3d %10ld %2d\n", millis(), i, temp, myport.available());
        myport.printf("%6ld %3d ", millis(), i);
        myport.printf("%3d %3d %3d\n", (temp>>16)&0xff, (temp>>8)&0xff, temp&0xff);
        if (i < nLEDs) pixel.setPixelColor(i, temp & 0xffffff);
        if (i == nLEDs - 1) {
            pixel.show();
            myport.println(millis());
            delay(tStep);
        }
        if (temp == 4294967295) {
            if (myport.parseInt() == -1) {
                myport.println("terminate");
                while(myport.available()) myport.read();
                break;
            }
        }
    }
	delay(1000);
    allPixel();
}

void HELP() {
    myport.println(F("PixelStick by hank"));
    myport.println(F("Version: 1.0.1 https://github.com/hankso/MyPixelStick"));
    myport.println(F("Usage: [state|push|ips|pixel|touch|clear|time|bar|light]"));
}

void listenSerial() {
    if (myport.available()) {
        String msg = myport.readStringUntil('\n');
        if (msg.endsWith(String('\r'))) msg = msg.substring(0, msg.length()-1);
        if (msg.length()) {
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
            else if (msg == "ips")
                pixelCmd();
            else if (msg.startsWith("bar") && msg.length() > 3)
                bar(abs(msg.substring(3).toFloat()));
            else if (msg == "help")
                HELP();
            else myport.println(msg + ": command not found");
        }
        myport.print("\n>>> ");
    }
    else delay(200);
}


void setup() {
    myport.begin(bt_baudrate);
    pixel.begin();
    pixel.show();
    myport.println(F("Initialized"));
    allPixel(0);
    myport.print(F("\nTry 'help' for more information.\n>>> "));
}

void loop() {
    listenSerial();
    if(STATE) rainbow(5);
    if(TOUCH) {
        if (touchDetect(touchPin) > 5){
            allPixel(LIGHT+=Lighterval);
            myport.println("light: " + String(LIGHT));
            delay(3000);
        }
    }
}
