# Fast PWM on the ATtiny84

I stumbled on the ATtiny84 by chance as a happy medium between the ATtiny85 and the ATmega328. It has many more digital pins than the ATtiny85, without the big real estate needed by the DIP version of the 328. I know, life would be a lot easier if I just went surface mount, but damn it, I'm not ready! I just like PDIP, ok? :) My printed circuits look great - provided you just stepped out of the 70s.

In any event, one of the more important tasks I have needed with many of my projects is more (or any) control over PWM frequencies. There exists a point in many of my projects where I want to calibrate something, or send a specific signal to a device, and customizing the PWM frequency ends up being the answer. Nothing I've been doing so far requires considering phase inversion, or wave symmetry - these are modes I don't tend to explore. The day that I need them, I'm sure I will start caring. But until then, there's this beautiful little problem I've solved for two MCUs so far:

https://playground.arduino.cc/Code/FastPWM/

https://github.com/dndubins/ATtiny85/tree/main/CustomPWM

Now on to the third!

My goal for this exercise was to generate the same one-stop set of instructions for the ATtiny84. Here goes!!!

The ATtiny84, like the ATtyiny85, has two timers: Timer 0 (responsible for the delay() and millis() functions), and Timer 1. There are only 4 pins on the ATtiny84 that are PWM-capable. PB2 and PA7 (physical pins 5 and 6) are controlled by Timer 0. Pins PA6 and PA5 (physical pins 7 and 8) are controlled by Timer 1. 

Just like my Arduino Playground article, I will organize this post by what you would like to do.


Timer 0 (controls Pins PB2 and PA7)
-----------------------------------
### "I want to output a specific frequency on Pin PB2 only, using Timer 0."

As I said in my notes for the ATmega328 and the ATtiny85, before you do this, know that messing with Timer 0 will screw with your delay() and millis() functions, which you can account for, but it's another headache.

Some structural details: OCR0A and OCR0B are the compare registers for Timer 0, and like the ATtiny85, they are 8 bits long, so they can hold a number from 0-255 and that's it.
The registers that control Timer 0 modes are still called TCCR0A and TCCR0B, so ATtiny85 fanatics will be happy here. And the nicest part ever is that they have the same structure and bit names, too! For all my example sketches below, uncomment the prescaler line you would like to use.

The code for Pin PB2 only (physical pin 5), using Timer 0:
```
  // Custom PWM on Pin PB2 (pin 8) only, using Timer 0: (Page87 on ATtiny84 full datasheet)
  // duty cycle fixed at 50% in this mode.
  //Formula: wave frequency=fclk/((OCR0A+1)*2*N)
  pinMode(8, OUTPUT); // output pin for OCR0A is PB2 (physical pin 5).
  TCCR0A = _BV(COM0A0) |_BV(WGM01) | _BV(WGM00); // Fast PWM mode, set set OC0A on compare match
  //TCCR0B = _BV(WGM02) | _BV(CS00);  // no prescaling
  //TCCR0B = _BV(WGM02)  | _BV(CS01);  // prescaler=8
  //TCCR0B = _BV(WGM02) | _BV(CS01) | _BV(CS00);  // nprscaler=64
  //TCCR0B = _BV(WGM02) | _BV(CS02);  // prescaler=256
  TCCR0B = _BV(WGM02) | _BV(CS02) | _BV(CS00);  // prescaler=1024
  OCR0A = 0; // counter limit: 255, duty cycle fixed at 50% in this mode. OCR0A=0 with no prescaler gives freq=4MHz.
```
There is only one counter in this mode: OCR0A. You are stuck at a 50% duty cycle, but you can set the frequency based on the prescaler value, and the value of OCR0A. The formula, as indicated in the code, is: frequency=fclk/((OCR0A+1)*2*N). This is very similar to the ATtiny85 method I posted.

