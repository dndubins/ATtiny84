# Timed ISRs on the ATtiny84 using CTC mode

The ATtiny84 is a very capable MCU that I almost use now by default for small devices. It has plenty of pins, and can do most things the ATmega328p can. One of the things I like to do is to use timers to plan ISRs to run, because ISRs are efficient, accurate and can run in the background of your sketch behind a delay() statement or whatever else you are doing.

The ATtiny84 has two timers. Timer 0 is 8 bits, and Timer 1 is 16 bits. If you use Timer 0 for anything, you are putting other time commands at risk. For instance, the delay(), millis(), and other time commands will be impacted and wrong when you set prescalars/etc. So, we will start with Timer 1, because it's less risky.<p>

Timer 1 (16 bit)
---------------
Here is the code to get Timer 1 into CTC mode:
```
  // CTC Match Routine using Timer 1
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
```
Other than the prescaler, OCR1A is the only number we need to set here. It behaves according to the following frequency chart, assuming an 8MHz clock speed: (all table values in Hz)

 | OCR1A | 	1 | 	8 | 	64 | 	256 | 	1024 | 
| --- |	--- |	--- |	--- |	--- |	--- |
 | 1 | 	4000000 | 	500000 | 	62500 | 	15625 | 	3906.3 | 
 | 2 | 	2666666.7 | 	333333.3 | 	41666.7 | 	10416.7 | 	2604.2 | 
 | 5 | 	1333333.3 | 	166666.7 | 	20833.3 | 	5208.3 | 	1302.1 | 
 | 7 | 	1000000 | 	125000 | 	15625 | 	3906.3 | 	976.6 | 
 | 10 | 	727272.7 | 	90909.1 | 	11363.6 | 	2840.9 | 	710.2 | 
 | 25 | 	307692.3 | 	38461.5 | 	4807.7 | 	1201.9 | 	300.5 | 
 | 50 | 	156862.7 | 	19607.8 | 	2451 | 	612.7 | 	153.2 | 
 | 75 | 	105263.2 | 	13157.9 | 	1644.7 | 	411.2 | 	102.8 | 
 | 100 | 	79207.9 | 	9901 | 	1237.6 | 	309.4 | 	77.4 | 
 | 250 | 	31872.5 | 	3984.1 | 	498 | 	124.5 | 	31.1 | 
 | 500 | 	15968.1 | 	1996 | 	249.5 | 	62.4 | 	15.6 | 
 | 750 | 	10652.5 | 	1331.6 | 	166.4 | 	41.6 | 	10.4 | 
 | 1000 | 	7992 | 	999 | 	124.9 | 	31.2 | 	7.8 | 
 | 2500 | 	3198.7 | 	399.8 | 	50 | 	12.5 | 	3.1 | 
 | 5000 | 	1599.7 | 	200 | 	25 | 	6.2 | 	1.6 | 
 | 7500 | 	1066.5 | 	133.3 | 	16.7 | 	4.2 | 	1 | 
 | 10000 | 	799.9 | 	100 | 	12.5 | 	3.1 | 	0.8 | 
 | 20000 | 	400 | 	50 | 	6.2 | 	1.6 | 	0.4 | 
 | 30000 | 	266.7 | 	33.3 | 	4.2 | 	1 | 	0.3 | 
 | 40000 | 	200 | 	25 | 	3.1 | 	0.8 | 	0.2 | 
 | 50000 | 	160 | 	20 | 	2.5 | 	0.6 | 	0.2 | 
 | 60000 | 	133.3 | 	16.7 | 	2.1 | 	0.5 | 	0.1 | 
 | 65535 | 	122.1 | 	15.3 | 	1.9 | 	0.5 | 	0.1 | 

You can disable Timer1 temporarily at any time in your code using the following command:<p>
TIMSK1 &= ~(1 << OCIE1A);  //disable CTC mode for Timer 1<p>
and then re-enable it using this command:<p>
TIMSK1 |= (1 << OCIE1A);   //re-enable CTC mode for Timer 1<p>
This could be important if other routines in your code need to use Timer1. Also, if you'd like to access (or change) the value of the counter inside the ISR, TCNT1 (Timer/Counter 1 register) is the register to use. So for instance, your sketch can reset this just by using:<p>
TCNT1=0;<p>
or it can use it to measure time inside the ISR since the ISR reset, by doing something like,<p>
if(TCNT1==10)digitalWrite(pin,HIGH);<p>

Speaking of ISRs, now we can talk about how the entire sketch might look if you are using Timer1 to time an event.
Let's write a sketch that prints the time in millis() to the Serial Monitor at a frequency of once per second (1 Hz, as set in the code above).
Here's what the sketch could look like:

```
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
  // CTC Match Routine using Timer 1
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
```
Note that any variables changed inside the ISR need to be declared as volatile in global space. This protects them
from being changed in unexpected ways.

