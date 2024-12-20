/*  BruteForceServo.ino sketch (written for the ATtiny84)
 *  David Dubins 19-Dec-24
 *  Last Modified: 19-Dec-24
 *  This sketch does not require any additional libraries.
 *  tinyDriver84 is a motor driver board configurable to control a 4-Wire stepper motor using the A4988 module,
 *  an N-channel MOSFET (FQP30N06L) turning on and off a simple load (like a DC motor pump) without controlling
 *  direction, or three servo motors. An optional limit switch is built into the board.
 *  This is the code if setting up the board to control 3 servo motors from pins 2, 3, and 4.
 *  After printing the board, I realized that these pins are not PWM-enabled. So I wrote custom functions to
 *  control the servos.
 *
 *  Connections:
 *  ============
 *  Servos: 
 *  Brown Wire - GND
 *  Red Wire - +5V
 *  Servo 0 Yellow Wire - PA2
 *  Servo 1 Yellow Wire - PA3
 *  Servo 2 Yellow Wire - PA4
 */

#define NSVO 3     // number of servos to control
#define POTPIN A7  // PA7 (A7) for wiper of potentiometer pin
// Servo Parameters:
byte sPin[NSVO] = { 2, 3, 4 };  // pin #s for servos PA2(D2), PA3(D3), PA4(D4)
#define SVOMAXANGLE 179         // maximum angle for servo.
#define SVOMINPULSE 500         // minimum pulse width in microseconds for servo signal (0 degrees). Default: 500
#define SVOMAXPULSE 2500        // maximum pulse width in microseconds for servo signal (for maximum angle). Default: 2500
#define SVOMAXTIME 1000         // maximum time it should take for servo to rotate at full speed between low and high limits.

void setup() {
  for (int i = 0; i < NSVO; i++) pinMode(sPin[i], OUTPUT);  // set all servo pins to OUTPUT mode
  homeServos();
}

void loop() {
  int location = map(analogRead(POTPIN), 0, 1023, SVOMAXANGLE, 0);
  servoWriteSame(location, 100);  // write location to all 3 servos
  //moveTo(0, 179, 0, 1);      //go to 0,179,0. Larger last number = slower movement.
  //moveTo(90, 90, 179, 1);    //go to 90,90,179
  //moveTo(179, 0, 90, 1);     //go to 179,0,90
  //moveTo(0, 0, 0, 1);        //go to 0,0,0
  //moveTo(179, 179, 179, 1);  //go to 179,179,179
  //moveTo(0, 0, 0, 1);        //go to 0,0,0
  //moveTo(179, 179, 179, 1);  //go to 179,179,179
  //servoWriteAll(0, 179, 0, 1000);       //go to 0,179,0, signal should last 1000 msec
  //servoWriteAll(90, 90, 179, 1000);     //go to 90,90,179, signal should last 1000 msec
  //servoWriteAll(179, 0, 90, 1000);      //go to 179,0,90, signal should last 1000 msec
  //servoWriteAll(0, 0, 0, 1000);         //go to 0,0,0 signal should last 1000 msec
  //servoWriteAll(179, 179, 179, 1000);   //go to 179,179,179 signal should last 1000 msec
}