Here is a chart of frequencies (in Hz) spanning your options, assuming an 8MHz clock speed: (It's the same as the first frequency table I posted for the ATtiny85)

| OCR0A | Prescaler: 1 | 8 | 64 | 256 | 1024 |
| --- | --- | --- | --- | --- | --- |
| 0 | 4000000 | 500000 | 62500 | 15625 | 3906 |
| 1 | 2000000 | 250000 | 31250 | 7813 | 1953 |
| 3 | 1000000 | 125000 | 15625 | 3906 | 977 |
| 10 | 363636 | 45455 | 5682 | 1420 | 355 |
| 20 | 190476 | 23810 | 2976 | 744 | 186 |
| 40 | 97561 | 12195 | 1524 | 381 | 95 |
| 60 | 65574 | 8197 | 1025 | 256 | 64 |
| 80 | 49383 | 6173 | 772 | 193 | 48 |
| 100 | 39604 | 4950 | 619 | 155 | 39 |
| 120 | 33058 | 4132 | 517 | 129 | 32 |
| 140 | 28369 | 3546 | 443 | 111 | 28 |
| 160 | 24845 | 3106 | 388 | 97 | 24 |
| 180 | 22099 | 2762 | 345 | 86 | 22 |
| 200 | 19900 | 2488 | 311 | 78 | 19 |
| 255 | 15625 | 1953 | 244 | 61 | 15 |

### "I want to output a specific frequency on Pin PA7 only (physical pin 6), using Timer 0."

Now you have options. Using OCR0A, you can control the frequency using the formula: frequency=fclk/((OCR0A+1)*N). You can control the duty cycle of the signal using OCR0B, using the formula: duty cycle=OCR0B/OCR0A. You can also set the prescaler values as above.

```
  // Custom PWM on Pin PA7 (physical pin 6) only, using Timer 0: (Page87 on ATtiny84 full datasheet)
   //Formula: wave frequency=fclk/((OCR0A+1)*N)
  pinMode(7, OUTPUT); // output pin for OCR0B is PA7 (physical pin 6)
  TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00); // set OC0A on compare match
  TCCR0B = _BV(WGM02) | _BV(CS00);  // no prescaling
  //TCCR0B = _BV(WGM02)  | _BV(CS01);  // prescaler=8
  //TCCR0B = _BV(WGM02) | _BV(CS01) | _BV(CS00);  // nprscaler=64
  //TCCR0B = _BV(WGM02) | _BV(CS02);  // prescaler=256
  TCCR0B = _BV(WGM02) | _BV(CS02) | _BV(CS00);  // prescaler=1024
  OCR0A = 127; // counter limit: 255
  OCR0B = 119; //duty cycle=OCR0B/OCR0A. OCR0B can't be greater than OCR0A. (OCR0B=0.5*OCR0A for 50% duty cycle)
```
This gives rise to the following frequency table (in Hz) assuming an 8MHz clock speed (again, the same table for the ATtiny85):

| OCR0A | 	Prescaler: 1 | 	8 | 	64 | 	256 | 	1024 | 
| --- | --- | --- | --- | --- | --- |
| 1 | 	4000000 | 	500000 | 	62500 | 	15625 | 	3906 | 
| 5 | 	1333333 | 	166667 | 	20833 | 	5208 | 	1302 | 
| 10 | 	727273 | 	90909 | 	11364 | 	2841 | 	710 | 
| 20 | 	380952 | 	47619 | 	5952 | 	1488 | 	372 | 
| 40 | 	195122 | 	24390 | 	3049 | 	762 | 	191 | 
| 60 | 	131148 | 	16393 | 	2049 | 	512 | 	128 | 
| 80 | 	98765 | 	12346 | 	1543 | 	386 | 	96 | 
| 100 | 	79208 | 	9901 | 	1238 | 	309 | 	77 | 
| 120 | 	66116 | 	8264 | 	1033 | 	258 | 	65 | 
| 140 | 	56738 | 	7092 | 	887 | 	222 | 	55 | 
| 160 | 	49689 | 	6211 | 	776 | 	194 | 	49 | 
| 180 | 	44199 | 	5525 | 	691 | 	173 | 	43 | 
| 200 | 	39801 | 	4975 | 	622 | 	155 | 	39 | 
| 255 | 	31250 | 	3906 | 	488 | 	122 | 	31 | 

### "I want a custom PWM signal on Pins PB2 and PA7 at the same time, using Timer 0."

Well, you can do this, but your options are a bit more limited. The frequency is set using the prescaler value only, calculated using the formula: frequency=fclk/(N*256). OCR0A and OCR0B are used to control the duty cycles of PB2 and PA7, respectively, using the formula: duty cycle=OCR0X/255. Here is the code:
```
  // Custom PWM on Pin PB2 and PA7 together, using Timer 0: (Page84 on ATtiny85 full datasheet)
  //Formula: wave frequency= fclk /(N x 510)
  pinMode(7, OUTPUT); // output pin for OCR0B is PA7 (physical pin 6)
  pinMode(8, OUTPUT); // output pin for OCR0A is PB2 (physical pin 5).
  TCCR0A = _BV(COM0A1) | _BV(COM0A0) | _BV(COM0B1) |_BV(COM0B0) |_BV(WGM01) |_BV(WGM00); // PWM (Mode 3)
  TCCR0B = _BV(CS00);  // no prescaling
  //TCCR0B = _BV(CS01);  // prescaler=8
  //TCCR0B = _BV(CS01) | _BV(CS00);  // prescaler=64
  //TCCR0B = _BV(CS02);  // prescaler=256
  //TCCR0B = _BV(CS02) | _BV(CS00);  // prescaler=1024
  OCR0A = 10; // counter limit: 255 (duty cycle PB2 =(255-OCR0A)/255, 50% duty cycle=127)
  OCR0B = 250; // counter limit: 255 (duty cycle PA7 =(255-OCR0B)/255, 50% duty cycle=127)
```
Here are the frequencies you can attain (in Hz) using an 8 MHz clock speed:

| Prescaler: | 	1 | 	8 | 	64 | 	256 | 	1024 | 
| --- | --- | --- | --- | --- | --- |
| Frequency (Hz): | 	31250 | 	3906 | 	488 | 	122 | 	31 | 


Timer 1 (Controls PA6 and PA5)
------------------------------------------
Timer 1 is a bit different on the ATtiny84. It's a 16-bit timer, with lots of bells and whisles. For a complete list, check out Table 12-5 in the ATtiny84 data sheet. 16 modes! I'm not going to even pretend I understand the intricacies of this timer. I just want to emerge here with the ability to set custom frequencies on PA6 and PA5, and vary the duty cycle. To make matters more confusing, Atmel renamed some of the I/O register locations and control bits. It is what it is. My favourite mode is fast PWM. Let's skip to the bottom of Table 12-5 to mode 14, Fast PWM with ICR1 at the top. I liked this mode because it worked for me. Here goes:

### "I want a custom PWM signal on PA6 only, using Timer 1."

Timer 1 is controlled by the TCCR1A and TCCR1B Timer/Counter1 Control Registers.

To set the frequency in this mode, the following equation is used: frequency=fclk/((OCR1C+1)*N). OCR1A is used to set the duty cycle. This equation will hold for all of the following examples. This makes life slightly less confusing! Here is the code for PA6 only, using Timer 1:

```
// Custom PWM on Pin PA6 only, using Timer 1: 
//Formula: frequency=fclk/((ICR1+1)*N)
 pinMode(6, OUTPUT); // output is PA6 (physical pin 7)
TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(WGM11); // fast PWM with ICR1 at the top
TCCR1B = _BV(WGM13) | _BV(WGM12); 
//TCCR1B |= _BV(CS10);  // prescaler=1
//TCCR1B |= _BV(CS11); // prescaler=8
TCCR1B |= _BV(CS11) |  _BV(CS10); // prescaler=64
//TCCR1B |= _BV(CS12); // prescaler=256
//TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
ICR1=100;  //enter a value from 0-32,767 to set the frequency
OCR1A=50;  //duty cycle = (ICR1-OCR1A)/ICR1
```
In this mode (mode 14), ICR1 is used to set the frequency of the PWM signal, and OCR1A is used to set the duty cycle, using the formula: duty cycle=(ICR1-OCR1A)/ICR1). OCR1A should be less than ICR1. There's a LOT of latitude here, ICR1 and OCR1A are 16-bit numbers, meaning that you can enter values from 0-32,767 for each of them! 

