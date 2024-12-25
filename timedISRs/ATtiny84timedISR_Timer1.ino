/*  ATtiny84timedISR_Timer1.ino - Timing an ISR using Timer 1 (without sleep)
Author: D. Dubins
Date: 24-Dec-24
Description: This program will run the ISR at the frequency set in the timer setup routine.

The following are the ATtiny84 pins by function:
------------------------------------------------
Pin 1: Vcc (1.8-5.5V)
Pin 2: 10/XTAL1/PCINT8/PB0
Pin 3: 9/XTAL2/PCINT9/PB1
Pin 4: dW/RESET/PCINT11/PB3
Pin 5: PWM/OC0A/CKOUT/8/INT0/PCINT10/PB2
Pin 6: PWM/ICP/OC0B/7/A7/ADC7/PCINT7/PA7
Pin 7: PWM/MOSI/SDA/OC1A/6/A6/ADC6/PCINT6/PA6
Pin 8: PWM/D0/OC1B/MISO/5/A5/ADC5/PCINT5/PA5
Pin 9: T1/SCL/SCK/4/A4/ADC4/PCINT4/PA4
Pin 10: 3/A3/ADC3/PCINT3/PA3
Pin 11: 2/A2/ADC2/PCINT2/PA2
Pin 12: 1/A1/ADC1/PCINT1/PA1
Pin 13: AREF/0/A0/ADC0/PCINT0/PA0
Pin 14: GND

Serial Hookup (if needed):
--------------------------
ATTINY84 digital pin 6 -> Arduino Uno pin 0 RX
ATTINY84 digital pin 7 -> Arduino Uno pin 1 TX
*/

#include <SoftwareSerial.h> // if needed

volatile bool report=false; // keep track if ISR is triggered

#define rxPin 6    // PA6 is physical pin 7 on ATtiny84
#define txPin 7    // PA7 is physical pin 6 on ATtiny84

SoftwareSerial mySerial(rxPin, txPin);

void setup(){
  mySerial.begin(9600);
  setTimer1();
}
 
void loop(){
  if(report){
    mySerial.println(millis());   // you shouldn't put serial or timed commands inside ISRs
    report=false;
  }
}

void setTimer1(){
  // CTC Match Routine using Timer 1 (ATtiny84)
  // Formula: frequency=fclk/((OCR1A+1)*N)
  cli();                // stop interrupts
  TCCR1A = 0;           // clear timer control register A
  TCCR1B = 0;           // clear timer control register B
  TCNT1 = 0;            // set counter to 0
  TCCR1B = _BV(WGM12);  // CTC mode (Table 12-5 on ATtiny84 datasheet)
  //TCCR1B = _BV(WGM13) | _BV(WGM12);
  //TCCR1B |= _BV(CS10);  // prescaler=1 (Table 14-6 on the ATtiny84 datasheet)
  //TCCR1B |= _BV(CS11);  // prescaler=8
  //TCCR1B |= _BV(CS11) | _BV(CS10);  // prescaler=64
  TCCR1B |= _BV(CS12); // prescaler=256
  //TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
  OCR1A = 31249;  //OCR1A=(fclk/(N*frequency))-1 (where N is prescaler).
  TIMSK1 |= _BV(OCIE1A);  // enable timer compare
  sei();                  // enable interrupts
}
  
ISR(TIM1_COMPA_vect){
  report=true;
}
