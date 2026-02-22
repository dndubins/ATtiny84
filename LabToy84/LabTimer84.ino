/* LabTimer.ino - Main LabToy Program for the ATtiny84 (no clock function)
   (temperature, timer, and stopwatch)
   Author: D. Dubins
   Date: 28-Apr-21
   Last Updated:20-Feb-26
   This sketch uses the TM1637 library by Avishay Orpaz version 1.2.0
   (available through the Library Manager)

   Burn bootloader to 16 MHz External Clock
   Connections:
   TM1637 -- ATtiny84
   CLK - PA2 (Physical pin 11)
   DIO - PA1 (Physical pin 12)
   GND - GND
   5V - PA0 (Physical pin 13)

   "Set" Momentary Switch on the right: (to start/stop/reset timer & stopwatch, and display the temperature)
   GND - sw1 - PA3 (Physical pin 10)
   "Mode" Momentary Switch on the left: (to change mode)
   GND - SW2 - PA4 (Physical pin 9)

   Active Piezo Buzzer:
   Longer leg (+) - PA5 (physical pin 8)
   Shorter leg - GND

   To set LED brightness:
   Pressing the MODE button (long push) enters "set brightness and LED on/off" mode
   set LED BRIGHTNESS with short push (SET button), accept with long push (SET button)
   Options: 0(most dim) to 7 (most bright)
   -->BATT status will display (in %)

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

   Burning the ATtiny84 with Uno as ISP
   ------------------------------------
   UNO -- ATtiny84
   +5V -- physical Pin 1 (Vcc)
   Pin 13 -- physical Pin 9 (SCK)
   Pin 12 -- physical Pin 8 (MISO)
   Pin 11 -- physical Pin 7 (MOSI)
   Pin 10 -- physical Pin 4 (RST)
   GND -- physical Pin 14 (GND)
   XTAL

   TM1637 Custom Character Map:
       -A-
     F|   |B
       -G-
     E|   |C
       -D-
*/

// Serial monitor if needed:
//#include <SoftwareSerial.h>
//#define rxPin 7   // PA7 is PP 6
//#define txPin 6   // PA6 is PP 7
//SoftwareSerial mySerial(rxPin, txPin);

// Fast pin modes, writes, and reads for the ATtiny84 (digital pins 0-11)
// Modes: 0 = INPUT, 1 = OUTPUT, 2 = INPUT_PULLUP
#define pinModeFast(p, m) \
  do { \
    if ((p) <= 7) { \
      if ((m)&1) DDRA |= (1 << (p)); \
      else DDRA &= ~(1 << (p)); \
      if (!((m)&1)) ((m)& 2 ? PORTA |= (1 << (p)) : PORTA &= ~(1 << (p))); \
    } else if ((p) >=8 && (p) <= 11) { \
      if ((m)&1) DDRB |= (1 << ((p) - 8)); \
      else DDRB &= ~(1 << ((p) - 8)); \
      if (!((m)&1)) ((m)& 2 ? PORTB |= (1 << ((p) - 8)) : PORTB &= ~(1 << ((p) - 8))); \
    } \
  } while(0)

#define digitalWriteFast(p, v) \
  do { \
    if ((p) <= 7) { \
      (v) ? PORTA |= (1 << (p)) : PORTA &= ~(1 << (p)); \
    } else if ((p) >= 8 && (p) <= 11) { \
      (v) ? PORTB |= (1 << ((p) - 8)) : PORTB &= ~(1 << ((p) - 8)); \
    } \
  } while(0)

#define digitalReadFast(p) \
  (((p) <= 7) ? \
    ((PINA & (1 << (p))) ? 1 : 0) : \
   ((p) >= 8 && (p) <= 11) ? \
    ((PINB & (1 << ((p) - 8))) ? 1 : 0) : 0)

//for sleep functions
#include <avr/sleep.h>  // sleep library
#include <avr/power.h>  // power library

//for flash space
#include <avr/pgmspace.h>

//define the classic bit functions:
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
//#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
//#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))
//#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))
//#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))

#define TOFFSET 2.5  // Temperature offset for core temperature routine. Individually calibrated per chip. Start with 0.0 and measure temperature against a real thermometer. Enter offset here. \
                     // There is no separate device or calibation program for this procedure. This sketch is simply uploaded twice.

