/* 
ATtiny84interrupt.ino Sketch
Author: D. Dubins
Date: 11-May-21
Description: Interrupt routines (awake) for ATtiny84. ISRs is written to only respond to FALLING signals.
Interrupt attached to momentary switch is used to change the state of an LED.

If you are using a USBtinyISP:
USBTinyISP to ATtiny84:
-----------------------
Vcc to physical Pin 1 (Vcc)
MOSI to physical Pin 7 (PA6)
GND to physical Pin 14 (GND)
RESET to physical Pin 4 (PB3)
SCK to physical pin 9 (PA4)
MISO to physical Pin 8 (PA6)

The following are the ATtiny84 pins by function:
------------------------------------------------
Pin 1: Vcc (1.8-5.5V)
Pin 2: 10/XTAL1/PCINT8/PB0
Pin 3: 9/XTAL2/PCINT9/PB1
Pin 4: dW/RESET/PCINT11/PB3
Pin 5: PWM/OC0A/7/A7/PCINT7/PA7
Pin 6: PWM/ICP/OC0B/7/A7/ADC7/PCINT7/PA7
Pin 7: PWM/MOSI/SDA/OC1A/6/A6/ADC6/PCINT6/PA6

Pin 8: PWM/D0/OC1B/MISO/5/A5/ADC5/PCINT5/PA5
Pin 9: T1/SCL/SCK/4/A4/ADC4/PCINT4/PA4
Pin 10: 3/A3/ADC3/PCINT3/PA3
Pin 11: 2/A2/ADC2/PCINT2/PA2
Pin 12: 1/A1/ADC1/PCINT1/PA1
Pin 13: AREF/0/A0/ADC0/PCINT0/PA0
Pin 14: GND

Connections to ATtiny84
-----------------------
+5V to ATtiny84 Pin 1 (Vcc)
LED(+) to ATtiny84 Pin 6 (=Digital Pin 7)
GND - SW1 - ATtiny84 Pin 7 (=Digital Pin 6)
NC - ATtiny84 Pin 4 (RST)  (you can also wire this to GND through a switch to reset the MCU)
GND - ATtiny84 Pin 14 (=GND)

Note: the ISR could have been written to change the state (ledState=!ledState; //inside the ISR)
However, debouncing would be extremely difficult! By writing the sketch this way, you can debounce
after you detect that the button has been pushed, using if(pushed).

*/

//define the classic bit functions:
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))
#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))
#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))

#define sw1 6               // use PA6 for set timer switch (physical pin 7)
#define ledPin 7            // use PA7 for LED (physical pin 6)

volatile bool pushed=false; // to hold (will be written to inside ISR)
bool ledState=false;        // to hold the LED state (on/off)

void setup(){
  pinMode(ledPin,OUTPUT);   // set ledPin to OUTPUT mode
  attach_interrupt(sw1);    // attach pin sw1 as interrupt
}
 
void loop(){
  if(pushed){
    ledState=!ledState;
    digitalWrite(ledPin,ledState);  // change
    delay(200);       // debounce
    pushed=false;     // reset pushed state
  }
}

void attach_interrupt(byte i){             // attach a pin change interrupt for pin i
  pinMode(i,INPUT_PULLUP);                 // set sw1 to INPUT_PULLUP mode
  if(i<8){  //pins 0-7 are controlled by PCIE0, 8-11 are controlled by PCIE1
    sbi(GIMSK,PCIE0);                      // enable pin change interrupts    
  }else{
    sbi(GIMSK,PCIE1);                      // enable pin change interrupts
  }
  if(i==0)sbi(PCMSK0,PCINT0);              // use PB0 as interrupt pin
  if(i==1)sbi(PCMSK0,PCINT1);              // use PB1 as interrupt pin
  if(i==2)sbi(PCMSK0,PCINT2);              // use PB2 as interrupt pin
  if(i==3)sbi(PCMSK0,PCINT3);              // use PB3 as interrupt pin
  if(i==4)sbi(PCMSK0,PCINT4);              // use PB4 as interrupt pin
  if(i==5)sbi(PCMSK0,PCINT5);              // use PB5 as interrupt pin
  if(i==6)sbi(PCMSK0,PCINT6);              // use PB6 as interrupt pin
  if(i==7)sbi(PCMSK0,PCINT7);              // use PB7 as interrupt pin
  if(i==8)sbi(PCMSK1,PCINT8);              // use PB8 as interrupt pin
  if(i==9)sbi(PCMSK1,PCINT9);              // use PB9 as interrupt pin
  if(i==10)sbi(PCMSK1,PCINT10);            // use PB10 as interrupt pin
  if(i==11)sbi(PCMSK1,PCINT11);            // use PB11 as interrupt pin
}


//This function isn't used in the sketch, but could be used to deactivate the interrupt for a specific section of code.
void detach_interrupt(byte i){             // detach a pin change interrupt for pin i
  pinMode(i,INPUT_PULLUP);                 // set sw1 to INPUT_PULLUP mode
  if(i<8){  //pins 0-7 are controlled by PCIE0, 8-11 are controlled by PCIE1
    cbi(GIMSK,PCIE0);                      // disable pin change interrupts    
  }else{
    cbi(GIMSK,PCIE1);                      // disable pin change interrupts
  }
  if(i==0)cbi(PCMSK0,PCINT0);              // clear PB0 as interrupt pin
  if(i==1)cbi(PCMSK0,PCINT1);              // clear PB1 as interrupt pin
  if(i==2)cbi(PCMSK0,PCINT2);              // clear PB2 as interrupt pin
  if(i==3)cbi(PCMSK0,PCINT3);              // clear PB3 as interrupt pin
  if(i==4)cbi(PCMSK0,PCINT4);              // clear PB4 as interrupt pin
  if(i==5)cbi(PCMSK0,PCINT5);              // clear PB5 as interrupt pin
  if(i==6)cbi(PCMSK0,PCINT6);              // clear PB6 as interrupt pin
  if(i==7)cbi(PCMSK0,PCINT7);              // clear PB7 as interrupt pin
  if(i==8)cbi(PCMSK1,PCINT8);              // clear PB8 as interrupt pin
  if(i==9)cbi(PCMSK1,PCINT9);              // clear PB9 as interrupt pin
  if(i==10)cbi(PCMSK1,PCINT10);            // clear PB10 as interrupt pin
  if(i==11)cbi(PCMSK1,PCINT11);            // clear PB11 as interrupt pin
}

ISR(PCINT0_vect){     // This ISR will run when interrupt is triggered
  if(!digitalRead(sw1))pushed=true; // only do something when state is FALLING
  // really no if statement is needed here. The ISR will be called on a FALLING signal on sw1.
  // so all that is needed inside the ISR routine is "pushed=true;".
}
