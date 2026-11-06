/*
ATtiny84 Project: controller84_ramp_PWM.ino
Author: D. Dubins
Last Updated: 05-Nov-26
AI Help: Claude.AI Sonnet v.5.5 Medium helped diagnose and tune the control strategy.
Perplexity.AI helped troubleshoot the feed forward strategy.
Description: A ramping temperature controller for melting DNA duplex.
The control loop is feed forward and feedback (PI controller):
  DRIVE = KF_INT + KF*SETPOINT + KP*ERROR + KI*INTEGRAL;
Libraries Used: SendOnlySoftwareSerial Library v1.0.5 by Nick Gammon
Available at: https://github.com/nickgammon/SendOnlySoftwareSerial

Notes: A 100K thermistor is used in series with a 100K resistor. Voltage is read half-way and 
converted to degreesC using the 2-term NTC thermistor equation.
The board is wired to switch a 10A MOSFET
It reports the temperature setpoint and measurement to an 8x2 LCD screen (LCD0802A).
Serial logging is sent to PA6 (disable to save memory). 

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

DC Fan: Not used in this build
------------------------------
CW: to PA6 (PP7): software serial send wire for this build (TX) to Uno for logging
Red Wire: +12V: not used
Black Wire: GND to Uno for logging

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

#define LOGGING  // Comment out to remove Serial commands for logging (saves memory)

#include <EEPROM.h>                   // Include the EEPROM.h library
#include <LiquidCrystal.h>            // Include the LiquidCrystal.h library
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);  // pins PA0-5
#ifdef LOGGING
#include <SendOnlySoftwareSerial.h>  // Include Nick Gammon's SendOnlySoftwareSerial.h library
SendOnlySoftwareSerial mySerial(6);  // Tx pin for PA6 (PP7)
#endif

#define DEBOUNCE 20                    // time to debounce a button
#define drivePin 8                     // Digital pin for PWM temperature control (heater). Note - for pinMode to work, use digital pin numbers here.
#define SW1 10                         // Digital pin for SW1 (PB0). This is the DOWN (or SET) button
#define SW2 9                          // Digital pin for SW1 (PB1). This is the UP button.
#define MESPin 7                       // Analog pin PA7 for temperature acquisition (PP6)
#define NUMREADS 1000                  // number of readings for data smoothing (default: 1000)
#define LOGINTERVAL 30000              // interval in millis() for logging (default 30 seconds)
unsigned long logTimer = LOGINTERVAL;  // to time logging event. Start with a reading.
unsigned long expStart = 0UL;          // to keep track of experiment start time (benchmark for ramp function)
float expTime = 0.0;                   // to keep track of experiment time in minutes
bool expRun = false;                   // run state of ramp experiment. Logging only happens during experiment.

// Controller definitions
#define INTTHRESH 3.0     // integral threshold where INTEGRAL term will engage
#define KF_INT 0.15       // intercept for Feed Forward Gain Constant (default 0.15)
float MEASURED = 0.0;     // To store measured value
float ERROR = 0.0;        // To store error (=SETPOINT-MEASURED)
float INTEGRAL = 0.0;     // To store the integral term for the PI controller
float DRIVE = 0.0;        // To store the DRIVE signal
bool driveState = false;  // To keep track of drive state
bool hcflag=false;        // heating curve has more assertive feed forward constant

// Process control variables to remember:
struct controlVals {  // declare struct in global space
  float TSTART;       // starting temperature (default: 22.0 degC)
  float TEND;         // ending temperature (default: 70.0 degC)
  float TRATE;        // ramp rate (default 0.9 degC/min)
  float TWAIT;        // holding time at startand end (default 5 min)
  bool hystFlag;      // hysteresis flag (false: no ramp down, true: ramp down)
  float SETPOINT;     // SETPOINT temperature: what you would like temperature to be (default: 25)
  float driveMin;     // minimum duty cycle, expressed as a ratio, for heating element (to maintain setpoint) (default: 0.0)
  float driveMax;     // maximum duty cycle, expressed as a ratio, for heating element (to prevent overheating) (default: 1.0)
  float PERIOD;       // CTR1.PERIOD in msec for pulsing drive (default: 3000)
  float KP;           // proportional gain constant (default: 0.1)
  float KI;           // integral gain constant (default: 0.02)
  float KF;           // feed forward gain constant (default: 0.5)
  float TOL;          // tolerance of control (default: 1.0)
  float OFFSET;       // offset adjustment for measurement (default: 0.0)
};                    // Note: semicolon required here
controlVals CTR1;     // use calibration structure for CTR1 (controller1) (to read/write to EEPROM)

void setup() {
  EEPROM.get(0, CTR1);                     // read CTR1 data @addr=0
  if (isnan(CTR1.KP) || CTR1.KP == 0.0) {  // The first time the program is loaded, EEPROM will be empty.
    loadDefaults();                        // Load the correct defaults.
  }                                        // Otherwise, last (saved) EEPROM values will be loaded.
  pinMode(drivePin, OUTPUT);   // set DRIVEPin to OUTPUT mode
  pinMode(SW1, INPUT_PULLUP);  // set SW1 to INPUT_PULLUP mode
  pinMode(SW2, INPUT_PULLUP);  // set SW2 to INPUT_PULLUP mode
  lcd.begin(8, 2);             // initialize LCD
#ifdef LOGGING
  mySerial.begin(9600);  // initialize Serial Monitor
#endif
}

// load the default values
void loadDefaults() {
  CTR1.TSTART = 25.0;    // starting temperature (default: 25.0 degC)
  CTR1.TEND = 70.0;      // ending temperature (default: 70.0 degC)
  CTR1.TRATE = 0.9;      // ramp rate (default 0.9 degC/min)
  CTR1.TWAIT = 5.0;      // holding time at startand end (default 5 min)
  CTR1.hystFlag = true;  // hysteresis flag (false: no ramp down, true: ramp down)
  CTR1.SETPOINT = 25.0;  // SETPOINT temperature: what you would like temperature to be (default: 25 degC)
  CTR1.driveMin = 0.1;   // minimum duty cycle, expressed as a ratio, for heating element (to maintain setpoint) (default: 0.10)
  CTR1.driveMax = 1.0;   // maximum duty cycle, expressed as a ratio, for heating element (to prevent overheating) (default: 1.0)
  CTR1.PERIOD = 5000;    // PERIOD in msec for pulsing drive (default: 5000). Also acts as a delay in drive mode when setpoint is attained.
  CTR1.KP = 0.2;         // proportional gain constant (default: 0.2)
  CTR1.KI = 0.035;       // integral gain constant (default: 0.035)
  CTR1.KF = 0.003;       // feed forward gain constant (default: 0.003)
  CTR1.TOL = 0.5;        // tolerance of control (default: 0.1) Since this is a ramp we want tighter tolerance on the way down
  CTR1.OFFSET = 0.0;     // offset adjustment for measurement (default: 0.0)
  EEPROM.put(0, CTR1);   // write CTR1 to EEPROM at addr=0
  expRun = false;        // cancel current experiment (loadDefaults() is also a bail-out function for the experiment)
}

void loop() {
  MEASURED = readTemp(NUMREADS);  // take average of n readings
  if (MEASURED < -200.0 || MEASURED > 120.0) {  // unrealistic measurement, something went wrong
    heaterOff();                                // turn heater off
    lcd.clear();
    lcd.print("FAULT");
    delay(1000);
    return;  // exit loop here (runs loop again, not a permanent lockout)
  }
  if (expRun) CTR1.SETPOINT = getSP();  // get new setpoint
  ERROR = CTR1.SETPOINT - MEASURED;     // calculate the error
  updateLCD();
  if (ERROR > CTR1.TOL && ERROR < INTTHRESH) {  // prevent integral wind-up by only engaging it close to SET.
    INTEGRAL = INTEGRAL + ERROR;                // accumulate the error integral
  } else {
    INTEGRAL = 0.0;                             // reset the integral term (too far from SETPOINT)
  }
  // Control Code Here:
  if (ERROR > CTR1.TOL) {                        // if ERROR > TOL (MEASURED < SETPOINT - TOL)
    MES_LT_SET();                                // run the routine when MEASURED too low
  } else if (ERROR < -CTR1.TOL) {                // if ERROR < -TOL (MEASURED > SETPOINT + TOL)
    MES_GT_SET();                                // run the routine when MEASURED too high
  } else {                                       // MES within TOL of SETPOINT
    INTEGRAL = 0;                                // we are at the setpoint, reset INTEGRAL
    drivePWM((hcflag?KF_INT:0) + CTR1.KF * CTR1.SETPOINT);  // feedforward carries the steady-state load
    anyKeyWait(CTR1.PERIOD);                     // make sure there's a delay if you are within TOL so the LCD doesn't flicker
  }
}

float readTemp(int n) {  // function to read thermistor temperature
// Thermistor calibration variables
//Reference Temperature (usually 0 degC)
#define T0 273.15      // reference temperature in Kelvin (Default: 273.15)
#define R0 335583.0    // resistance of thermistor at reference temperature (Default: 13453.7)
#define RSENSE 100300  // resistance of sense resistor (in series with thermistor) (Default: 5100.0)
#define BCOEFF 4376.6  // comment out to calculate BCOEFF in sketch (less efficient) (Default: 3434.4)
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

/*float readR(int n) {  // function to read and report thermistor resistance
  // take the average of n readings (using iterative mean approach):
  float avg = 0.0;
  for (int i = 0; i < n; i++) {
    avg += (analogRead(MESPin) - avg) / (i + 1);  // iterative mean calculated here
  }
  float volts = avg * 5.0 / 1023.0;       // read divs and convert to volts
  return RSENSE * volts / (5.0 - volts);  // return resistance
}*/

