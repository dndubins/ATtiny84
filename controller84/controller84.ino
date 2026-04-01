/*
ATtiny84 Project: controller84+
Author: D. Dubins
Last Updated: 31-Mar-26
This project is to make a (somewhat) universal PI controller for various pieces of 
equipment around the lab. To date, I have used it to control:
-PC Fan with blue control wire
-A lab chiller, for cooling during sonication
-A toaster, to convert it to a low-temperature incubator
-A lab oven
-A styrofoam box incubator
-The cabinet of a gel imager

The unit can either use 4x10W resistors to generate heat, or control a relay for
an external temperature element.

A 5K thermistor is used in series with a 5.1K resistor. Voltage is read half-way and 
converted to degreesC using the 2-term NTC thermistor equation.
The board is designed to either switch a 6A MOSFET or a SPTD relay,
depending on how it is configured.
It reports the temperature setpoint and measurement to an 8x2 LCD screen (LCD0802A).

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

Connections to ATtiny84
-----------------------
+5V to ATtiny84 Physical Pin 1 (Vcc)
GND - ATtiny84 PP14

LCD0802A - ATtiny84:
--------------------
1 Vss - GND
2 Vdd - +5V
3 Vo - Contrast (430R resistor - GND)
4 RS - PA0 (PP13) - D0
5 R/W - GND
6 EN - PA1 (PP12) - D1
7 DB0 - NC
8 DB1 - NC
9 DB2 - NC
10 DB3 - NC
11 DB4 - PA2 (PP11) - D2
12 DB5 - PA3 (PP10) - D3
13 DB6 - PA4 (PP9) - D4
14 DB7 - PA5 (PP8) - D5

Base of transitor to heating element: PB2 (PP5)
Thermistor measured using PA7 (PP6) - this is A7.

DC Fan:
-------
Fan Control Wire (blue) to: PA6 (PP7)
Red Wire: +12V
Black Wire: GND

FQP30N06L: (N-Channel MOSFET)
-----------------------------
1 (Gate) - PB2 (PP5)
2 (Drain) - Heating Element - +12V
3 (Source) - GND
1 - 10K pulldown - 3

78L05 (Linear 5.0V Voltage Regulator)
-------------------------------------
1 (Vout) - +5V
2 (GND) - GND
3 (Vin) - +12V
3 (Vin) - 0.33 uF cap - GND
1 (Vout) - 0.1 uf cap - GND

Momentary Switches:
-------------------
Digital Pin 10 (PB0, PP2) - SW1 - GND
Digital Pin 9 (PB1, PP3) - SW2 - GND
PP4 (RESET) - SW3 - GND 
*/

#include <EEPROM.h>                   // Include the EEPROM.h library
#include <LiquidCrystal.h>            // Include the LiquidCrystal.h library
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);  // pins PA0-5

#define DEBOUNCE 20  // time to debounce a button
#define drivePin 8   // Digital pin for PWM temperature control (heater). Note - for pinMode to work, use digital pin numbers here.
#define fanPin 6     // Digital pin for fan clock signal (digital pin 6, PA6)
#define SW1 10       // Digital pin for SW1 (PB0). This is the DOWN (or SET) button
#define SW2 9        // Digital pin for SW1 (PB1). This is the UP button.
#define MESPin 7     // Analog pin PA7 for temperature acquisition (PP6)

// Controller definitions
#define INTTHRESH 3.0     // integral threshold where INTEGRAL term will engage
float MEASURED = 0.0;     // To store measured value
float ERROR = 0.0;        // To store error (=SETPOINT-MEASURED)
float INTEGRAL = 0.0;     // To store the integral term for the PI controller
float DRIVE = 0.0;        // To store the DRIVE signal
bool driveState = false;  // To keep track of drive state