#include <TM1637Display.h>  // For TM1637 display library (Avishay Orpaz v. 1.2.0)
#define CLK 2               // use PA2 for CLK (physical pin 11)
#define DIO 1               // use PA1 for DIO (physical pin 12)
#define TMVCC 0             // use PA0 for Vcc of TM1637 (physical pin 13)

byte mode = 0;  // mode=0: timer, mode=1: stopwatch, mode=2: temperature
enum modes {    // define mode options as enum
  TIMER,        // timer mode (=0)
  STOPWATCH,    // stopwatch mode (=1)
  TEMPERATURE   // temperature mode (=2)
};
bool modeChanged = false;  // to capture a change in mode

byte brightness = 0;  // initial brightness setting for TM1637 (0-7) (to save batteries, use a lower number). Use 2 for rechargeable, 3 for alkaline

TM1637Display display(CLK, DIO);

//Active Piezo Buzzer Parameters:
#define BEEPTIME 100  // duration of beep in milliseconds
#define buzzPin 5     // use PA5 for active piezo buzzer (physical pin 8)
//Define momentary switches:
#define sw1 3  // use PA3 for set switch (physical pin 10) - add time to timer, etc.
#define sw2 4  // use PA4 for mode switch (physical pin 9) - toggle mode

//timer variables
unsigned long tDur = 0;        // for setting duration of timer
volatile bool pushed = false;  // for storing if button has been pushed
volatile bool beeped = true;   // whether or not device beeped. start out in true state upon device reset
unsigned long tEnd = 0UL;      // for timer routine, end time in msec

//stopwatch variables
unsigned long toffsetSW = 0UL;  // to hold offset time for stopwatch

//programmed delays
#define DISPTIME_SLOW 800  // time to flash user information e.g. time, date
#define DISPTIME_FAST 300  // time to flash menu item between clicks
#define DEBOUNCE 20        // time to debounce a button
#define WARMUP 50          // time to warm up after sleeping/etc.

const uint8_t SEG_PUSH[] PROGMEM = {
  SEG_E | SEG_F | SEG_A | SEG_B | SEG_G,  // P
  SEG_F | SEG_E | SEG_D | SEG_C | SEG_B,  // U
  SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,  // S
  SEG_F | SEG_E | SEG_G | SEG_B | SEG_C   // H
};

const uint8_t SEG_DONE[] PROGMEM = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,          // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // O
  SEG_C | SEG_E | SEG_G,                          // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G           // E
};

const uint8_t SEG_LED[] PROGMEM = {
  // for brightness
  SEG_F | SEG_E | SEG_D,                  // L
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,  // E
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,  // d
  0x00                                    // space
};

const uint8_t SEG_BATT[] PROGMEM = {
  // for brightness
  SEG_F | SEG_E | SEG_D | SEG_G | SEG_C,          // b
  SEG_E | SEG_F | SEG_A | SEG_B | SEG_C | SEG_G,  // A
  SEG_F | SEG_E | SEG_D | SEG_G,                  // t
  SEG_F | SEG_E | SEG_D | SEG_G                   // t
};

const uint8_t SEG_DEGC[] PROGMEM = {
  0x00,                           // space
  0x00,                           // space
  SEG_A | SEG_F | SEG_G | SEG_B,  // degree sign
  SEG_A | SEG_F | SEG_E | SEG_D   // C
};

const uint8_t SEG_ON[] PROGMEM = {
  0x00,                                           // space
  0x00,                                           // space
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // O
  SEG_E | SEG_G | SEG_C                           // n
};

const uint8_t SEG_OFF[] PROGMEM = {
  0x00,                                           // space
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,  // O
  SEG_E | SEG_F | SEG_A | SEG_G,                  // F
  SEG_E | SEG_F | SEG_A | SEG_G                   // F
};

void setup() {
  //mySerial.begin(9600);                  // start the serial monitor
  tEnd = millis();                    // initialize end time
  cbi(ADCSRA, ADEN);                  // disable ADC to save power (not needed for this sketch)
  TMVCCon();                          // turn on Vcc for the TM1637 display
  display.clear();                    // clear the display
  display.setBrightness(brightness);  // 0:MOST DIM, 7: BRIGHTEST
  pinModeFast(sw1, INPUT_PULLUP);         // input pullup mode for sw1
  pinModeFast(sw2, INPUT_PULLUP);         // input pullup mode for sw2
}