This gives rise to the following awesome table, assuming an 8MHz clock speed:

|ICR1 | 	1 | 	8 | 	64 | 	256 | 	1024 | 
| --- | --- | --- | --- | --- | --- |
|1 | 	4000000 | 	500000 | 	62500 | 	15625 | 	3906 | 
|2 | 	2666667 | 	333333 | 	41667 | 	10417 | 	2604 | 
|5 | 	1333333 | 	166667 | 	20833 | 	5208 | 	1302 | 
|7 | 	1000000 | 	125000 | 	15625 | 	3906 | 	977 | 
|10 | 	727273 | 	90909 | 	11364 | 	2841 | 	710 | 
|20 | 	380952 | 	47619 | 	5952 | 	1488 | 	372 | 
|50 | 	156863 | 	19608 | 	2451 | 	613 | 	153 | 
|70 | 	112676 | 	14085 | 	1761 | 	440 | 	110 | 
|100 | 	79208 | 	9901 | 	1238 | 	309 | 	77 | 
|200 | 	39801 | 	4975 | 	622 | 	155 | 	39 | 
|500 | 	15968 | 	1996 | 	250 | 	62 | 	16 | 
|700 | 	11412 | 	1427 | 	178 | 	45 | 	11 | 
|1000 | 	7992 | 	999 | 	125 | 	31 | 	8 | 
|2000 | 	3998 | 	500 | 	62 | 	16 | 	4 | 
|5000 | 	1600 | 	200 | 	25 | 	6 | 	2 | 
|7000 | 	1143 | 	143 | 	18 | 	4 | 	1 | 
|10000 | 	800 | 	100 | 	12 | 	3 | 	1 | 
|20000 | 	400 | 	50 | 	6 | 	2 | 	0 | 
|32767 | 	244 | 	31 | 	4 | 	1 | 	0 | 

