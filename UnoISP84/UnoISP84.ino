/* 
UnoISP84.ino Original Sketch
Author: D. Dubins
Date: 14-Oct-21
Links: https://www.instructables.com/How-to-Program-an-Attiny85-From-an-Arduino-Uno/
Description: This program uploads a blink sketch to an ATtiny84 chip, using the Arduino Uno R3 as a programmer.

Arduino Uno to ATtiny84:
-----------------------
+5V -- physical Pin 1 (Vcc)
Pin 13 -- physical Pin 9 (SCK)
Pin 12 -- physical Pin 8 (MISO)
Pin 11 -- physical Pin 7 (MOSI)
Pin 10 -- physical Pin 4 (RST)
GND -- physical Pin 14 (GND)

Optional: On the Uno, wire a 10uF capacitor between GND and RES (prevents Uno from resetting). 
The capacitor isn't needed for this sketch to work.

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

Wire an LED to physical pin 3 (PB1, digital pin 9) of ATtiny84:
ATtiny84 Physical Pin 3 -- LED -- 1K resistor -- GND

To set up the Arduino IDE to support ATtiny MCUs: (only need to do this once)
-------------------------------------------------
File --> Preferences
Under "Additional Boards Manager URLs" enter the following web address:
https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json

Click: Tools --> Board:"Arduino/Genuino Uno" --> Boards Manager
-Scroll down to attiny by David A. Mellis
-Select it, then click the "Install" button
-ATtiny should now appear on the Tools-->Board dropdown menu

To upload a sketch:
-------------------
Prepare the Uno as a programmer by uploading the example sketch ArduinoISP.ino to the Uno:
File --> Examples --> ArduinoISP

If you haven't burned a bootloader to the ATtiny84 (likely)
Tools --> Select Board --> ATtiny Microcontrollers --> ATtiny24/44/84
Tools --> Processor --> ATtiny84
Tools --> Clock --> Internal 8 MHz
Tools --> Port --> select the port for the Uno
Tools --> Programmer --> Arduino as ISP
Tools --> Burn Bootloader

If burning is successful, you are ready to upload this simple sketch to the ATtiny84 (the usual way), using the Upload button.
*/

#define LEDpin 9             // use digital pin 9 to light up LED (= physical pin 3 on the ATtiny84).

void setup(){
  pinMode(LEDpin,OUTPUT);    // set LEDpin to OUTPUT mode
}
 
void loop(){
  digitalWrite(LEDpin,HIGH); // flash LED wired to pin
  delay(250);
  digitalWrite(LEDpin, LOW);
  delay(250);
}