void loop() {
  bool resetTimer = false;  // if true, reset the timer
  byte p2 = 0;              // to hold sw2 push
  byte p1 = 0;              // to hold sw1 push
  p2 = buttonRead(sw2);     // read MODE button and store to p1
  buttonReset(sw2);         // user should let go of MODE button

  if (p2 == 1) {            // if sw2 mode button pushed (short). This routine runs when first switched to new mode.
    mode++;                 // increment mode
    if(mode>2)mode=0;       // wrap around max mode
    modeChanged = true;     // change in mode happened
  } else if (p2 == 2) {     // if long push on mode button
    setLED();               // set brightness
    modeChanged = true;     // resets stopwatch if leaving setLED()
  }

  if (modeChanged) {  // user has changed the mode
    if (mode == TIMER) {
      resetTimer = true;
      timer_reset();
    } else if (mode == STOPWATCH) {
      stopWatch_reset();               // reset stopwatch on mode change
    } else if (mode == TEMPERATURE) {  // first occurrence of temperature after switching mode
      TMVCCon();                       // turn on Vcc to the TM1637 display
      showTemp(readCoreTemp(100));     // show avg of 100 core temperature readings
      safeWait(sw2, DISPTIME_SLOW);    // show the temperature
      TMVCCoff();                      // turn off power to the TM1637 display
    }
    modeChanged = false;
  } else {  // Mode is not changing. Steady-state behaviour starts here (regular loop)

    // Now the regular loop:
    if (mode == TIMER) {
      p1 = buttonRead(sw1);  // take a button reading
      if (p1 == 2) {         // if long push
        resetTimer = true;   // reset the timer
      } else if (p1 == 1) {  // short push adds time to the timer
        TMVCCon();           // turn on Vcc to the TM1637 display
        beeped = false;      // rearm the buzzer

        // Uncomment section for desired timer button behaviour:

        // Strategy 1: SET button adds 30 seconds with each push:
        if (tEnd > millis()) {
          unsigned long remaining = (tEnd - millis() + 999) / 1000UL;
          remaining += 30;  // Add 30 seconds directly, then round result up to 30-sec boundary
          tDur = ((remaining + 29) / 30) * 30;
        } else {
          tDur = 30;
        }

        // Strategy 2: Bucketed approach:
        /*if (tEnd > millis()) {
          uint32_t remaining = tEnd - millis();
          uint32_t minutes = remaining / 60000UL;
          uint32_t secondsPart = remaining % 60000UL;

          // If more than 50 seconds into current minute,
          // round to next minute
          if (secondsPart > 50000UL) minutes++;
          tDur = minutes * 60UL;
        }
        if (tDur >= 6UL * 60UL * 60UL) {
          tDur += 60UL * 60UL;
        } else if (tDur >= 60UL * 60UL) {
          tDur += 15UL * 60UL;
        } else if (tDur >= 20UL * 60UL) {
          tDur += 5UL * 60UL;
        } else {
          tDur += 60UL;
        }*/

        showTimeTMR(tDur * 1000UL, true);          // show start time remaining (true=force display)
        delay(DEBOUNCE);                           // have a small real delay. This prevents double presses.
        safeWait(sw1, 1000 - DEBOUNCE);            // button-interruptable wait function
        tEnd = millis() + (tDur * 1000UL);         // calculate new tEnd time
      } else if (p1 == 0) {                        // if button not pushed
        if (tDur > 0 && millis() < tEnd - 1000) {  // after waiting the allotted time (don't include last second).
          showTimeTMR(tEnd - millis(), false);     // show time remaining
        } else if (!beeped) {                      // 20 second timer
          beeped = true;                           // yes! we beeped!
          for (int i = 0; i < 10; i++) {
            if (i % 2 == 0) {         // if i is even
              showTimeTMR(0, false);  // show zero
            } else {
              showSegments_P(SEG_DONE);  // show "done" message
            }
            if (beepBuzz(buzzPin, 3) > 0) {  // flash and beep 3x
              resetTimer = true;             // flag the timer to reset
              break;                         // leave early (user silenced alarm)
            }
          }
        } else {              // what the sketch does after time runs out, and the timer beeped
          resetTimer = true;  // flag the timer to reset
        }
      }
      if (resetTimer)timer_reset();
    } else if (mode == STOPWATCH) {
      showTimeSW(millis() - toffsetSW);  // show time elapsed
      p1 = buttonRead(sw1);              // read the SET button
      if (p1 == 1) {                     // short push
        stopWatch_pause();               // pause stopwatch
      } else if (p1 == 2) {
        stopWatch_reset();
      }
      buttonReset(sw1);  // wait until suser lets go of sw1
    } else if (mode == TEMPERATURE) {
      p1 = buttonRead(sw1);            // read button sw1
      if (p1 == 1) {                   // if short push
        TMVCCon();                     // turn on Vcc to the TM1637 display
        showTemp(readCoreTemp(100));   // show avg of 100 core temperature readings
        safeWait(sw1, DISPTIME_SLOW);  // show the temperature for DISPTIME, interruptable with sw1
        buttonReset(sw1);              // wait until user lets go of SET button
        TMVCCoff();                    // turn off Vcc for the TM1637 display
        sleep_interrupt();             // go to sleep here (waits in sleep mode, with 0.8 uA current draw)
        TMVCCon();                     // turn on Vcc for the TM1637 display
      }
    }
  }
}