// Process control variables to remember:
struct controlVals {  // declare struct in global space
  float SETPOINT;     // CTR1.SETPOINT temperature: what you would like temperature to be (default: 25)
  bool slaveFlag;     // if slave, accept external signal on PB2
  bool driveDir;      // Drive direction. 0: engage drive below CTR1.SETPOINT, 1: engage drive above CTR1.SETPOINT. (default: 0)
  bool driveMode;     // 0: ON-OFF mode; 1: pulsed mode (PI)
  bool maintPulse;    // 0: no maintenance pulse within tolerance; 1: maintenance pulse within tolerance
  float driveMin;     // minimum duty cycle, expressed as a ratio, for heating element (to maintain setpoint) (default: 0.0)
  float driveMax;     // maximum duty cycle, expressed as a ratio, for heating element (to prevent overheating) (default: 0.75)
  float PERIOD;       // CTR1.PERIOD in msec for pulsing drive (default: 3000)
  float KP;           // proportional gain constant (default: 0.1)
  float KI;           // integral gain constant (default: 0.02)
  float TOL;          // tolerance of control (default: 1.0)
  float OFFSET;       // offset adjustment for measurement (default: 0.0)
  int fanSpeed;       // fanSpeed=ICR1-(DC*ICR1) x 100 (in percent) (default: 1)
  int numReads;       // number of readings for data smoothing
};                    // Note: semicolon required here
controlVals CTR1;     // use calibration structure for CTR1 (controller1) (to read/write to EEPROM)

void setup() {
  EEPROM.get(0, CTR1);                     // read CTR1 data @addr=0
  if (isnan(CTR1.KP) || CTR1.KP == 0.0) {  // The first time the program is loaded, EEPROM will be empty.
    loadDefaults();                        // Load the correct defaults.
  }
  if (!CTR1.slaveFlag) {
    pinMode(drivePin, OUTPUT);  // set DRIVEPin to OUTPUT mode
  } else {
    pinMode(drivePin, INPUT);  // set DRIVEPin to OUTPUT mode
  }
  pinMode(SW1, INPUT_PULLUP);  // set SW1 to INPUT_PULLUP mode
  pinMode(SW2, INPUT_PULLUP);  // set SW2 to INPUT_PULLUP mode
  pinMode(fanPin, OUTPUT);     // output is PA6 (physical pin 7)
  setFan(CTR1.fanSpeed);       // set the speed of the fan
  lcd.begin(8, 2);             // initialize LCD
}

// Option 1 for defaults:
/*void loadDefaults(controlVals *CTRX) {
  // structure is taken as an input argument here. Not necessary, but if I wanted to save different
  // controller configurations, I could use this syntax to load them separately without changing the code.
  // Below, each command could have been replaced for instance with "CTR1.SETPOINT = 25.0;"
  CTRX->SETPOINT = 25.0;   // SETPOINT temperature: what you would like temperature to be (default: 25)
  CTRX->driveDir = 0;      // Drive direction. 0: engage drive below SETPOINT, 1: engage drive above CTR1.SETPOINT. (default: 0)
  CTRX->pulseFlag = true;  // Pulsed drive (false or true) (default: true)
  CTRX->driveMax = 0.75;   // maximum duty cycle, expressed as a ratio, for heating element (to prevent overheating) (default: 0.75)
  CTRX->PERIOD = 3000.0;   // PERIOD in msec for pulsing drive (default: 3000)
  CTRX->KP = 0.1;          // proportional gain constant (default: 0.1)
  CTRX->TOL = 1.0;         // tolerance of control (default: 1.0)
  CTRX->OFFSET = 0.0;      // offset adjustment for measurement (default: 0.0)
  CTRX->fanSpeed = 1;      // fanSpeed=ICR1-(DC*ICR1) x 100 (in percent) (default: 1)
}
*/

/* Option 2 for defaults:
void loadDefaults(controlVals &CTRX) {  //& prefix gives the void function access to change the original variable
  // structure is taken as an input argument here. Not necessary, but if I wanted to save different
  // controller configurations, I could use this syntax to load them separately without changing the code.
  // Below, each command could have been replaced for instance with "CTR1.SETPOINT = 25.0;"
  CTRX.SETPOINT = 13.0;    // SETPOINT temperature: what you would like temperature to be (default: 25)
  CTRX.driveDir = 1;       // Drive direction. 0: engage drive below SETPOINT, 1: engage drive above CTR1.SETPOINT. (default: 0)
  CTRX.pulseFlag = false;  // Pulsed drive (false or true) (default: true)
  CTRX.driveMax = 0.75;    // maximum duty cycle, expressed as a ratio, for heating element (to prevent overheating) (default: 0.75)
  CTRX.PERIOD = 3000.0;    // PERIOD in msec for pulsing drive (default: 3000)
  CTRX.KP = 0.1;           // proportional gain constant (default: 0.1)
  CTRX.TOL = 1.0;          // tolerance of control (default: 1.0)
  CTRX.OFFSET = 0.0;       // offset adjustment for measurement (default: 0.0)
  CTRX.fanSpeed = 1;       // fanSpeed=ICR1-(DC*ICR1) x 100 (in percent) (default: 1)
  CTRX.numReads = 500;     // number of readings for data smoothing
  EEPROM.put(0, CTRX);     // write CTRX to EEPROM at addr=0
}
*/

