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
}

void loop(){
    listen_serial();
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
    else delay(1000);
}