void showBoolState(bool a) {  // time in min
  display.clear();            // clear the display
  if (a) {
    showSegments_P(SEG_ON);  // show "ON" message
  } else {
    showSegments_P(SEG_OFF);  // show "OFF" message
  }
}

void setItemByte(byte &item, byte lowLim, byte highLim, void (*f)(byte)) {  // sets an individual byte item during the setAll() routine
  //(*f) passes function as argument (it's a function pointer)
  byte push = 0;
  (*f)(item);                             // show item on LCD
  while (digitalReadFast(sw2)) {              // sw2 exits this routine
    push = buttonRead(sw1);               // read button
    if (push == 1) {                      // if there was a short push
      item++;                             // add 1 to item
      if (item > highLim) item = lowLim;  // wrap around item
      (*f)(item);                         // show updated item on LCD
      buttonReset(sw1);                   // debounce sw1
    } else if (push == 2) {               // if there was a long push
      item--;                             // add 1 to item
      if (item == 255) item = highLim;    // wrap around item
      (*f)(item);                         // show updated item on LCD
      buttonReset(sw1);                   // debounce sw1
    }                                     // end if
  }                                       // end while
  buttonReset(sw1);                       // don't leave until user lets go of sw1
  buttonReset(sw2);                       // don't leave until user lets go of sw2
  delay(100);                             // prevents nonsense
}

void setItemBool(bool &item, void (*f)(bool)) {  // sets an individual bool item during the setAll() routine
  //(*f) passes function as argument (it's a function pointer)
  byte push = 0;
  (*f)(item);                 // show item on LCD
  while (digitalReadFast(sw2)) {  // MODE button to exit
    push = buttonRead(sw1);   // read button
    if (push == 1) {          // if there was a short push
      item = !item;           // invert item value
      (*f)(item);             // show updated item on LCD
      buttonReset(sw1);       // debounce sw1 and reset push
    }                         // end if
  }                           // end while
  buttonReset(sw1);           // don't leave until user lets go of sw1
  buttonReset(sw2);           // don't leave until user lets go of sw2
  delay(100);                 // extra delay
}

void setLED() {
  TMVCCon();         // turn on Vcc for the TM1637 display
  byte push = 0;     // push will store button result (0: no push, 1: short push, 2: long push)
  buttonReset(sw1);  // make sure sw1 isn't pushed
  //reset the brightness level (2-7)
  showSegments_P(SEG_LED);  // show "LED" message
  delay(DISPTIME_SLOW);
  // First set hours
  display.clear();
  display.showNumberDec(brightness, true, 2, 2);      // show user current brightness
  while (digitalReadFast(sw2)) {                          // MODE button exits
    push = buttonRead(sw1);                           // read button
    if (push == 1) {                                  // if there was a short push
      brightness++;                                   // add 1 to hours
      if (brightness > 7) brightness = 0;             // wrap around brightness
      display.clear();                                // clear display
      display.setBrightness(brightness);              // 0:MOST DIM, 7: BRIGHTEST
      display.showNumberDec(brightness, true, 2, 2);  // show user current brightness
    }                                                 //end if (push==1)
  }                                                   // end while
  buttonReset(sw1);                                   // debounce sw1
  buttonReset(sw2);                                   // debounce sw1
  delay(100);

  display.clear();
  showSegments_P(SEG_BATT);  // show "LED" message
  delay(DISPTIME_SLOW);
  int battLife = constrain((readVcc() / 12) - 167, 0, 100);  // calculate remaining battery life
  display.clear();                                           // clear the display
  display.showNumberDec(battLife, false);                    // show user remaining battery life
  delay(DISPTIME_SLOW);                                      // one last delay
  display.clear();                                           // clear the display
}