void MES_LT_SET() {                                                                   // What to do when MEASURED less than SETPOINT. This function relies heavily on global variables.
  DRIVE = ((hcflag?KF_INT:0) + CTR1.KF * CTR1.SETPOINT) + CTR1.KP * ERROR + CTR1.KI * INTEGRAL;  // the larger the error, the larger the drive
  drivePWM(DRIVE);                                                                    // send PWM DRIVE signal to the plant (MES<SET)
  anyKeyWait(CTR1.PERIOD);
}

void MES_GT_SET() {  // What to do when MEASURED greater than SETPOINT. This function relies heavily on global variables.
  heaterOff();
  anyKeyWait(CTR1.PERIOD);
}

void drivePWM(float drive) {                               // drive here is a duty cycle, in percent
  drive = constrain(drive, CTR1.driveMin, CTR1.driveMax);  // constrain drive to limits defined by program and user
  DRIVE = drive;                                           // log what was actually applied
  analogWrite(drivePin, (int)(255 * drive));               // write PWM signal (1.0 drive = 255 PWM)
  if(drive > 0.0){                                         // drive is on now.
    driveState = true;                                     // set driveState to true
    driveIndicator(true);
  }
}

void heaterOff() {
  analogWrite(drivePin, 0);  // turn off heater
  DRIVE = 0.0;               // set DRIVE to 0
  driveState=false;          // set driveState to false
  driveIndicator(false);     // turn off drive indicator
}