/* DEFAULTS FOR VARIOUS DEVICES:
                         Toaster     Cooler      Imager      Chiller     Fridge    Lab Oven (Pulse)  Lab Oven (slow pulse)
  CTR1.SETPOINT            37           37          60          14         5         60              60     
  CTR1.slaveFlag          false      false       false        false       false      false           false
  CTR1.driveDir          0:below     0:below     0:below      1:above     1:above    0:below         0:below
  CTR1.driveMode         1:pulsed    1:pulsed    1:pulsed     0:full      0:full     1:pulsed        1:pulsed
  CTR1.maintPulse        1:yes       1:yes       1:yes        0:no        0:no       1:yes           0:no
  CTR1.driveMin          0.05        0.4         0.4          -           -          0.05            0.5
  CTR1.driveMax          0.4         1.0         0.4          -           -          0.6             1.0
  CTR1.PERIOD            5000        2000        3000         -           -          5000            10000
  CTR1.KP                0.1         0.3         0.1          -           -          0.1             0.1
  CTR1.KI                0.0         0.0         0.0          -           -          0.01            0.01
  CTR1.TOL               0.5         0.5         0.5          1.0         1.0        0.5             0.5
  CTR1.OFFSET            0.0         0.0         0.0          0.0         0.0        0.0             0.0
  CTR1.fanSpeed          100           5           1            1           0        0               0
  CTR1.numReads          500         500         500          500         500        500             500
*/

// Option 3 for defaults (most memory efficient)
void loadDefaults() {
  CTR1.SETPOINT = 60.0;    // SETPOINT temperature: what you would like temperature to be (default: 25)
  CTR1.slaveFlag = false;  // master (false) or slave (true). Slave accepts external signal to drivePin (PB2). Default: false
  CTR1.driveDir = 0;       // Drive direction. 0: engage drive below SETPOINT, 1: engage drive above CTR1.SETPOINT. (default: 0)
  CTR1.driveMode = 1;      // 0: ON-OFF mode; 1: pulsed mode (PI)
  CTR1.maintPulse = 0;     // 0: no maintenance pulse within tolerance; 1: maintenance pulse within tolerance
  CTR1.driveMin = 0.5;     // maximum duty cycle, expressed as a ratio, for heating element (to maintain setpoint) (default: 0.30)
  CTR1.driveMax = 1.0;     // maximum duty cycle, expressed as a ratio, for heating element (to prevent overheating) (default: 0.75)
  CTR1.PERIOD = 10000;     // PERIOD in msec for pulsing drive (default: 3000, 5000 for toaster). Also acts as a delay in drive mode when setpoint is attained.
  CTR1.KP = 0.1;           // proportional gain constant (default: 0.1)
  CTR1.KI = 0.01;          // integral gain constant (default: 0.01)
  CTR1.TOL = 0.5;          // tolerance of control (default: 1.0)
  CTR1.OFFSET = 0.0;       // offset adjustment for measurement (default: 0.0)
  CTR1.fanSpeed = 0;       // fanSpeed=ICR1-(DC*ICR1) x 100 (in percent) (default: 1)
  CTR1.numReads = 500;     // number of readings for data smoothing
  EEPROM.put(0, CTR1);     // write CTRX to EEPROM at addr=0
}