byte buttonRead(byte pin) {
  // routine to read a button push
  // 0: button not pushed
  // 1: short push
  // 2: long push
  byte ret = 0;  // byte the function will return
  unsigned long timer1 = millis();
  if (!digitalReadFast(pin)) {                                      // if button is pushed down
    delay(DEBOUNCE);                                            // debounce if button pushed
    ret = 1;                                                    // 1 means short push
    while (!digitalReadFast(pin) && (millis() - timer1) < 600) {};  // 600 msec is timeout
    delay(DEBOUNCE);                                            // debounce if button pushed
  }
  if (millis() - timer1 > 500) ret = 2;  // long push is > 500 msec
  return ret;
}

void buttonReset(byte pin) {
  if (!digitalReadFast(pin)) {  // only do something if pin is pushed down
    while (!digitalReadFast(pin))
      ;
    delay(DEBOUNCE);  // debounce
  }
}

void safeWait(byte pin, unsigned long dly) {  // delay that is interruptable by button push on pin
  unsigned long timer1 = millis();
  while (digitalReadFast(pin) && (millis() - timer1) < dly) {}  // wait but exit on button push
}

byte anyKeyWait(unsigned long dly) {  // delay that is interruptable by either button (program dependent routine)
  byte ret = 0;
  unsigned long timer1 = millis();
  bool sw1State = HIGH;
  bool sw2State = HIGH;
  do {
    sw1State = digitalReadFast(sw1);
    sw2State = digitalReadFast(sw2);
  } while (sw1State && sw2State && (millis() - timer1) < dly);  // wait, but exit on button push
  if (millis() - timer1 < dly) {
    beeped = true;           // silence alarm here
    if (!sw1State) ret = 1;  // if user pressed sw1 button to interrupt delay, return 1
    if (!sw2State) ret = 2;  // if user pressed sw2 button to interrupt delay, return 2
  } else {
    ret = 0;  // if the delay ended without a button push, return 0
  }
  digitalWriteFast(buzzPin, LOW);  // prevents buzzer from being stuck on
  while (!digitalReadFast(sw1) || !digitalReadFast(sw2))
    ;               // wait until both buttons not pushed
  delay(DEBOUNCE);  // debounce
  return ret;
}

byte beepBuzz(byte pin, int n) {  // pin is digital pin wired to buzzer. n is number of series of beeps.
  byte x = 0;
  pinModeFast(pin, OUTPUT);  // set pin to OUTPUT mode
  for (int j = 0; j < n; j++) {
    for (int i = 0; i < 3; i++) {  // 3 beeps
      digitalWriteFast(pin, HIGH);
      x = anyKeyWait(BEEPTIME);  // interruptable wait
      if (x > 0) {
        digitalWriteFast(pin, LOW);  // stop the beep
        pinModeFast(pin, INPUT);
        return x;  // leave routine here if user pressed button
      }
      digitalWriteFast(pin, LOW);
      if (n > 1) {
        x = anyKeyWait(BEEPTIME);  // interruptable wait
        if (x > 0) {
          pinModeFast(pin, INPUT);
          return x;  // leave routine here if user pressed button
        }
      }
    }

    x = anyKeyWait(250);  // final interruptable wait
    if (x > 0) {
      digitalWriteFast(pin, LOW);  // stop the beep
      pinModeFast(pin, INPUT);
      return x;  // leave routine here if user pressed button
    }
  }
  pinModeFast(pin, INPUT);
  return 0;  // exit normally
}

