/* Code to report the remaining battery life for the ATtiny84
 * Author: D. Dubins
 * Date: 5-May-21
 * This sketch assumes the ATtiny84 is powered using a 3.2V CR2032 battery.
 * From: https://forum.arduino.cc/index.php?topic=416934.0
 * And:  https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
 * 
 * The following are the ATtiny84 pins by function:
 * ------------------------------------------------
 * Pin 1: Vcc (1.8-5.5V)
 * Pin 2: 10/XTAL1/PCINT8/PB0
 * Pin 3: 9/XTAL2/PCINT9/PB1
 * Pin 4: dW/RESET/PCINT11/PB3
 * Pin 5: PWM/OC0A/7/A7/PCINT7/PA7
 * Pin 6: PWM/ICP/OC0B/7/A7/ADC7/PCINT7/PA7
 * Pin 7: PWM/MOSI/SDA/OC1A/6/A6/ADC6/PCINT6/PA6
 * 
 * Pin 8: PWM/D0/OC1B/MISO/5/A5/ADC5/PCINT5/PA5
 * Pin 9: T1/SCL/SCK/4/A4/ADC4/PCINT4/PA4
 * Pin 10: 3/A3/ADC3/PCINT3/PA3
 * Pin 11: 2/A2/ADC2/PCINT2/PA2
 * Pin 12: 1/A1/ADC1/PCINT1/PA1
 * Pin 13: AREF/0/A0/ADC0/PCINT0/PA0
 * Pin 14: GND
 */
 
#define HIGHBATT 3200  // 3200 mV for "full" battery
#define LOWBATT 2000  // 2000 mV for "empty" battery

// Serial monitor if needed:
#include <SoftwareSerial.h>
#define rxPin 7   // PA7 is PP 6
#define txPin 6   // PA6 is PP 7
SoftwareSerial mySerial(rxPin, txPin);

//define the classic bit functions:
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))

void setup(){
  mySerial.begin(9600);                       // start the serial monitor
}

void loop(){
  int slope=(HIGHBATT-LOWBATT)/100;
  int intercept=0-LOWBATT/slope;
  int battLife=constrain((readVcc()/slope)+intercept,0,100); // calculate remaining battery life (in percent)
  mySerial.println(battLife);                 // print remaining battery life in percent to serial monitor
  delay(1000);
}

long readVcc(){                               // back-calculates voltage (in mV) applied to Vcc of ATtiny84
  //sbi(ADCSRA,ADEN);                           // enable ADC (comment out if already on)
  delay(50);                                  // wait for ADC to warm up 
  byte ADMUX_P = ADMUX;                       // store present values of these two registers
  byte ADCSRA_P = ADCSRA;
  ADMUX = _BV(MUX5)|_BV(MUX0);                // set Vbg to positive input of analog comparator (bandgap reference voltage=1.1V). Table 16.4 ATtiny24A/44A/84A Datasheet, p151
  delay(2);                                   // Wait for Vref to settle
  sbi(ADCSRA,ADSC);                           // Single conversion or free-running mode: write this bit to one to start a conversion.
  loop_until_bit_is_clear(ADCSRA,ADSC);       // ADSC will read as one as long as a conversion is in progress. When the conversion is complete, it returns a zero. This waits until the ADSC conversion is done.
  uint8_t low  = ADCL;                        // read ADCL first (17.13.3.1 ATtiny85 datasheet)
  uint8_t high = ADCH;                        // read ADCL second (17.13.3.1 ATtiny85 datasheet)
  long result = (high<<8) | low;              // combine low and high bits into one reading
  result = 1125300L / result;                 // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  ADMUX=ADMUX_P;                              // restore original values of these two registers
  ADCSRA=ADCSRA_P;
  //cbi(ADCSRA,ADEN);                           // disable ADC to save power (comment out to leave ADC on)
  delay(2);                                   // wait a bit
  return result;                              // Vcc in millivolts
}
