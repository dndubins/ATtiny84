// CTCservo.ino
// Control 3 servos from pins PA2, PA3, and PA4 using CTC mode of Timer1 (ATtiny84)
// Clock speed = 8MHz
// Transferability: This is a very specific sketch! Will only work on the ATtiny84.
// Authors: D.Dubins, Perplexity.AI, and ChatGPT (mostly Perplexity.AI)
// Date: 21-Dec-24

#include <avr/io.h>
#include <avr/interrupt.h>

#define NSVO 3              // number of servos to control
#define CYCLE_LENGTH 20000  // 20ms cycle (50Hz)
#define SVOMAXANGLE 179     // maximum angle for servo.
#define SVOMINPULSE 500     // minimum pulse width in microseconds for servo signal (0 degrees). Default: 500
#define SVOMAXPULSE 2500    // maximum pulse width in microseconds for servo signal (for maximum angle). Default: 2500

#define POTPIN A7  // PA7 (A7) for wiper of potentiometer pin

unsigned int servo_PWs[NSVO] = { 1500, 1500, 1500 };  // Pulse widths in microseconds (default to center position)
volatile bool servo_attached[NSVO] = { 0, 0, 0 };     // Servo attachment status
volatile unsigned long timer_counter = 0;             // For timing pulse widths inside the ISR

void setup() {
  setCTC();
  attachServo(0);
  attachServo(1);
  attachServo(2);
  //detachServo(0);
  //detachServo(1);
  //detachServo(2);
  homeServos();  // start sketch by sending attached servos to home position
}

void loop() {
  // Uncomment for potentiometer control:
  int location = map(analogRead(POTPIN), 1023, 0, 0, SVOMAXANGLE);
  setServo(0, location);  // write new location to servo 0
  setServo(1, location);  // write new location to servo 1
  setServo(2, location);  // write new location to servo 2
  delay(100);             // wait a bit

  //Uncomment to rock servo 1 slowly
  /*for (int i = 0; i < SVOMAXANGLE; i++) {
    setServo(1, i);
    delay(500); // 0.5s delay should let you see each angle
  }
  for (int i = SVOMAXANGLE; i >=0; i--) {
    setServo(1, i);
    delay(500);
  }
  */

  //Uncomment to rock all servos through 0-SVOMAXANGLE sequentially.
  /*for (int i = 0; i < NSVO; i++) {
    setServo(i, 0);
    delay(1000);
    setServo(i, SVOMAXANGLE);
    delay(1000);
  }
  */
}

void attachServo(byte servo_num) {
  if (servo_num < NSVO) {
    servo_attached[servo_num] = true;  // Set servo_attached to true
    DDRA |= (1 << PA2 + servo_num);    // Set servo pin to OUTPUT mode
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
  // Convert pulse width in microseconds to pulse width in # times ISR runs (32 µs for Timer1, 1 us per tick)
  int pulse_width = map(angle, 0, SVOMAXANGLE, SVOMINPULSE, SVOMAXPULSE);  // map angle to pulse width
  pulse_width = constrain(pulse_width, SVOMINPULSE, SVOMAXPULSE);          // constrain pulse width to min and max
  cli();                                                                   // Disable interrupts. It's best to update volatile global variables with interrupts diabled.
  servo_PWs[servo_num] = pulse_width;                                      // Store pulse_width in servo_PWs.
  sei();                                                                   // Enable interrupts. Spend as little time in "disabled interrupt land" as possible.
}

void homeServos() {  // routine to home servos
  // Note: servo will not home unless it is first enabled.
  for (byte i = 0; i < NSVO; i++) {
    setServo(i, 90);  // send all servos to middle position
  }
  delay(1000);  // wait for servos to home
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
  TCCR1B |= _BV(CS11);  // prescaler=8
  //TCCR1B |= _BV(CS11) |  _BV(CS10); // prescaler=64
  //TCCR1B |= _BV(CS12); // prescaler=256
  //TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
  OCR1A = 47;  //OCR1A=(fclk/(N*frequency))-1 (where N is prescaler).
  //Note: OCR1A=63: less angle resolution, less chatter. OCR1A=47: more angle resolution. Lower # seems to result in more chatter/buggy behaviour.
  TIMSK1 |= _BV(OCIE1A);  // enable timer compare
  sei();                  // enable interrupts
}

ISR(TIM1_COMPA_vect) {                  // This is the ISR that will turn off the pins at the correct widths
  timer_counter += 48;                  // ISR will run every (OCR1A+1)µs.
  if (timer_counter >= CYCLE_LENGTH) {  // if the timer reaches the cycle length (20ms)
    timer_counter = 0;                  // reset counter after the full cycle (20ms)
    for (byte i = 0; i < NSVO; i++) {
      // Turn on the servo pin when the timer resets
      if (servo_attached[i]) {      // only turn on pin if servo attached
        PORTA |= (1 << (PA2 + i));  // set servo pin high
      }
    }
  }
  // Handle the pulse generation for each servo pin
  for (byte i = 0; i < NSVO; i++) {
    if (servo_attached[i]) {
      if (timer_counter >= servo_PWs[i]) {
        // Turn off the servo pin if the timer exceeds the pulse width
        PORTA &= ~(1 << (PA2 + i));  // Set pin low
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