int readCoreTemp(int n) {  // Calculates and reports the chip temperature of ATtiny84
  sbi(ADCSRA, ADEN);     // enable ADC (comment out if already on)
  delay(WARMUP);         // wait for ADC to warm up
  byte ADMUX_P = ADMUX;  // store present values of these two registers
  byte ADCSRA_P = ADCSRA;
  ADMUX = B00100010;  // Page 149 of ATtiny84 datasheet - enable temperature sensor
  cbi(ADMUX, ADLAR);  // Right-adjust result
  sbi(ADMUX, REFS1);
  cbi(ADMUX, REFS0);   // set internal ref to 1.1V
  delay(WARMUP);       // wait for Vref to settle
  cbi(ADCSRA, ADATE);  // disable autotrigger
  cbi(ADCSRA, ADIE);   // disable interrupt
  long avg = 0;        // to calculate mean of n readings
  for (int i = 0; i < n; i++) {
    sbi(ADCSRA, ADSC);                              // single conversion or free-running mode: write this bit to one to start a conversion.
    loop_until_bit_is_clear(ADCSRA, ADSC);          // ADSC will read as one as long as a conversion is in progress. When the conversion is complete, it returns a zero. This waits until the ADSC conversion is done.
    uint8_t low = ADCL;                             // read ADCL first (17.13.3.1 ATtiny85 datasheet)
    uint8_t high = ADCH;                            // read ADCL second (17.13.3.1 ATtiny85 datasheet)
    int adc = (high << 8) | low;                    // combine high and low bits
    long TdegC = (893L * adc)/1000 -244 + (int)TOFFSET;  // Temperature formula, p149 of datasheet. 
    avg += (TdegC - avg) / (i + 1);                 // calculate iterative mean
  }
  ADMUX = ADMUX_P;  // restore original values of these two registers
  ADCSRA = ADCSRA_P;
  cbi(ADCSRA, ADEN);  // disable ADC to save power (comment out to leave ADC on)

  // According to Table 16-2 in the ATtiny84 datasheet, the function to change the ADC reading to
  // temperature is linear. A best-fit line for the data in Table 16-2 yields the following equation:
  //
  //  Temperature (degC) = 0.8929 * ADC - 244.5
  //
  // These coefficients can be replaced by performing a 2-point calibration, and fitting a straight line
  // to solve for kVal and Tos. These are used to convert the ADC reading to degrees Celsius (or another temperature unit).

  return (int)avg+TOFFSET;  // return temperature in degC. TOFFSET is a fudge factor defined at the top of the program.
}

// Show temperature function
void showTemp(float T) {  // temperature
  int Tint = (int)T;
  showSegments_P(SEG_DEGC);             // show degree message
  display.showNumberDec(Tint, false, 2, 0);  // false: don't show leading zeros, Start at first digit (position 0).
}

// Timer Functions
void timer_reset() {  // reset on the fly without the PUSH screen
  tDur = 0;           // reset timer on mode change (comment out if you'd like to change this)
  tEnd = millis();
  beeped = true;
      TMVCCon();                      // turn on Vcc for the TM1637 display
      showSegments_P(SEG_PUSH);  // show "PUSH" message
      delay(DISPTIME_FAST);           // wait a bit
      buttonReset(sw1);               // wait until user lets go of SET button
      TMVCCoff();                     // turn off Vcc for the TM1637 display
      sleep_interrupt();              // go to sleep here (waits in sleep mode, with 0.8 uA current draw)
      TMVCCon();                      // turn on Vcc for the TM1637 display
}

void showTimeTMR(unsigned long msec, bool force) {  // time remaining in msec. force=true forces the display.
  // this function converts milliseconds to h, m, s
  // For a function that takes h,m,s as input arguments, see PB860.pbworks.com
  static unsigned long hlast = 999;
  static unsigned long mlast = 999;
  static unsigned long slast = 999;
  unsigned long h = (msec / 3600000UL);                                   // calculate hours left (rounds down)
  unsigned long m = ((msec - (h * 3600000UL)) / 60000UL);                 // calculate minutes left (rounds down)
  unsigned long s = ((msec - (h * 3600000UL) - (m * 60000UL))) / 1000UL;  // calculate seconds left (rounds down)
  byte dotsMask = (0x80 >> 1);
  if (h != hlast || m != mlast || s != slast || force) {  // only change the display if the time has changed
    if (h > 0) {                                          // if hours left, show hrs and min
      display.showNumberDecEx(h, dotsMask, false, 2, 0);  // true: show leading zero, 0: flush with end
      display.showNumberDec(m, true, 2, 2);               // false: don't show leading zero, 2: start 2 spaces over
    } else if (m > 0) {                                   // if min left, show min and sec
      display.showNumberDecEx(m, dotsMask, false, 2, 0);  // true: show leading zero, 0: flush with end
      display.showNumberDec(s, true, 2, 2);               // false: don't show leading zero, 2: start 2 spaces over
    } else {
      display.showNumberDec(s, false);  // false: don't show leading zero
    }
  }
  hlast = h;  // store current hrs
  mlast = m;  // store current min
  slast = s;  // store current sec
}

