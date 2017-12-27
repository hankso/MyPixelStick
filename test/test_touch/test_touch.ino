/*
In Arduino.h:
#define digitalPinToPort(P)
        ( pgm_read_byte( digital_pin_to_port_PGM + (P) ) )
#define digitalPinToBitMask(P)
        ( pgm_read_byte( digital_pin_to_bit_mask_PGM + (P) ) )
#define digitalPinToTimer(P)
        ( pgm_read_byte( digital_pin_to_timer_PGM + (P) ) )
#define analogInPinToBit(P) (P)
#define portOutputRegister(P)
        ( (volatile uint8_t *)( pgm_read_word( port_to_output_PGM + (P))) )
#define portInputRegister(P)
        ( (volatile uint8_t *)( pgm_read_word( port_to_input_PGM + (P))) )
#define portModeRegister(P)
        ( (volatile uint8_t *)( pgm_read_word( port_to_mode_PGM + (P))) )
*/

#define BAUDRATE 115200
#define touchPin 10
#define myport   Serial

void setup() {
    myport.begin(BAUDRATE);
}

void loop() {
    delay(200);
    myport.printf("DigitalRead: %d, Register: %d\n", touchDigitalRead(touchPin), touchRegister(touchPin));
}

int touchDigitalRead(int pin) {
    // volatile uint8_t*
    //     PORT = portOutputRegister(digitalPinToPort(pin)),
    //     PIN  = portInputRegister(digitalPinToPort(pin)),
    //     DDR  = portModeRegister(digitalPinToPort(pin));

    // byte bitmask = digitalPinToBitMask(pin);
    // volatile uint8_t* PORT = portOutputRegister(digitalPinToPort(pin));
    // volatile uint8_t* PIN  = portInputRegister(digitalPinToPort(pin));
    // volatile uint8_t* DDR  = portModeRegister(digitalPinToPort(pin));

    // *DDR |= bitmask; *PORT &= ~(bitmask);
    pinMode(pin, OUTPUT); digitalWrite(pin, LOW);

    delay(1);
    int times = 0;

    // *DDR &= ~(bitmask); *PORT |= bitmask;
    pinMode(pin, INPUT_PULLUP);

    // while (!(*PIN & bitmask)) {times++;}
    while (!digitalRead(pin)) {times++;}

    // *DDR |= bitmask; *PORT &= ~(bitmask);
    pinMode(pin, OUTPUT); digitalWrite(pin, LOW);

    return times;
}

int touchRegister(int pin) {
    byte bitmask = digitalPinToBitMask(pin);
    volatile uint8_t* PORT = portOutputRegister(digitalPinToPort(pin));
    volatile uint8_t* PIN  = portInputRegister(digitalPinToPort(pin));
    volatile uint8_t* DDR  = portModeRegister(digitalPinToPort(pin));
    pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
    delay(1);
    int times = 0;
    pinMode(pin, INPUT_PULLUP);
    while (!(*PIN & bitmask)) {times++;}
    pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
    return times;
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
