/* 
ATtiny84sleeps.ino Sketch
Author: D. Dubins
Date: 21-Apr-21
Examples: https://github.com/DaveCalaway/ATtiny/blob/master/Examples/AT85_sleep_interrupt/AT85_sleep_interrupt.ino

Description: Examples of two sleep modes to flash an LED light.
Sketch starts out in sleep mode (wake on interrupt). Push button sw1 to exit sleep.
Then sleep mode is used as a delay to flash the LED light.

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

If you are using a USBtinyISP:
USBtinyISP to ATtiny84:
-----------------------
Vcc to physical Pin 1 (Vcc)
MOSI to physical Pin 7 (PA6)
GND to physical Pin 14 (GND)
RESET to physical Pin 4 (PB3)
SCK to physical pin 9 (PA4)
MISO to physical Pin 8 (PA6)

Connections to ATtiny84
-----------------------
Batt +3.3V to ATtiny84 Pin 1 (Vcc)
LED(+) to ATtiny84 Pin 7 (=Digital Pin 2)
GND - SW1 - ATtiny85 Pin 5 (=Digital Pin 0)
NC - ATtiny85 Pin 1 (RST)  (you can also wire this to GND through a switch to reset the MCU)
Batt GND - ATtiny85 Pin 4 (=GND)

Note 1: To save more power, set output pinModes to be input pinmodes before sleeping.
Note 2: Always burn bootloader for a new chip. Burn with internal 1 MHz clock (will be more efficient on 3V battery).
Note 3: In deep sleep mode millis() stops functioning.
*/

#include <avr/interrupt.h>    // interrupt library
#include <avr/sleep.h>        // sleep library
#include <avr/power.h>        // power library
#include <avr/wdt.h>          // watchdog timer library

//define the classic bit functions:
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))
#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))
#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))

#define sw1 0                // use PB0 for set timer switch (physical pin 13)
#define ledPin 2             // use PB2 for LED (physical pin 11)

void setup(){
  pinMode(ledPin,OUTPUT);    // set ledPin to OUTPUT mode
  sleep_interrupt(sw1);      // sleep until button sw1 is pressed down.
}

 
void loop(){
  digitalWrite(ledPin,HIGH);
  sleep_timed(6);             // Sleep here. 6: 1 second delay. See sleep_timed() for options.
  digitalWrite(ledPin,LOW);
  sleep_timed(6);             // Sleep here. 6: 1 second delay. See sleep_timed() for options.
}

void sleep_interrupt(byte i){             // interrupt sleep routine - this one restores millis() and delay() after.
  //adapted from: https://github.com/DaveCalaway/ATtiny/blob/master/Examples/AT85_sleep_interrupt/AT85_sleep_interrupt.ino
  pinMode(i,INPUT_PULLUP);                // set sw1 to INPUT_PULLUP mode
  if(i<8){  //pins 0-7 are controlled by PCIE0, 8-11 are controlled by PCIE1
    sbi(GIMSK,PCIE0);                     // enable pin change interrupts    
  }else{
    sbi(GIMSK,PCIE1);                     // enable pin change interrupts
  }
  if(i==0)sbi(PCMSK0,PCINT0);             // use PB0 as interrupt pin
  if(i==1)sbi(PCMSK0,PCINT1);             // use PB1 as interrupt pin
  if(i==2)sbi(PCMSK0,PCINT2);             // use PB2 as interrupt pin
  if(i==3)sbi(PCMSK0,PCINT3);             // use PB3 as interrupt pin
  if(i==4)sbi(PCMSK0,PCINT4);             // use PB4 as interrupt pin
  if(i==5)sbi(PCMSK0,PCINT5);             // use PB5 as interrupt pin
  if(i==6)sbi(PCMSK0,PCINT6);             // use PB6 as interrupt pin
  if(i==7)sbi(PCMSK0,PCINT7);             // use PB7 as interrupt pin
  if(i==8)sbi(PCMSK1,PCINT10);            // use PB8 as interrupt pin
  if(i==9)sbi(PCMSK1,PCINT9);             // use PB9 as interrupt pin
  if(i==10)sbi(PCMSK1,PCINT8);            // use PB10 as interrupt pin
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // set sleep mode Power Down
  // The different modes are:
  // SLEEP_MODE_IDLE         -the least power savings
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN     -the most power savings
  cbi(ADCSRA,ADEN);                       // disable the ADC before powering down
  power_all_disable();                    // turn power off to ADC, TIMER 1 and 2, Serial Interface
  sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sleep_cpu();                            // go to sleep here
  sleep_disable();                        // after ISR fires, return to here and disable sleep
  power_all_enable();                     // turn everything back on
  sbi(ADCSRA,ADEN);                       // enable ADC again  
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
  if(i==8)cbi(PCMSK1,PCINT10);             // clear PB8 as interrupt pin
  if(i==9)cbi(PCMSK1,PCINT9);              // clear PB9 as interrupt pin
  if(i==10)cbi(PCMSK1,PCINT8);             // clear PB10 as interrupt pin
}