//Stopwatch functions
void showTimeSW(unsigned long msec) {  // show time (input argument: msec) for the stopwatch mode
  // This function converts milliseconds to h, m, s
  static byte hlast = 255;
  static byte mlast = 255;
  static byte slast = 255;

  byte h = (msec / 3600000UL);                                             // calculate hours left (rounds down)
  byte m = ((msec - (h * 3600000UL)) / 60000UL);                           // calculate minutes left (rounds down)
  byte s = ((msec - (h * 3600000UL) - (m * 60000UL))) / 1000UL;            // calculate seconds left (rounds down)
  byte ms = (msec - (h * 3600000UL) - (m * 60000UL) - (s * 1000UL)) / 10;  // calculate msec left (rounds down)
  byte dotsMask = 0x80 >> 1;                                               // colon on
  if (h != hlast || m != mlast || s != slast) {                            // only change the display if the time has changed
    if (h > 0) {                                                           // if hours left, show hrs and min
      display.showNumberDecEx(h, dotsMask, false, 2, 0);                   // true: show leading zero, 0: flush with end
      display.showNumberDec(m, true, 2, 2);                                // false: don't show leading zero, 2: start 2 spaces over
    } else if (m > 0) {                                                    // if min left, show min and sec
      display.showNumberDecEx(m, dotsMask, false, 2, 0);                   // true: show leading zero, 0: flush with end
      display.showNumberDec(s, true, 2, 2);                                // false: don't show leading zero, 2: start 2 spaces over
    } else if (s > 0) {
      display.showNumberDecEx(s, dotsMask, false, 2, 0);  // true: show leading zero, 0: flush with end
      display.showNumberDec(ms, true, 2, 2);              // false: don't show leading zero, 2: start 2 spaces over
    } else {
      display.showNumberDec(ms, true, 2, 2);  // false: don't show leading zero, 2: start 2 spaces over
    }
  }
  if (h == 0 && m == 0) {                   // just update msec here (prevents LCD flashing)
    display.showNumberDec(ms, true, 2, 2);  // false: don't show leading zero, 2: start 2 spaces over
  }
  hlast = h;  // store current hrs
  mlast = m;  // store current min
  slast = s;  // store current sec
}

void stopWatch_pause() {
  unsigned long tnow = millis() - toffsetSW;         // calculate ending point
  while (!digitalReadFast(sw1) || !digitalReadFast(sw2)) {}  // wait for user to let go of buttons
  delay(DEBOUNCE);
  while (digitalReadFast(sw1) && digitalReadFast(sw2))
    ;                       // wait for user to press a button again (HIGH=UNPUSHED)
  if (!digitalReadFast(sw2)) {  // if user presses the mode button here
    toffsetSW = millis();   // new starting point
    return;  // get outta dodge
  }
  byte push = buttonRead(sw1);  // if user presses the sw1 button to resume
  if (push == 2) {              // if it's a long push
    stopWatch_reset();          // reset the stopwatch
  } else {
    toffsetSW = millis() - tnow;  // otherwise resume where you left off
  }
}

void stopWatch_reset() {
  display.clear();
  display.showNumberDec(0, true, 2, 2);  // tell user stopwatch is reset
  while (!digitalReadFast(sw1))
    ;                    // wait for user to let go of button
  delay(DEBOUNCE);       // debounce. Pin should be in HIGH state now.
  delay(DISPTIME_FAST);  // delay to show 00
  TMVCCoff();            // turn off Vcc for the TM1637 display
  sleep_interrupt();     // sleep here to save battery life
  TMVCCon();             // turn on Vcc for the TM1637 display
  if (!digitalReadFast(sw2)) {
    return;  // leave routine
  }
  TMVCCon();        // turn on Vcc for the TM1637 displau
  while (!digitalReadFast(sw1) || !digitalReadFast(sw2))
    ;  // make sure user is not touching button
  delay(DEBOUNCE);
  toffsetSW = millis();  // new starting point
}