void loop() {
  if (!CTR1.slaveFlag) {                    // if device not in slave mode
    MEASURED = readTemp(CTR1.numReads);     // take average of n readings
    if (MEASURED < -200.0) loadDefaults();  // something bad happened
    ERROR = CTR1.SETPOINT - MEASURED;       // calculate the error
    updateLCD();
    if (abs(ERROR) < INTTHRESH) {   // prevent integral wind-up by only engaging it close to SET.
      INTEGRAL = INTEGRAL + ERROR;  // accumulate the error integral
    } else {
      INTEGRAL = 0.0;  // reset the integral term (too far from SETPOINT)
    }
    // Control Code Here
    if (ERROR > CTR1.TOL) {          // if ERROR > TOL (MEASURED < SETPOINT - TOL)
      MES_LT_SET();                  // run the routine when MEASURED too low
    } else if (ERROR < -CTR1.TOL) {  // if ERROR < -TOL (MEASURED > SETPOINT + TOL)
      MES_GT_SET();                  // run the routine when MEASURED too high
    } else {
      if (CTR1.driveMode == 1) {                   // if driveMode is 1 (pulsed)
        //drivePulse(CTR1.driveMin, CTR1.PERIOD);  // comment this out if you don't want a maintenance pulse (or just set CTR1.driveMin to zero)
      } else {
        anyKeyWait(CTR1.PERIOD);  // make sure there's a delay if you are within TOL so the LCD doesn't flicker
      }
    }
  } else {  // device in slave mode
    lcd.clear();
    if (digitalRead(drivePin)) {  // a HIGH is detected on the drivePin (electronics will be heating)
      lcd.print("ON");
    } else {
      lcd.print("OFF");
    }
    anyKeyWait(CTR1.PERIOD);
  }
}

void setFan(int s) {  //s is fan speed in percent (1 to 100)
  // Custom PWM on Pin PA6 only, using Timer 1:
  //Formula: frequency=fclk/((ICR1+1)*N)
  TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11);  // fast PWM with ICR1 at the top
  TCCR1B = _BV(WGM13) | _BV(WGM12);
  TCCR1B |= _BV(CS10);  // prescaler=1
  //TCCR1B |= _BV(CS11); // prescaler=8
  //TCCR1B |= _BV(CS11) |  _BV(CS10); // prescaler=64
  //TCCR1B |= _BV(CS12); // prescaler=256
  //TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
  ICR1 = 399;                     //enter a value from 0-32,767 to set the frequency
  OCR1A = 399 - (s * 399 / 100);  //duty cycle = (ICR1-OCR1A)/ICR1
}

float readTemp(int n) {  // function to read thermistor temperature
// Thermistor calibration variables
//Reference Temperature (usually 0 degC)
#define T0 273.15      // reference temperature in Kelvin (Default: 273.15)
#define R0 16120.0     // resistance of thermistor at reference temperature (Default: 13453.7)
#define RSENSE 5072.0  // resistance of sense resistor (in series with thermistor) (Default: 5100.0)
#define BCOEFF 3905.0  // comment out to calculate BCOEFF in sketch (less efficient) (Default: 3434.4)
#ifndef BCOEFF         // for calculating BCOEFF locally

#define T1 294.65                                           // second temperature in Kelvin
#define R1 16120.0                                          // resistance of thermistor at second temperature
  float BCOEFF = log(R1 / R0) / ((1.0 / T1) - (1.0 / T0));  // calculate Bcoeff here

#endif
  // take the average of n readings (using iterative mean approach):
  float avg = 0.0;
  for (int i = 0; i < n; i++) {
    avg += (analogRead(MESPin) - avg) / (i + 1);  // iterative mean calculated here
  }
  float volts = avg * 5.0 / 1023.0;                      // read divs and convert to volts
  float R2 = RSENSE * volts / (5.0 - volts);             // Rsense should be about 10K (10000 Ohm)
  float T = (1.0 / T0) + (1.0 / BCOEFF) * log(R2 / R0);  // first part of equation
  T = 1.0 / T;                                           // invert the answer
  T = T - 273.15;                                        // convert to celsius
  return T + CTR1.OFFSET;                                // return average temperature
}

float readR(int n) {  // function to read thermistor temperature
  // take the average of n readings (using iterative mean approach):
  float avg = 0.0;
  for (int i = 0; i < n; i++) {
    avg += (analogRead(MESPin) - avg) / (i + 1);  // iterative mean calculated here
  }
  float volts = avg * 5.0 / 1023.0;       // read divs and convert to volts
  return RSENSE * volts / (5.0 - volts);  // return resistance
}

