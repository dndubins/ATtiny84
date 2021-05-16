/* Core temperature reading for the ATtiny84 
 * Author: D. Dubins
 * Date: 15-May-21
 * 
 * Adapted from: https://github.com/mharizanov/TinySensor/blob/master/TinySensor_InternalTemperatureSensor/TinySensor_InternalTemperatureSensor.ino
 * (Martin Harizanov)
 *  
 * Measures and reports the core temperature.
 * 
 * The following are the ATtiny84 pins by function:
 * ------------------------------------------------
 * Pin 1: Vcc (1.8-5.5V)
 * Pin 2: 10/XTAL1/PCINT8/PB0
 * Pin 3: 9/XTAL2/PCINT9/PB1
 * Pin 4: dW/RESET/PCINT11/PB3
 * Pin 5: PWM/OC0A/CKOUT/8/INT0/PCINT10/PB2
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
 *
 */

//define the classic bit functions:
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))
#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))
#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))
  
// Serial monitor if needed:
#include <SoftwareSerial.h>
#define rxPin 4   // PA4 is PP 9
#define txPin 5   // PA5 is PP 8
SoftwareSerial mySerial(rxPin, txPin);

void setup() {
  mySerial.begin(9600);                  // Start the Serial Monitor
}

void loop() {
  float r1=readCoreTemp(100);
  mySerial.println(r1);
  delay(250);
}

float readCoreTemp(int n){                    // Calculates and reports the chip temperature of ATtiny84
  // Tempearture Calibration Data
  float kVal=0.8929;                          // k-value fixed-slope coefficient (default: 1.0). Adjust for manual 2-point calibration.
  float Tos=-244.5+12.5;                      // temperature offset (default: 0.0). Adjust for manual calibration. Second number is the fudge factor.

  //sbi(ADCSRA,ADEN);                         // enable ADC (comment out if already on)
  delay(50);                                  // wait for ADC to warm up 
  byte ADMUX_P = ADMUX;                       // store present values of these two registers
  byte ADCSRA_P = ADCSRA;
  ADMUX = B00100010;                          // Page 149 of ATtiny84 datasheet - enable temperature sensor
  cbi(ADMUX,ADLAR);                           // Right-adjust result
  sbi(ADMUX,REFS1);
  cbi(ADMUX,REFS0);                           // set internal ref to 1.1V
  delay(2);                                   // wait for Vref to settle
  cbi(ADCSRA,ADATE);                          // disable autotrigger
  cbi(ADCSRA,ADIE);                           // disable interrupt
  float avg=0.0;                              // to calculate mean of n readings
  for(int i=0;i<n;i++){  
    sbi(ADCSRA,ADSC);                         // single conversion or free-running mode: write this bit to one to start a conversion.
    loop_until_bit_is_clear(ADCSRA,ADSC);     // ADSC will read as one as long as a conversion is in progress. When the conversion is complete, it returns a zero. This waits until the ADSC conversion is done.
    uint8_t low  = ADCL;                      // read ADCL first (17.13.3.1 ATtiny85 datasheet)
    uint8_t high = ADCH;                      // read ADCL second (17.13.3.1 ATtiny85 datasheet)
    long Tkelv=kVal*((high<<8)|low)+Tos;      // remperature formula, p149 of datasheet
    avg+=(Tkelv-avg)/(i+1);                   // calculate iterative mean
  }
  ADMUX=ADMUX_P;                              // restore original values of these two registers
  ADCSRA=ADCSRA_P;
  //cbi(ADCSRA,ADEN);                         // disable ADC to save power (comment out to leave ADC on)
  
  // According to Table 16-2 in the ATtiny84 datasheet, the function to change the ADC reading to 
  // temperature is linear. A best-fit line for the data in Table 16-2 yields the following equation:
  //
  //  Temperature (degC) = 0.8929 * ADC - 244.5
  //
  // These coefficients can be replaced by performing a 2-point calibration, and fitting a straight line
  // to solve for kVal and Tos. These are used to convert the ADC reading to degrees Celsius (or another temperature unit).
  
  return avg;                  // return temperature in degC
}
