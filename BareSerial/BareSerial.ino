/*
This bare system only support transmitting cmd data through serial
-PixelStick Data Pin 6
-Bluetooth
    -RX --> Arduino TX1
    -TX --> Arduino RX0
    -baudrate: 115200(default)
    -soft serial performs not so well at 115200, so use hard serial please
*/
#include <Adafruit_NeoPixel.h>
#define PixelPin   6
#define BT_baud    115200
#define myport     Serial

uint8_t  light   = 0;
uint16_t n_LEDs  = 30;
uint16_t t_Step  = 50;
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(n_LEDs, PixelPin, NEO_KHZ800 + NEO_RGB);

void listen_serial();
void clear_pixel();
void pixel_cmd();
void receive_file(bool);

void setup(){
    myport.begin(BT_baud);
    pixel.begin();
    pixel.show();
    myport.println("Initialized");
    pinMode(5, INPUT);
}

void loop(){
    listen_serial();

    // use as light
    if (readCapacitivePin(5) > 5){
        light++;
        if (light > 3) light = 0;
        pixel.setBrightness(light*85);
        pixel.show();
    }
    delay(3000);
}

void clear_pixel(){
    for (int i = 0; i < n_LEDs; i++)
        pixel.setPixelColor(i, 0, 0, 0);
    pixel.show();
}

void pixel_cmd(){
    int i;
    uint8_t r, g, b;
    while (true){
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
    myport.println("terminated('-1' detected)");
    while(myport.available()){myport.read();}
    clear_pixel();
}

void receive_file(){
    while (myport.read()!='#'){}
    String filename = myport.readStringUntil('#');
    myport.println("Receiving " + String(filename));
    delay(10);
    uint32_t temp;
    uint8_t i;
    while (true){
        while (!myport.available()){}
        temp = myport.parseInt();
        i = temp >> 24;
        myport.printf("%6ld %3d %10ld %2d\n", millis(), i, temp, myport.available());
        if (i < n_LEDs) pixel.setPixelColor(i, temp & 0xffffff);
        if (i == n_LEDs - 1){
            pixel.show();
            myport.println(millis());
            delay(t_Step);
        }
        if (temp == 4294967295){
            if (myport.parseInt() == -1){
                myport.println("terminate");
                while(myport.available()){myport.read();}
                break;
            }
        }
    }
	delay(1000);
    clear_pixel();
}

void listen_serial(){
    if (myport.available()){
        String msg = myport.readStringUntil('\n');
        if (msg.endsWith(String('\r'))) msg = msg.substring(0, msg.length()-1);
        if (msg == "clear")
            clear_pixel();
        else if (msg == "ips")
            pixel_cmd();
        else if (msg == "push")
            receive_file();
        else if (msg == "time")
            myport.println("t_Step: "+String(t_Step));
    }
    else delay(3000);
}

uint8_t readCapacitivePin(int pinToMeasure) {
    // this function is from http://playground.arduino.cc/Code/CapacitiveSensor
    // pretty useful when implementing touch-detect utilities
    // Thanks to Mario Becker et al.

    volatile uint8_t* port;
    volatile uint8_t* ddr;
    volatile uint8_t* pin;
    byte bitmask;
    port = portOutputRegister(digitalPinToPort(pinToMeasure));
    ddr = portModeRegister(digitalPinToPort(pinToMeasure));
    bitmask = digitalPinToBitMask(pinToMeasure);
    pin = portInputRegister(digitalPinToPort(pinToMeasure));
    // Discharge the pin first by setting it low and output
    *port &= ~(bitmask);
    *ddr |= bitmask;
    delay(1);
    // Make the pin an input with the internal pull-up on
    *ddr &= ~(bitmask);
    *port |= bitmask;
    uint8_t cycles = 17;
    if      (*pin & bitmask) { cycles = 0;}
    else if (*pin & bitmask) { cycles = 1;}
    else if (*pin & bitmask) { cycles = 2;}
    else if (*pin & bitmask) { cycles = 3;}
    else if (*pin & bitmask) { cycles = 4;}
    else if (*pin & bitmask) { cycles = 5;}
    else if (*pin & bitmask) { cycles = 6;}
    else if (*pin & bitmask) { cycles = 7;}
    else if (*pin & bitmask) { cycles = 8;}
    else if (*pin & bitmask) { cycles = 9;}
    else if (*pin & bitmask) { cycles = 10;}
    else if (*pin & bitmask) { cycles = 11;}
    else if (*pin & bitmask) { cycles = 12;}
    else if (*pin & bitmask) { cycles = 13;}
    else if (*pin & bitmask) { cycles = 14;}
    else if (*pin & bitmask) { cycles = 15;}
    else if (*pin & bitmask) { cycles = 16;}
    // Discharge the pin again by setting it low and output
    // It's important to leave the pins low if you want to
    // be able to touch more than 1 sensor at a time - if
    // the sensor is left pulled high, when you touch
    // two sensors, your body will transfer the charge between
    // sensors.
    *port &= ~(bitmask);
    *ddr |= bitmask;
    return cycles;
}
