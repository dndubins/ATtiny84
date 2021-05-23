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

An interesting and important question for any battery-powered device is "how long will it last on fresh batteries"? This design was a little bit of a fail, in that I was hoping to run the ATtiny84 chip continously using a CR2032 3.2V battery. I ran several experiments using the finished board, and found that the current draw varied linearly with LED brightness, when powered using the 3.3V pin from an arduino Uno:






