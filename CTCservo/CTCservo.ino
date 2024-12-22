// CTCservo.ino
// Control 3 servos from pins PA2, PA3, and PA4 using CTC mode of Timer1 (ATtiny84)
// Clock speed = 8MHz
// Transferability: This is a very specific sketch! Will only work on the ATtiny84.
// However, if you have a good understanding of timers and CTC mode, you can adapt it to
// non-PWM pins of other MCUs as well.
// Authors: D.Dubins, Perplexity.AI, and ChatGPT (mostly Perplexity.AI)
// Date: 19-Dec-24
// Last Updated: 22-Dec-24

#include <avr/io.h>
#include <avr/interrupt.h>

#define NSVO 3            // number of servos to control
#define SVOMAXANGLE 179   // maximum angle for servo.
#define SVOMINPULSE 500   // minimum pulse width in microseconds for servo signal (0 degrees). Default: 500
#define SVOMAXPULSE 2500  // maximum pulse width in microseconds for servo signal (for maximum angle). Default: 2500
#define SVOTIMEOUT 500    // timeout in ms to disable servos. Should be long enough to attain setpoint.

#define POTPIN A7  // PA7 (A7) for wiper of potentiometer pin

unsigned int servo_PWs[NSVO] = { 1500, 1500, 1500 };  // Pulse widths in microseconds (default to center position)
bool servo_attached[NSVO] = { 0, 0, 0 };              // Servo attachment status

void setup() {
  setCTC();        // set CTC mode to start a 50Hz timer for the servo signals
  attachServo(0);  // attach servos here
  attachServo(1);
  attachServo(2);
  //detachServo(0); // if you would like to detach a specific servo at any time:
  //detachServo(1);
  //detachServo(2);
  homeServos();  // start sketch by sending attached servos to home position
}

void loop() {
  // This servo_timeout_check() is optional. Temporarily turning off Timer1 will free mcu to do other things.
  servo_timeout_check();  // if servos are inactive, stop Timer1 (less trouble for other routines)

  // Uncomment for potentiometer control:
  int location = map(analogRead(POTPIN), 1023, 0, 0, SVOMAXANGLE); // take pot reading & remap to angle.
  setServo(0, location);  // write new location to servo 0
  setServo(1, location);  // write new location to servo 1
  setServo(2, location);  // write new location to servo 2
  delay(50);              // wait a bit to reduce jittering

  //Uncomment for potentiometer control of all servos with slower movement:
  //int location = map(analogRead(POTPIN), 1023, 0, 0, SVOMAXANGLE);
  //moveTo(location, location, location, 5);  // move to new location, delay=4 ms between steps

  //Uncomment to rock servo 1 slowly
  /*for (int i = 0; i < SVOMAXANGLE; i++) {
    setServo(1, i);
    delay(500); // 0.5s delay should let you see each angle
  }
  for (int i = SVOMAXANGLE; i >=0; i--) {
    setServo(1, i);
    delay(500);
  }*/

  //Uncomment to rock all servos through 0-SVOMAXANGLE sequentially.
  /*for (int i = 0; i < NSVO; i++) {
    setServo(i, 0);
    delay(1000);
    setServo(i, SVOMAXANGLE);
    delay(1000);
  }*/

  //Uncomment for aggressive movement of all servos simultaneously.
  /*setServo(0, 0);
  setServo(1, 0);
  setServo(2, 0);
  delay(1000);
  setServo(0, SVOMAXANGLE);
  setServo(1, SVOMAXANGLE);
  setServo(2, SVOMAXANGLE);
  delay(1000);*/
}

void attachServo(byte servo_num) {
  if (servo_num < NSVO) {
    servo_attached[servo_num] = true;  // Set servo_attached to true
    DDRA |= (1 << (PA2 + servo_num));  // Set servo pin to OUTPUT mode
  }
}

void detachServo(byte servo_num) {
  if (servo_num < NSVO) {
    servo_attached[servo_num] = false;
    PORTA &= ~(1 << (PA2 + servo_num));  // Set servo pin low
    DDRA &= ~((1 << PA2 + servo_num));   // Set servo pin to INPUT mode (less chatter when not doing anything)
  }
}

