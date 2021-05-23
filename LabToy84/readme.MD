# ATtiny84 Lab Toy

This fun project was my attempt at making a minimalist timer/stopwatch for collecting data points in my pharmaceutics labs. Many labs (e.g. USP dissolution) require the timed collection of samples. Every year, small battery-powered timers are somewhat of a consumable, both in terms of the batteries, and the timers themselves.

What I really wanted was a small, energy efficient timer/stopwatch that was easy to program and use. Then the upgrades started. I added:
 - a clock mode, to keep track of the time
 - an alarm mode, to beep at a specific time of day (with silence feature)
 - a stopwatch mode
 - a temperature mode, by calibrating the onboard core temperature sensor on the ATtiny84

### Circuit Diagram
<img src="https://github.com/dndubins/ATtiny84/blob/main/LabToy84/pics/LabToy84_circuitdiag.png">

Here is a list of components for this project:

### PCB Label - Component Description
- U1: ATtiny84V-10PU
- C1: 100 uF capacitor (electrolytic)
- X1: CRYSTAL 8.0000MHZ 18PF TH
- C2: 22pf 50V ceramic capacitor
- C3: 22pf 50V ceramic capacitor
- SP: Piezo active buzzer 3V - Mallory Sonalert PB-12N23P-03Q
- S1: momentary switch (generic 4 post) - Panasonic EVQ-PBC04M
- S2: momentary switch (generic 4 post) - Panasonic EVQ-PBC04M
- D1: 1N4001 Diode
- BT1: MPD BS-7 coincell holder (CR2032)
- SW1: SPDT Top Slide 6A 250VAC 28VDC 0.4VA PC Pins Thru-Hole Bulk
- microUSB jack (4 post anchor)
- 1 x 0.36" TM1637 7-Segment 4-digit White Digital Tube LED Display Module for Arduino
- 4 x PCB Support Flat Spacer Plastic Rivet Nylon Natural Ø4mm
- 1 x CR2032 battery (optional)

### Power Consumption

An interesting and important question for any battery-powered device is "how long will it last on fresh batteries"? This design was a little bit of a fail, in that I was hoping to run the ATtiny84 chip continously using a CR2032 3.2V battery. I ran several experiments using the finished board, and found that the current draw varied just about linearly with LED brightness setting, when powered using the 3.3V pin from an arduino Uno:

<img src="https://github.com/dndubins/ATtiny84/blob/main/LabToy84/pics/CurrentDraw.png">

Here is a graph of the current draw:
<img src="https://github.com/dndubins/ATtiny84/blob/main/LabToy84/pics/CurrentDraw.png">

On the x-axis, -1 is the ATtiny84 powered up with the TM1637 module off, and -2 is the whole system in sleep mode. The TM1637 is a remarkably energy efficient device. When you subtract off the MCU, it requires between 0.67 mA (brightness=0) and 2.91 mA (brightness=7). Unfortunately, it's the microprocessor that kills us here. While not in sleep mode, the ATtiny84 uses about 2.25 mA in this circut. So even with the TM1637 off, we can do the math of how long a CR2032 battery should run this device. Although estimates range for the current capacity of a CR2032 battery, let's take a value of 210 mAh, published by Farnell: http://www.farnell.com/datasheets/1496885.pdf

So then, the BEST case scenario for this device is to run for 210 mAh / 2.25 mA = 93.3 h, = 3.8 days. NOT GOOD! This means that we would need to change batteries more than once a week!!!!

This is why I incorporated a USB port on this device. While plugged in, the clock can run 24/7 with no worries of operating time. When switched to battery power, it is advisable to turn the clock off using the MODE button, and set the LED brightness to "OFF". This means that the device will only operate in temperature, timer, and stopwatch mode, will go to sleep in between uses, and will shut off the LED when not in use. In sleep mode, the device only registers using 0.17 microAmps, which would be virtually equivalent to the background drain rate of the battery (similar to turning it off).