void driveIndicator(bool state) {
  lcd.setCursor(7, 1);
  if (state == true) {
    lcd.print("*");  // show heating indicator
  } else {
    lcd.print(" ");  // show heating indicator
  }
  lcd.setCursor(7, 0);  // print an experiment run indicator if experiment is running
  if (expRun) {
    lcd.print("R");
  } else {
    lcd.print(" ");
  }
}

void anyKeyWait(unsigned long dly) {  // delay that is interruptable by either button (program dependent routine)
  unsigned long timer1 = millis();
  byte push1 = 0;  // to hold SW1 state
  byte push2 = 0;  // to hold SW2 state
  do {
    push1 = buttonRead(SW1);                                        // read SW1
    push2 = buttonRead(SW2);                                        // read SW2
#ifdef LOGGING
    logData();                                                      // log the data here
#endif
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
  lcd.setCursor(2, 0);
  lcd.print(CTR1.SETPOINT, 1);
  lcd.setCursor(0, 1);
  lcd.print("M:");
  lcd.setCursor(2, 1);
  lcd.print(MEASURED, 1);
  driveIndicator(driveState);  // redraw the driveState
}

void clearLCD2() {  // clear 2nd line of LCD
  lcd.setCursor(0, 1);
  lcd.print("        ");
  lcd.setCursor(0, 1);
}

/*void setItemInt(int &item, char msg[], int lowLim, int highLim, int i) {  // sets an individual int item during the setupMenu() routine
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
}*/

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
  // opt1 should display when the item is FALSE
  // opt 2 should display when the item is TRUE
  byte push1 = 0;  // to hold SW1 state
  byte push2 = 0;  // to hold SW2 state
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
  setItemBool(restoreDefaults, "RST?", "NO", "YES");  // set drive direction here
  if (restoreDefaults) {
    loadDefaults();
  } else {
    // setItemFloat: four numbers at the end are low, high, delta, #decimals to display
    setItemFloat(CTR1.TSTART, "TSTART", 10.0, 70.0, 2.5, 1);
    setItemFloat(CTR1.TEND, "TEND", 10.0, 70.0, 2.5, 1);   // set the peak (end) temperature
    setItemFloat(CTR1.TRATE, "TRATE", 0.1, 1.0, 0.1, 1);   // set the temperature ramp rate
    setItemFloat(CTR1.TWAIT, "TWAIT", 0, 20, 5, 0);        // set the start, middle, and end wait times (all same duration)
    setItemBool(CTR1.hystFlag, "Hyst?", "NO", "YES");      // hysteresis curve back down to starting temperature?
    setItemFloat(CTR1.KP, "KP", 0.1, 1.0, 0.05, 2);        // set proportional gain constant
    setItemFloat(CTR1.KI, "KI", 0.0, 0.1, 0.005, 3);       // set integral gain constant
    setItemFloat(CTR1.KF, "KF", 0.0, 0.01, 0.0005, 4);     // set feed forward gain constant
    setItemBool(expRun, "Begin?", "NO", "YES");
    if (expRun) {
      expStart = millis();  // time=0 min for experiment start
#ifdef LOGGING
      mySerial.println("Exp start");  // welcome message
#endif
    }
    /* // Advanced setup options (don't need - students will be using this equipment?)
      setItemFloat(CTR1.driveMin, "MINPULSE", 0.0, 0.75, 0.05, 2);
      setItemFloat(CTR1.driveMax, "MAXPULSE", 0.2, 1.0, 0.05, 2);
      setItemFloat(CTR1.PERIOD, "PERIOD", 100, 20000, 100, 0);
      setItemFloat(CTR1.KP, "KP", 0.01, 10.0, 0.05, 2);
      setItemFloat(CTR1.KI, "KI", 0.0, 1.0, 0.01, 2);
      setItemFloat(CTR1.TOL, "TOL:", 0.0, 10.0, 0.25, 2);
      setItemFloat(CTR1.OFFSET, "OFFSET:", -10.0, 10.0, 0.1, 1);
      setItemInt(CTR1.numReads, "#READS:", 1, 1000, 50); // this used to exist and now its hard coded.
      */

    // calibration option (last)
    /*bool calibrate = false;
    setItemBool(calibrate, "CALIB?", "NO", "YES");  // show resistance of thermistor
    if (calibrate) {
      while (digitalRead(SW1) && digitalRead(SW2)) {  // while no buttons are pressed
        lcd.clear();
        lcd.print("R(Ohm)");
        clearLCD2();
        lcd.print(readR(10), 1);
        delay(500);
      }
      delay(DEBOUNCE);
    }*/
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

// This function returns the setpoint based on the current time
float getSP() {
  float expTime = (millis() - expStart) / 60000.0;         // calculate current experiment time
  float t1 = CTR1.TWAIT;                                   // first period - hold at starting temperature
  float t2 = t1 + (CTR1.TEND - CTR1.TSTART) / CTR1.TRATE;  // t2 is the end of the heating curve
  float t3 = t2 + CTR1.TWAIT;                              // t3 is the end of the holding wait time
  float t4 = t3 + (CTR1.TEND - CTR1.TSTART) / CTR1.TRATE;  // t4 is the end of the cooling curve
  float t5 = t4 + CTR1.TWAIT;                              // t5 is the end of the last holding time and the end of the experiment
  if (expTime < t1) {    // Initial waiting period
    hcflag=false;
    return CTR1.TSTART;  
  } else if (expTime < t2) {  // Heating curve period (more assertive feed forward) 
    hcflag=true;
    return CTR1.TRATE * expTime + (CTR1.TEND - CTR1.TRATE * t2);
  } else if (expTime < t3) {  // Peak waiting period (more assertive feed forward to dampen oscillations)
    hcflag=true;
    return CTR1.TEND;
  } else if (expTime < t4) {  // Cooling curve period
    hcflag=false;
    if (CTR1.hystFlag) {
      return -CTR1.TRATE * expTime + (CTR1.TSTART + CTR1.TRATE * t4);
    } else {
      return CTR1.TSTART;  // go back to the starting temperature as fast as possible if no hysteresis
    }
  } else if (expTime < t5) { // Final waiting period
    hcflag=false;
    return CTR1.TSTART;
  } else {
#ifdef LOGGING
    if (expRun) {                // first time this happens
      mySerial.println("End.");  // stop logging
    }
#endif
    hcflag = false;
    expRun = false;      // experiment is over, set global flag to false
    return CTR1.TSTART;  // failsafe
  }
}

#ifdef LOGGING
void logData() {
  if (expRun && millis() - logTimer >= LOGINTERVAL) {
    mySerial.print(millis() / 1000);
    mySerial.print(", ");
    mySerial.print(MEASURED, 1);
    mySerial.print(", ");
    mySerial.println(100.0 * DRIVE, 1);
    logTimer = millis();
  }
}
#endif
