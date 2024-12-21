// CTCservo.ino
// Control 3 servos from pins PA2, PA3, and PA4 using CTC mode of Timer1 (ATtiny84)
// Transferability: This is a very specific sketch! Will only work on the ATtiny84.
// Author: D.Dubins, Perplexity.AI, and ChatGPT (mostly Perplexity.AI)
// Date: 20-Dec-24

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
  DDRA |= (1 << PA2) | (1 << PA3) | (1 << PA4);  // Set servo pins as outputs
  attachServo(0);
  attachServo(1);
  attachServo(2);
  //detachServo(1);
  //detachServo(2);
  //detachServo(3);
}

void loop() {
  int location = map(analogRead(POTPIN), 0, 1023, SVOMINPULSE, SVOMAXPULSE);
  setServo(0, location);  // write new location to servo 0
  setServo(1, location);  // write new location to servo 0
  setServo(2, location);  // write new location to servo 0
}

void attachServo(byte servo_num) {
  if (servo_num < NSVO) {
    servo_attached[servo_num] = true;
  }
}

void detachServo(byte servo_num) {
  if (servo_num < NSVO) {
    servo_attached[servo_num] = false;
    PORTA &= ~(1 << (PA2 + servo_num));  // Set pin low
  }
}

void setServo(byte servo_num, int pulse_width) {
  if (servo_num < NSVO) {
    // Convert pulse width in microseconds to pulse width in # times ISR runs (64 µs for Timer1, 1 us per tick)
    servo_PWs[servo_num] = pulse_width;  // Store pulse_width in servo_PWs
  }
}

void setCTC() {  //setting the registers aof the ATtiny84 for CTC mode
  // Set up Timer1 for 1µs ticks (assuming 8MHz clock)
  cli();                  //stop interrupts
  TCCR1A = 0;             //clear timer control register A
  TCCR1B = 0;             //clear timer control register B
  TCNT1 = 0;              //set counter to 0
  TCCR1B = _BV(WGM12);    //CTC mode (Table 12-5 on ATtiny84 datasheet)
  //TCCR1B = _BV(WGM13) | _BV(WGM12);
  //TCCR1B |= _BV(CS10);  // prescaler=1
  TCCR1B |= _BV(CS11);  // prescaler=8
  //TCCR1B |= _BV(CS11) |  _BV(CS10); // prescaler=64
  //TCCR1B |= _BV(CS12); // prescaler=256
  //TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
  OCR1A = 31;           //OCR1A=(fclk/(N*frequency))-1 (where N is prescaler)
  TIMSK1 |= _BV(OCIE1A);  //enable timer compare
  sei();                  // enable interrupts
}

ISR(TIM1_COMPA_vect) {                  // This is the ISR that will turn off the pins at the correct widths
  timer_counter += 32;                  // ISR will run every 32µs
  if (timer_counter >= CYCLE_LENGTH) {  // if the timer reaches the cycle length (20ms)
    timer_counter = 0;                  // Reset counter after the full cycle (20ms)
    for (byte i = 0; i < NSVO; i++) {
      // Turn on the servo pin when the timer resets
      PORTA |= (1 << (PA2 + i));  // Set pin high
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