Timer 0 (8 bit)
---------------
Timer 0 is set to CTC mode in a similar way. Using OCR0A (range 0-255), you can control the frequency of Timer 0 using the formula: frequency=fclk/((OCR0A+1)*N). Your choices for a prescaler value for Timer 0 are 1, 8, 64, 256, and 1024. Here is the code to get Timer 0 into CTC mode:

```
// CTC Match Routine using Timer 0
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
OCR0A = 124;            // OCR0A=(fclk/(N*frequency))-1 (where N is prescaler).
TIMSK0 |= _BV(OCIE0A);  // enable interrupt on Compare Match A for Timer0
sei();                  // Enable global interrupts
```

Here is a chart of frequencies (in Hz) spanning your options, assuming an 8MHz clock speed:
 | OCR0A | 	1 | 	8 | 	64 | 	256 | 	1024 | 
| --- |	--- |	--- |	--- |	--- |	--- |
 | 1 | 	4000000 | 	500000 | 	62500 | 	15625 | 	3906.3 | 
 | 5 | 	1333333.3 | 	166666.7 | 	20833.3 | 	5208.3 | 	1302.1 | 
 | 10 | 	727272.7 | 	90909.1 | 	11363.6 | 	2840.9 | 	710.2 | 
 | 20 | 	380952.4 | 	47619 | 	5952.4 | 	1488.1 | 	372 | 
 | 40 | 	195122 | 	24390.2 | 	3048.8 | 	762.2 | 	190.5 | 
 | 60 | 	131147.5 | 	16393.4 | 	2049.2 | 	512.3 | 	128.1 | 
 | 80 | 	98765.4 | 	12345.7 | 	1543.2 | 	385.8 | 	96.5 | 
 | 100 | 	79207.9 | 	9901 | 	1237.6 | 	309.4 | 	77.4 | 
 | 120 | 	66115.7 | 	8264.5 | 	1033.1 | 	258.3 | 	64.6 | 
 | 140 | 	56737.6 | 	7092.2 | 	886.5 | 	221.6 | 	55.4 | 
 | 160 | 	49689.4 | 	6211.2 | 	776.4 | 	194.1 | 	48.5 | 
 | 180 | 	44198.9 | 	5524.9 | 	690.6 | 	172.7 | 	43.2 | 
 | 200 | 	39801 | 	4975.1 | 	621.9 | 	155.5 | 	38.9 | 
 | 220 | 	36199.1 | 	4524.9 | 	565.6 | 	141.4 | 	35.4 | 
 | 240 | 	33195 | 	4149.4 | 	518.7 | 	129.7 | 	32.4 | 
 | 255 | 	31250 | 	3906.3 | 	488.3 | 	122.1 | 	30.5 | 

Putting this all together, we can write a sketch to ask the ATtiny85 to take an analogRead() from pin A0,
at a frequency of about once per second (1 Hz). Note that the maximum value of OCR0A and a pre-scaler 1024 really limits us if we need to go slower than 30 Hz. 
The slowest we can do anything is about 30 Hz. So how can we accomplish this? We need to track the number of rollovers the ISR does, and then act inside the ISR accordingly.
We can just keep track of the total time elapsed with a global volatile integer. Here's a stab at this idea:

```
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
  // CTC Match Routine using Timer 0
  // Formula: frequency=fclk/((OCR0A+1)*N)
  cli();                // stop interrupts
  TCCR0A = 0;          // Clear TCCR0A
  TCCR0B = 0;          // Clear TCCR0B
  TCNT0 = 0;           // Initialize counter
  TCCR0A |= (1 << WGM01); // Set CTC mode
  //TCCR0B = _BV(CS00);  // prescaler=1 (no prescaling)   (Table 13-9, ATtiny84 datasheet)
  //TCCR0B = _BV(CS01);  // prescaler=8
  TCCR0B = _BV(CS01) | _BV(CS00);  // nprscaler=64
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
```

Similarly, you can disable Timer0 temporarily at any time in your code using the following command:<p>
TIMSK0 &= ~(1 << OCIE0A);  //disable CTC mode for Timer 0<p>
and then re-enable it using this command:<p>
TIMSK0 |= (1 << OCIE0A);   //re-enable CTC mode for Timer 0<p>

This could be important if other routines in your code need to use Timer1. Also, if you'd like to access (or change) the value of the counter inside the ISR, TCNT0 (Timer/Counter 0 register) is the register to use. So for instance, your sketch can reset this just by using:<p>
TCNT0=0;<p>
or it can use it to measure time inside the ISR since the ISR reset, by doing something like:<p>
if(TCNT0==10)digitalWrite(pin,HIGH);<p>

Note that for both sketches, even though timers were being used, the SoftwareSerial connection still worked, and reported data. Yay!