void MES_LT_SET() {                                        // What to do when MEASURED less than SETPOINT. This function relies heavily on global variables.
  DRIVE = CTR1.KP * abs(ERROR) + CTR1.KI * abs(INTEGRAL);  // the larger the error, the larger the drive
  // Action to take if ERROR>TOL (MEASURED value below SETPOINT-TOL)
  if (CTR1.driveDir == 0) {     // direction=0 means DRIVE engaged when MEASURED is less than CTR1.SETPOINT (ERROR is positive). So drive should go on here, because its less.
    if (CTR1.driveMode == 0) {  // driveMode=0: ON-OFF mode; 1: pulsed mode (PI)
      digitalWrite(drivePin, HIGH);
      driveIndicator(true);
      driveState = true;  // keep track of drivestate for rendering on lcd refresh
      anyKeyWait(CTR1.PERIOD);
    } else {                           // if driveMode is 1 (pulsed)
      drivePulse(DRIVE, CTR1.PERIOD);  // pulse the drive (MES<SET)
    }
  } else {                      // driveDir=1 means DRIVE engaged when MEASURED is greater than CTR1.SETPOINT (ERROR is positive). So drive should go off, here because it's less.
    if (CTR1.driveMode == 0) {  // if driveMode is 0 (ON/OFF)
      digitalWrite(drivePin, LOW);
      driveIndicator(false);
      driveState = false;  // keep track of drivestate for rendering on lcd refresh
      anyKeyWait(CTR1.PERIOD);
    } else {                    // driveDir is 1 and driveMode is 1 (pulsed)
      anyKeyWait(CTR1.PERIOD);  // don't pulse drive if MES < SET and drive engaged when MES > SET (e.g. cooling)
    }
  }
}

void MES_GT_SET() {                                        // What to do when MEASURED greater than SETPOINT. This function relies heavily on global variables.
  DRIVE = CTR1.KP * abs(ERROR) + CTR1.KI * abs(INTEGRAL);  // the larger the error, the larger the drive
  // Action to take if ERROR<-TOL (MEASURED VALUE above SETPOINT+TOL)
  if (CTR1.driveDir == 0) {     // driveDir=0 means DRIVE engaged when MEASURED is less than CTR1.SETPOINT (ERROR is positive). So drive should go off here, because its greater.
    if (CTR1.driveMode == 0) {  // driveMode=0: ON-OFF mode; 1: pulsed mode (PI)
      digitalWrite(drivePin, LOW);
      driveState = false;  // keep track of drivestate for rendering on lcd refresh
      driveIndicator(false);
      anyKeyWait(CTR1.PERIOD);
    } else {
      anyKeyWait(CTR1.PERIOD);  // don't pulse drive if MES > SET and drive engaged when MES < SET (e.g. heating)
    }
  } else {                      // direction=1 means DRIVE engaged when MEASURED is greater than CTR1.SETPOINT (ERROR is positive). So drive should go off, here because it's less.
    if (CTR1.driveMode == 0) {  // ON-OFF modes
      digitalWrite(drivePin, HIGH);
      driveState = true;  // keep track of drivestate for rendering on lcd refresh
      driveIndicator(true);
      anyKeyWait(CTR1.PERIOD);
    } else {                           // driveDir is 1 and driveMode is 1 (pulsed)
      drivePulse(DRIVE, CTR1.PERIOD);  // pulse drive if MES > SET and drive engaged when MES > SET (e.g. cooling)
    }
  }
}

float drivePulse(float drive, int per) {                   // drive here is a duty cycle, in percent
  drive = constrain(drive, CTR1.driveMin, CTR1.driveMax);  // constrain drive to be realistic. Minimum is DRIVEMIN, max is CTR1.driveMax
  if (drive > 0.0) {                                       // if drive==0 don't bother turning on the plant
    digitalWrite(drivePin, HIGH);
    driveIndicator(true);
  }
  anyKeyWait(drive * per);
  if (drive < 1.0) {  // if drive==1 don't bother turning off the plant
    digitalWrite(drivePin, LOW);
    driveIndicator(false);
    anyKeyWait(per - (drive * per));  // wait the remainder of the period.
  }
  driveState = false;  // keep track of drivestate for rendering on lcd refresh. ALways end on a false drivestate.
}

void driveIndicator(bool state) {
  lcd.setCursor(7, 1);
  if (state == true) {
    lcd.print("*");  // show heating indicator
  } else {
    lcd.print(" ");  // show heating indicator
  }
}