void TMVCCon() {
  pinModeFast(TMVCC, OUTPUT);     // set TMVCC pin to output mode
  digitalWriteFast(TMVCC, HIGH);  // turn on TM1637 LED display
  delay(WARMUP);              // wait for TM1637 to warm up
  display.clear();
}

void TMVCCoff() {
  display.clear();           // clear the display
  digitalWriteFast(TMVCC, LOW);  // turn off TM1637 LED display to save power
  pinModeFast(TMVCC, INPUT);     // set TMVCC pin to input mode
}

long readVcc() {         // back-calculates voltage (in mV) applied to Vcc of ATtiny84
  sbi(ADCSRA, ADEN);     // enable ADC (comment out if already on)
  delay(WARMUP);         // wait for ADC to warm up
  byte ADMUX_P = ADMUX;  // store present values of these two registers
  byte ADCSRA_P = ADCSRA;
  ADMUX = _BV(MUX5) | _BV(MUX0);          // set Vbg to positive input of analog comparator (bandgap reference voltage=1.1V). Table 16.4 ATtiny24A/44A/84A Datasheet, p151
  delay(WARMUP);                          // Wait for Vref to settle
  sbi(ADCSRA, ADSC);                      // Single conversion or free-running mode: write this bit to one to start a conversion.
  loop_until_bit_is_clear(ADCSRA, ADSC);  // ADSC will read as one as long as a conversion is in progress. When the conversion is complete, it returns a zero. This waits until the ADSC conversion is done.
  uint8_t low = ADCL;                     // read ADCL first (17.13.3.1 ATtiny85 datasheet)
  uint8_t high = ADCH;                    // read ADCL second (17.13.3.1 ATtiny85 datasheet)
  long result = (high << 8) | low;        // combine low and high bits into one reading
  result = 1125300L / result;             // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  ADMUX = ADMUX_P;                        // restore original values of these two registers
  ADCSRA = ADCSRA_P;
  cbi(ADCSRA, ADEN);  // disable ADC to save power (comment out to leave ADC on)
  delay(WARMUP);      // wait a bit
  return result;      // Vcc in millivolts
}

void sleep_interrupt() {  // interrupt sleep routine - this one restores millis() and delay() after.
  //adapted from: https://github.com/DaveCalaway/ATtiny/blob/master/Examples/AT85_sleep_interrupt/AT85_sleep_interrupt.ino
  sbi(GIMSK, PCIE0);                    // enable pin change interrupts
  sbi(PCMSK0, PCINT3);                  // use PB3 as interrupt pin
  sbi(PCMSK0, PCINT4);                  // use PB4 as interrupt pin
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // set sleep mode Power Down
  // The different modes are:
  // SLEEP_MODE_IDLE         -the least power savings
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN     -the most power savings
  //cbi(ADCSRA,ADEN);                         // disable the ADC before powering down
  power_all_disable();  // turn power off to ADC, TIMER 1 and 2, Serial Interface
  sleep_enable();       // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sleep_cpu();          // go to sleep here
  sleep_disable();      // after ISR fires, return to here and disable sleep
  power_all_enable();   // turn everything back on
  //sbi(ADCSRA,ADEN);                         // enable ADC again
  cbi(GIMSK, PCIE0);    // disable pin change interrupts
  cbi(PCMSK0, PCINT3);  // clear PB3 as interrupt pin
  cbi(PCMSK0, PCINT4);  // clear PB4 as interrupt pin
}

ISR(PCINT0_vect) {  // This ISR is always called after waking from sleep mode.
}

void showSegments_P(const uint8_t *p) {
  delayMicroseconds(50);    // wait a bit (in case waking up from sleep)
  uint8_t buf[4]={0};       // define buffer as 4 bytes and clear it out
  for (uint8_t i = 0; i < 4; i++) {
    buf[i] = pgm_read_byte(&p[i]); // read next byte in progmem
  }
  display.setSegments(buf); // display 4 bytes
  delayMicroseconds(50);    // wait a bit
}