A small application note is that according to the datasheet, you have to take care in sending 16-bit numbers to the registers. When writing to the registers, it does it in two goes, first the high-bit and then the low-bit. I am using the David Mellis version of the Attiny board manager, and his library takes care of it all in one line, so when I write (for example) ICR1=32000; it just works. However, if you are using a different build, consider heeding these instructions if you get wonky behaviour. I would suggest the following (doing the same for OCR1AH and OCR1AL):

```
int ICR1n=32000;
ICR1H=ICR1n>>8; //move high byte to ICR1H
ICR1L=ICR1n&&0xff; //move low byte to ICR1L
```
You may or may not need to do this. Probably not, but it's just worth noting.


### "I want a custom PWM signal on PA5 only, using Timer 1."

For PA5, you need only set channels COM1B1 and COM1B0 high instead, and set the duty cycle with OCR1B:

```
// Custom PWM on Pin PA5 only, using Timer 1: 
pinMode(5, OUTPUT); // output is PA5 (physical pin 8)
//Formula: frequency=fclk/((ICR1+1)*N)
TCCR1A = _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11); // fast PWM with ICR1 at the top
TCCR1B = _BV(WGM13) | _BV(WGM12); 
//TCCR1B |= _BV(CS10);  // prescaler=1
//TCCR1B |= _BV(CS11); // prescaler=8
TCCR1B |= _BV(CS11) |  _BV(CS10); // prescaler=64
//TCCR1B |= _BV(CS12); // prescaler=256
//TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
ICR1=100;  //enter a value from 0-32,767 to set the frequency
OCR1B=50;  //duty cycle = (ICR1-OCR1B)/ICR1
```
The frequency is set again by ICR1 according to the formula: frequency=fclk/((ICR1+1)*N), and the duty cycle again is just inverted (ICR1-OCR1B)/ICR1. The same frequency table above applies, as it is the same method.

### "I want a custom PWM signal on both pins PA6 and PA5, using Timer 1."

Similarly, you just need to set all of the COM1XX bits high in TCCR1A to get a custom PWM signal on both pins, and control their duty cycles independently with OCR1A and OCR1B:

```
// Custom PWM on both pins PA6 and PA5, using Timer 1: 
 pinMode(6, OUTPUT); // output is PA6 (physical pin 7)
 pinMode(5, OUTPUT); // output is PA5 (physical pin 8)
//Formula: frequency=fclk/((ICR1+1)*N)
TCCR1A = _BV(COM1A1) | _BV(COM1A0) | _BV(COM1B1) | _BV(COM1B0) | _BV(WGM11); // fast PWM with ICR1 at the top
TCCR1B = _BV(WGM13) | _BV(WGM12); 
//TCCR1B |= _BV(CS10);  // prescaler=1
//TCCR1B |= _BV(CS11); // prescaler=8
TCCR1B |= _BV(CS11) |  _BV(CS10); // prescaler=64
//TCCR1B |= _BV(CS12); // prescaler=256
//TCCR1B |= _BV(CS12) |  _BV(CS10); // prescaler=1024
ICR1=100;  //enter a value from 0-32,767 to set the frequency
OCR1A=25;  //duty cycle PA6 = (ICR1-OCR1B)/ICR1 (this example is a 75% duty cycle at 1238 Hz)
OCR1B=75;  //duty cycle PA5 = (ICR1-OCR1B)/ICR1 (this example is a 25% duty cycle at 1238 Hz)
```

Well, that wasn't too bad. I will hunt through and hopefully find typos above that I will catch over time. If you find one, let me know!! Special thanks to the people who posted sketches at the following links, which guided me in the right direction:

Andreas Rochner wrote this AMAZING tutorial, written in 2015:
https://andreasrohner.at/posts/Electronics/How-to-set-the-PWM-frequency-for-the-Attiny84/

This very useful post saved me hours:
https://stackoverflow.com/questions/59160802/how-do-i-output-a-compare-match-to-oc1b-in-fast-pwm-mode-on-the-attiny84

And also (of course) special thanks go to the attiny84 datasheet.
