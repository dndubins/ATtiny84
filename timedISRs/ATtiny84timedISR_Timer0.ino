/*  ATtiny84timedISR_Timer0.ino - Timing an ISR using Timer 0 (without sleep)
Author: D. Dubins
Date: 24-Dec-24
Description: This program will run the ISR at the time delay specified by reportingTime.

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

#include <SoftwareSerial.h>  // if needed

volatile bool report = false;  // keep track if ISR is triggered
volatile int reading = 0;      // to hold analog reading
volatile int cycles = 0;       // to hold total# cycles ISR ran
int reportingTime = 2000;      // data collection interval (in milliseconds)

#define rxPin 6  // PA6 is physical pin 7 on ATtiny84
#define txPin 7  // PA7 is physical pin 6 on ATtiny84

SoftwareSerial mySerial(rxPin, txPin);

void setup() {
  mySerial.begin(9600);
  setTimer0();
}

void loop() {
  if (report) {
    mySerial.println(reading);  // you shouldn't put serial or timed commands inside ISRs
    report = false;
  }
}

void setTimer0() {
  // CTC Match Routine using Timer 0 (ATtiny84)
  // Formula: frequency=fclk/((OCR0A+1)*N)
  cli();                // stop interrupts
  TCCR0A = 0;          // Clear TCCR0A
  TCCR0B = 0;          // Clear TCCR0B
  TCNT0 = 0;           // Initialize counter
  TCCR0A |= (1 << WGM01); // Set CTC mode
  //TCCR0B = _BV(CS00);  // prescaler=1 (no prescaling)   (Table 13-9, ATtiny84 datasheet)
  //TCCR0B = _BV(CS01);  // prescaler=8
  TCCR0B = _BV(CS01) | _BV(CS00);  // prescaler=64
  //TCCR0B = _BV(CS02);  // prescaler=256
  //TCCR0B = _BV(CS02) | _BV(CS00);  // prescaler=1024
  OCR0A = 124;        //OCR1A=(fclk/(N*frequency))-1 (where N is prescaler).
  TIMSK0 |= _BV(OCIE0A);       // enable interrupt on Compare Match A for Timer0
  sei();                  // Enable global interrupts
}

ISR(TIM0_COMPA_vect) {
  //This ISR will run 31 times per second. We keep track of the number of times it ran
  //to report a reading.
  //Make ISRs as simple and as short as possible. Any global arrays changed should be declared
  //as volatile.
  cycles++;
  if (cycles >= reportingTime) {  // (2000 msec/1000) x 1000 cycles/sec = 2000 cycles.
    reading = analogRead(A1);     // take an analog reading from pin A1
    report = true;                // flag that datum is ready (just for reporting)
    cycles = 0;                   // reset the number of cycles
  }
}