void anyKeyWait(unsigned long dly) {  // delay that is interruptable by either button (program dependent routine)
  unsigned long timer1 = millis();
  byte push1 = 0;  // to hold SW1 state
  byte push2 = 0;  // to hold SW2 state
  do {
    push1 = buttonRead(SW1);                                        // read SW1
    push2 = buttonRead(SW2);                                        // read SW2
  } while (push1 == 0 && push2 == 0 && (millis() - timer1) < dly);  // wait, but exit on button push
  buttonReset(SW1);                                                 // don't leave until user lets go of SW1
  buttonReset(SW2);                                                 // don't leave until user lets go of SW2
  delay(100);
  if (push1 == 1) {
    CTR1.SETPOINT -= 1.0;  // subtract 1 degree from CTR1.SETPOINT
    EEPROM.put(0, CTR1);   // write CTR1 to EEPROM at addr=0
  }
  if (push2 == 1) {
    CTR1.SETPOINT += 1.0;  // add 1 degree to CTR1.SETPOINT
    EEPROM.put(0, CTR1);   // write CTR1 to EEPROM at addr=0
  }
  if (push1 == 2) {
    setupMenu();
  }
  updateLCD();
}

void updateLCD() {
  lcd.clear();
  lcd.print("S:");
  lcd.setCursor(3, 0);
  lcd.print(CTR1.SETPOINT, 1);
  //lcd.print(DRIVE, 1); // for debugging DRIVE term
  lcd.setCursor(0, 1);
  lcd.print("M:");
  lcd.setCursor(3, 1);
  lcd.print(MEASURED, 1);
  driveIndicator(driveState);  // redraw the driveState
}

void clearLCD2() {  // clear 2nd line of LCD
  lcd.setCursor(0, 1);
  lcd.print("        ");
  lcd.setCursor(0, 1);
}

void setItemInt(int &item, char msg[], int lowLim, int highLim, int i) {  // sets an individual int item during the setupMenu() routine
  byte push1 = 0;                                                         // to hold SW1 state
  byte push2 = 0;                                                         // to hold SW2 state
  lcd.clear();
  lcd.print(msg);  // print title to LCD
  clearLCD2();     // clear second line
  lcd.print(item);
  do {
    push1 = buttonRead(SW1);               // read SW1
    push2 = buttonRead(SW2);               // read SW2
    if (push2 == 1) {                      // if there was a short push on SW2
      item += i;                           // add 1 to item
      if (item > highLim) item = highLim;  // top
      clearLCD2();
      lcd.print(item);
      buttonReset(SW2);                  // debounce SW2
    } else if (push1 == 1) {             // if there was a short push on SW1
      item -= i;                         // subtract 1 from item
      if (item < lowLim) item = lowLim;  // bottom
      clearLCD2();
      lcd.print(item);
      buttonReset(SW1);                // debounce SW1
    }                                  // end if
  } while (push1 != 2 && push2 != 2);  // end while when long press detected on SW1 or SW2
  buttonReset(SW1);                    // don't leave until user lets go of SW1
  buttonReset(SW2);                    // don't leave until user lets go of SW2
  delay(100);                          // prevents unwonton menu advancing
}

void setItemFloat(float &item, char msg[], float lowLim, float highLim, float delta, float d) {  // sets an individual float item during the setupMenu() routine
  byte push1 = 0;                                                                                // to hold SW1 state
  byte push2 = 0;                                                                                // to hold SW2 state
  lcd.clear();
  lcd.print(msg);  // print title to LCD
  clearLCD2();
  lcd.print(item, d);  // d is the number of decimals to print
  do {
    push1 = buttonRead(SW1);               // read SW1
    push2 = buttonRead(SW2);               // read SW2
    if (push2 == 1) {                      // if there was a short push on SW2
      item += delta;                       // add delta to item
      if (item > highLim) item = highLim;  // top
      clearLCD2();
      lcd.print(item, d);
      buttonReset(SW2);                  // debounce SW2
    } else if (push1 == 1) {             // if there was a short push on SW1
      item -= delta;                     // subtract delta from item
      if (item < lowLim) item = lowLim;  // bottom
      clearLCD2();
      lcd.print(item, d);
      buttonReset(SW1);                // debounce SW1
    }                                  // end if
  } while (push1 != 2 && push2 != 2);  // end while when long press detected on SW1 or SW2
  buttonReset(SW1);                    // don't leave until user lets go of SW1
  buttonReset(SW2);                    // don't leave until user lets go of SW2
  delay(100);                          // prevents unwonton menu advancing
}