void setServo(byte servo_num, int angle) {
  int pulse_width = map(angle, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);  // convert angle to pulse width in microseconds
  pulse_width = constrain(pulse_width, SVOMINPULSE, SVOMAXPULSE);          // constrain pulse width to min and max
  if (pulse_width != servo_PWs[servo_num && servo_attached[servo_num]) {   // Disable interrupts only if signal changes and servo is attached
    cli();                                                                 // Disable interrupts. It's best to update volatile global variables with interrupts diabled.
    servo_PWs[servo_num] = pulse_width;                                    // Store new pulse_width in servo_PWs.
    sei();
  }  // Enable interrupts. Spend as little time in "disabled interrupt land" as possible.
}

void homeServos() {  // routine to home servos
  // Note: servo will not home unless it is first enabled.
  for (byte i = 0; i < NSVO; i++) {
    setServo(i, 90);  // send all servos to middle position
  }
  delay(1000);  // wait for servos to home
}

void moveTo(int s0, int s1, int s2, int wait) {  // routine for controlling 3 servos slowly, simultaneously.
  // wait=0: as fast as possible. do not use wait < 10 msec.
  // Change structure of moveTo based on # servos needed (add coordinates)
  int loc[NSVO] = { s0, s1, s2 };  // create array for loc’ns
  static int pos[NSVO];            // remembers last value of pos
  if (wait == 0) {                 // if wait=0, move as fast as possible
    for (int i = 0; i < NSVO; i++) {
      setServo(i, loc[i]);  // write new position to servos
    }
  } else {
    int dev = 0;  // to track deviation
    do {
      dev = 0;
      for (int i = 0; i < NSVO; i++) {  // moves servos one step
        if (loc[i] > pos[i]) pos[i]++;  // add 1 to pos[i]
        if (loc[i] < pos[i]) pos[i]--;  // subtr 1 from pos[i]
        dev += abs(pos[i] - loc[i]);    // calculate deviation
      }
      for (int i = 0; i < NSVO; i++) {
        setServo(i, pos[i]);  // write new position to servos
      }
      delay(wait);      // slow down movement
    } while (dev > 0);  // stop when location attained
  }                     // end if
}

void setCTC() {  // setting the registers of the ATtiny84 for CTC mode
  // Setting up Timer1 for 1µs ticks (assuming 8MHz clock)
  cli();                // stop interrupts
  TCCR1A = 0;           // clear timer control register A
  TCCR1B = 0;           // clear timer control register B
  TCNT1 = 0;            // set counter to 0
  TCCR1B = _BV(WGM12);  // CTC mode (Table 12-5 on ATtiny84 datasheet)
  //TCCR1B = _BV(WGM13) | _BV(WGM12);
  //TCCR1B |= _BV(CS10);  // prescaler=1
  //TCCR1B |= _BV(CS11);  // prescaler=8
  TCCR1B |= _BV(CS11) | _BV(CS10);  // prescaler=64
  //TCCR1B |= _BV(CS12); // prescaler=256
  //TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
  OCR1A = 2499;  //OCR1A=(fclk/(N*frequency))-1 (where N is prescaler).
  //N=64, OCR1A=2499: 50Hz cycle, 8us per tick.
  //N=8, OCR1A=19999: 50Hz cycle, 1us per tick.
  TIMSK1 |= _BV(OCIE1A);  // enable timer compare
  sei();                  // enable interrupts
}

ISR(TIM1_COMPA_vect) {  // This is the ISR that will turn off the pins at the correct widths
  //The function micros() does not advance inside the ISR.
  //TCNT1 starts at 0 and counts up. Each increment lasts 8 microseconds. We are going to use this as a timer.
  //8 microseconds gives us (2500-500us)/8us =250 steps. This is fine for a 180 degree servo. For a 360 degree servo,
  //if more steps are needed, you can set the timer to N=8, OCR1A=19999 and this will give 2000 steps, but require
  //more attention by the ISR.
  for (byte i = 0; i < NSVO; i++) {
    // Turn on the servo pin
    if (servo_attached[i]) {      // only turn on pin if servo attached
      PORTA |= (1 << (PA2 + i));  // set correct servo pin high
    }
  }

  while ((TCNT1 * 8) < (SVOMAXPULSE + 10)) {  // multiply TCNT1 by microseconds/step
    // a 50 Hz pulse has a period of 20,000 us. We just need to make it past SVOMAXPULSE with a small buffer.
    for (byte i = 0; i < NSVO; i++) {
      if (servo_attached[i] && (TCNT1 * 8) > servo_PWs[i]) {
        // Turn off the servo pin if the timer exceeds the pulse width
        PORTA &= ~(1 << (PA2 + i));  // Set correct servo pin low
      }
    }
  }
}

void disableTimerInterrupt() {  // run this if you'd like to disable CTC timer interrupt. This will disable all servos.
  TIMSK1 &= ~(1 << OCIE1A);     // Disable Timer1 Compare Match A interrupt
}

void enableTimerInterrupt() {  // run this if you'd like to (re)enable CTC timer interrupt
  TIMSK1 |= (1 << OCIE1A);     // Enable Timer1 Compare Match A interrupt
}

void servo_timeout_check() {  // this routine disables the timers on inactivity and enables them on a reading change.
  static int totalLast;
  static unsigned long servo_timer;  // servo timer
  int total = 0;
  for (int i = 0; i < NSVO; i++) total += servo_PWs[i];
  if (abs(total - totalLast) > 10) {  // if reading changed beyond noise
    servo_timer = millis();           // reset the timer
    enableTimerInterrupt();           // make sure timer1 is enabled
  }
  if (millis() - servo_timer > SVOTIMEOUT) disableTimerInterrupt();
  totalLast = total;  // store total to totalLast
}