void awake_interrupt(byte i){              // interrupt sleep routine
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
  if(i==8)sbi(PCMSK1,PCINT10);              // use PB8 as interrupt pin
  if(i==9)sbi(PCMSK1,PCINT9);              // use PB9 as interrupt pin
  if(i==10)sbi(PCMSK1,PCINT8);            // use PB10 as interrupt pin
}

void sleep_timed(byte i){  
  //adapted from: https://forum.arduino.cc/index.php?topic=558075.0
  //and: http://www.gammon.com.au/forum/?id=11497&reply=6#reply6
  //sleep(0): 16 ms     sleep(5): 0.5 s
  //sleep(1): 32 ms     sleep(6): 1.0 s
  //sleep(2): 64 ms     sleep(7): 2.0 s
  //sleep(3): 0.125 s   sleep(8): 4.0 s
  //sleep(4): 0.25 s    sleep(9): 8.0 s
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // set sleep mode Power Down
  cbi(ADCSRA,ADEN);                       // disable the ADC before powering down
  power_all_disable();                    // turn power off to ADC, TIMER 1 and 2, Serial Interface
  cli();                                  // disable interrupts
  resetWDT(i);                            // reset the watchdog timer (routine below) 
  sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                                  // enable interrupts (required now)
  sleep_cpu();                            // go to sleep here
  sleep_disable();                        // after ISR fires, return to here and disable sleep
  power_all_enable();                     // turn everything back on
  sbi(ADCSRA,ADEN);                       // enable ADC again
}

void resetWDT(byte j){
  /* WDP3  WDP2  WDP1  WDP0   Timeout @5V  (From Table 8-3 of ATtiny84 datasheet)
   * 0     0     0     0      16 ms
   * 0     0     0     1      32 ms
   * 0     0     1     0      64 ms
   * 0     0     1     1      0.125 s
   * 0     1     0     0      0.25 s
   * 0     1     0     1      0.5 s
   * 0     1     1     0      1.0 s
   * 0     1     1     1      2.0 s
   * 1     0     0     0      4.0 s
   * 1     0     0     1      8.0 s
   */
  int WDTCSRarr[10][2]={
    {0b11011000,16},   // 16 ms  (idx: 0)
    {0b11011001,32},   // 32 ms  (idx: 1)
    {0b11011010,64},   // 64 ms  (idx: 2)
    {0b11011011,125},  // 0.125s (idx: 3)
    {0b11011100,250},  // 0.25s  (idx: 4)
    {0b11011101,500},  // 0.5s   (idx: 5)
    {0b11011110,1000}, // 1.0s   (idx: 6)
    {0b11011111,2000}, // 2.0s   (idx: 7)
    {0b11111000,4000}, // 4.0s   (idx: 8)
    {0b11111001,8000}  // 8.0s   (idx: 9)
  };
  MCUSR = 0; // reset the MCUSR register
  //Older routine to set WDTCSR:
  /*WDTCSR = _BV(WDCE)|_BV(WDE)|_BV(WDIF); // allow changes, disable reset, clear existing interrupt
  WDTCSR = _BV(WDIE);   // set WDIE (Interrupt only, no Reset)
  sbi(WDTCSR,WDIE);     // set WDIE (Interrupt only, no Reset)
  //Now set the following 4 bits as per the timing chart above, to set the sleep duration:
  sbi(WDTCSR,WDP3);     //(1,0,0,0 = 4.0 s)
  cbi(WDTCSR,WDP2);
  cbi(WDTCSR,WDP1);
  cbi(WDTCSR,WDP0);*/
  WDTCSR=(byte)WDTCSRarr[j][0];        // set WDTCSR register
  wdt_reset();                       // reset watchdog timer 
}

ISR(WDT_vect){
  wdt_disable();      // disable watchdog timer
                      // optionally, keep track of number of cycles here if you would like to wait
}                     // multiples of cycles to do something (stick a counter here).

ISR(PCINT0_vect){     // This always needs to be "PCINT0_vect" regardless of what PCINT you select above
                      // Put whatever code you would like to appear here after the button is pushed.
}                     // ISR called on interrupt sleep