void setItemBool(bool &item, char msg[], char opt1[], char opt2[]) {  // sets an individual bool item during the setupMenu() routine
  byte push1 = 0;                                                     // to hold SW1 state
  byte push2 = 0;                                                     // to hold SW2 state
  lcd.clear();
  lcd.print(msg);  // print title to LCD
  clearLCD2();
  if (item) {
    lcd.print(opt2);
  } else {
    lcd.print(opt1);
  }
  do {
    push1 = buttonRead(SW1);           // read SW1
    push2 = buttonRead(SW2);           // read SW2
    if (push2 == 1) {                  // if there was a short push on SW2
      item = true;                     // set item to true
      clearLCD2();                     // clear line2 of LCD
      lcd.print(opt2);                 // print second option to LCD
      buttonReset(SW2);                // debounce SW2
    } else if (push1 == 1) {           // if there was a short push on SW1
      item = false;                    // set item to false
      clearLCD2();                     // clear line2 of LCD
      lcd.print(opt1);                 // print first option to LCD
      buttonReset(SW1);                // debounce SW1
    }                                  // end if
  } while (push1 != 2 && push2 != 2);  // end while when long press detected on SW1 or SW2
  buttonReset(SW1);                    // don't leave until user lets go of SW1
  buttonReset(SW2);                    // don't leave until user lets go of SW2
  delay(100);                          // prevents unwonton menu advancing
}

void setupMenu() {
  digitalWrite(drivePin, LOW);  // avoid having the drive on during setup
  lcd.clear();
  lcd.print("SETUP");
  delay(1000);
  bool restoreDefaults = false;
  setItemBool(restoreDefaults, "DEFAULT?", "NO", "YES");  // set drive direction here
  if (restoreDefaults) {
    loadDefaults();
  } else {
    setItemBool(CTR1.slaveFlag, "DEVICE", "MASTER", "SLAVE");  // unit to master or slave operation
    if (!CTR1.slaveFlag) {
      setItemBool(CTR1.driveDir, "DRIVE ON", "BELOW S", "ABOVE S");  // set drive direction here
      setItemBool(CTR1.driveMode, "DRIVE", "ON/OFF", "PULSED");
      if (CTR1.driveMode > 0) { // if pulsed mode is on
        setItemFloat(CTR1.driveMin, "MINPULSE", 0.0, 0.75, 0.05, 2);
        setItemFloat(CTR1.driveMax, "MAXPULSE", 0.2, 1.0, 0.05, 2);
        setItemBool(CTR1.maintPulse, "MNTPULSE", "OFF", "ON");  // maintenance pulse
        setItemFloat(CTR1.PERIOD, "PERIOD", 100, 20000, 100, 0);
        setItemFloat(CTR1.KP, "KP", 0.01, 10.0, 0.05, 2);
        setItemFloat(CTR1.KI, "KI", 0.0, 1.0, 0.01, 2);
      }
      setItemFloat(CTR1.TOL, "TOL:", 0.0, 10.0, 0.25, 2);
      setItemFloat(CTR1.OFFSET, "OFFSET:", -10.0, 10.0, 0.1, 1);
      setItemInt(CTR1.fanSpeed, "FAN:", 1, 100, 1);
      setFan(CTR1.fanSpeed);  // set the fan speed now
      setItemInt(CTR1.numReads, "#READS:", 1, 1000, 50);
    }
    // calibration option (last)
    bool calibrate = false;
    setItemBool(calibrate, "CALIB?", "NO", "YES");  // set drive direction here
    if (calibrate) {
      while (digitalRead(SW1) && digitalRead(SW2)) {  // while no buttons are pressed
        lcd.clear();
        lcd.print("R(Ohm)");
        clearLCD2();
        lcd.print(readR(10), 1);
        delay(500);
      }
      delay(DEBOUNCE);
    }
  }
  EEPROM.put(0, CTR1);  // write CTR1 to EEPROM at addr=0
}

byte buttonRead(byte pin) {
  // routine to read a button push
  // 0: button not pushed
  // 1: short push
  // 2: long push
  byte ret = 0;  // byte the function will return
  unsigned long timer1 = millis();
  if (!digitalRead(pin)) {                                      // if button is pushed down
    delay(DEBOUNCE);                                            // debounce if button pushed
    ret = 1;                                                    // 1 means short push
    while (!digitalRead(pin) && (millis() - timer1) < 600) {};  // 600 msec is timeout
    delay(DEBOUNCE);                                            // debounce if button pushed
  }
  if (millis() - timer1 > 500) ret = 2;  // long push is > 500 msec
  return ret;
}

void buttonReset(byte pin) {
  if (!digitalRead(pin)) {  // only do something if pin is pushed down
    while (!digitalRead(pin)) {};
    delay(DEBOUNCE);  // debounce
  }
}