void moveTo(byte s0, byte s1, byte s2, int wait) {  // routine for controlling 3 servos slowly, simultaneously.
  // wait=0: as fast as possible. do not use wait < 10 msec.
  // Change structure of moveTo based on # servos needed (add coordinates)
  byte loc[NSVO] = { s0, s1, s2 };                      //create array for loc’ns
  static int pos[NSVO];                                 // remembers last value of pos
  int dev = 0;                                          // to track deviation
  loc[0] = constrain(loc[0], 0, SVOMAXANGLE);           // limits for servo 0
  loc[1] = constrain(loc[1], 0, SVOMAXANGLE);           // limits for servo 1
  loc[2] = constrain(loc[2], 0, SVOMAXANGLE);           // limits for servo 2
  if (wait == 0) {                                      // if wait is zero, do the job as fast as possible.
    servoWriteAll(pos[0], pos[1], pos[2], SVOMAXTIME);  // write new position to servos
  } else {                                              // do the job in steps
    do {
      dev = 0;
      for (int i = 0; i < NSVO; i++) {  // moves servos one step
        if (loc[i] > pos[i]) pos[i]++;  // add 1 to pos[i]
        if (loc[i] < pos[i]) pos[i]--;  // subtr 1 from pos[i]
        dev += abs(pos[i] - loc[i]);    // calculate deviation
      }
      servoWriteAll(pos[0], pos[1], pos[2], wait);  // write new position to servos
    } while (dev > 0);                              // stop when location attained
  }
}

void homeServos() {         // even servo motors can be homed
  servoWriteSame(0, 1000);  // send the same signal to all 3 servos
}

void servoWrite(byte pin, byte angle, unsigned int dur) {  // write a signal to a single servo for time dur (msec)
// 50Hz custom duty cycle routine for servo control
// This routine should be portable to other MCUs.
  unsigned long timer1 = millis();                                          // start the timer
  unsigned int tON = map(angle, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);  // pulse width usually 1000-2000us (full range 500-2500us)
  unsigned int tOFF = 20000 - tON;                                          // a 50 Hz pulse has a period of 20,000 us. tOFF should be 20,000-tON.
  while (millis() - timer1 < dur) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(tON);
    digitalWrite(pin, LOW);
    delayMicroseconds(tOFF);
  }
}

void servoWriteSame(byte angle, unsigned int dur) {  // write the same signal to all 3 servos
// This routine was written for the ATtiny84. Consult PORT architecture if using for other MCUs.
  unsigned long timer1 = millis();
  unsigned int tON = map(angle, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);
  unsigned int tOFF = 20000 - tON;  // a 50 Hz pulse has a period of 20,000 us. tOFF should be 20,000-tON.
  while (millis() - timer1 < dur) {
    //for(int i=0;i<3;i++)digitalWrite(sPin[i],HIGH); // set all servo pins HIGH
    PORTA |= (1 << PA2) | (1 << PA3) | (1 << PA4);  // set pins PA2, PA3, and PA4 HIGH at the same time
    delayMicroseconds(tON);
    //for (int i = 0; i < 3; i++) digitalWrite(sPin[i], LOW);  // set all servo pins LOW
    PORTA &= ~((1 << PA2) | (1 << PA3) | (1 << PA4));  // set pins PA2, PA3, and PA4 LOW at the same time
    delayMicroseconds(tOFF);
  }
}

void servoWriteAll(byte s0, byte s1, byte s2, unsigned int dur) {  // write different signals to all servos (more efficient)
// This routine was written for the ATtiny84. Consult PORT architecture if using for other MCUs.
  unsigned long timer1 = millis();
  unsigned int tON0 = map(s0, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);  // tON for Servo 0.
  unsigned int tON1 = map(s1, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);  // tON for Servo 1.
  unsigned int tON2 = map(s2, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);  // tON for Servo 2.
  while (millis() - timer1 < dur) {                                       // repeat for time dur in msec
    unsigned long timer2 = micros();                                      // start the microsecond timer
    PORTA |= (1 << PA2) | (1 << PA3) | (1 << PA4);                        // set pins PA2, PA3, and PA4 HIGH at the same time
    while (micros() - timer2 < 20000) {  // a 50 Hz pulse has a period of 20,000 us.
      if (micros() - timer2 > tON0) PORTA &= ~(1 << PA2);  // turn off PA2 at the right time
      if (micros() - timer2 > tON1) PORTA &= ~(1 << PA3);  // turn off PA3 at the right time
      if (micros() - timer2 > tON2) PORTA &= ~(1 << PA4);  // turn off PA4 at the right time
    }
  }
}
