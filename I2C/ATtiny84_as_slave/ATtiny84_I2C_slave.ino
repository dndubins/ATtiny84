/* ATtiny84_I2C_slave.ino
   ATtiny84 as an I2C slave, programmed with Uno_I2C_master.ino
   Author: David Dubins
   Date: 08-Feb-25
   Written to work with TinyWireS.h available here: https://github.com/rambo/TinyWire
   Adapted from: https://pwbotics.wordpress.com/2021/05/05/programming-attiny85-and-i2c-communication-using-attiny85/

   The following are the ATtiny84 pins by function:
   -----------------------------------------------
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

   Wiring:
   -------
   ATtiny84 - Uno
   Pin 10 (PA3) - LED - 220R - GND
   Pin 14 - GND
   Pin 7 - SDA (use 10K pullup)
   Pin 9 - SCL (use 10K pullup)
   Pin 1 - 5V
 */

#include <TinyWireS.h>

#define I2C_ADDR 0x08  // ATtiny84 I2C Address
byte LEDpin=3;                          // PB3 ia physical pin 2 
byte Vpin=A2;                           // take a reading on A2 (physical pin 11)
byte myInt=0;                           // to store integer read from master

void setup() {
  pinMode(LEDpin,OUTPUT);       // set LEDpin to OUTPUT mode
  TinyWireS.begin(I2C_ADDR);
  TinyWireS.onReceive(receiveEvent);
  TinyWireS.onRequest(requestEvent);
  flashLED(3);
}

void loop() { // The slave will continuously wait for requests or data from the master.
  TinyWireS_stop_check();  // needs to be in the loop
  delay(50);               // small delay
}

// Function to handle data received from the master
void receiveEvent() {
  while (TinyWireS.available()) {
    myInt = TinyWireS.receive();  // Receive the byte from the master
  }
  flashLED(2);
}

// Function to send data to the master when requested
void requestEvent() {
  // Uncomment to send a reading:
  int reading=analogRead(Vpin);
  sendInt(reading);
  //sendArr("This is a test"); // send random text
  //sendChar('h'); // send the letter 'h'
  //sendInt(myInt);
  // Uncomment to send a float:
  //sendFloat(3.141,2);
}

void sendArr(char* arr){
  int i=0;
  do{
      TinyWireS.send(arr[i]);
      i++;
  }while(arr[i]!='\0');
  TinyWireS.send('\0'); // send terminal character
}

void sendChar(char c){
  TinyWireS.send(c);
  TinyWireS.send('\0'); // send terminal character
}

void sendByte(byte b){
  TinyWireS.send(b);
  TinyWireS.send('\0'); // send terminal character
}

void sendFloat(float f, byte dec){     // float number, number of decimals
  byte n=sizeof(f);
  char B[n];
  dtostrf(f,n,dec,B); // 3 is number of decimals to send
  int i=0;
  do{
      TinyWireS.send(B[i]);
      i++;
  }while(B[i]!='\0');
  TinyWireS.send('\0'); // send terminal character
}

void sendInt(int j){      // integer to send
  byte n=(1+log10(j)); // count the number of digits in the integer
  char B[n+1];              // create a char array of length #digits+1
  itoa(j,B,10); // convert integer to char array. 10 is for base10 format
  int i=0;
  do{
      TinyWireS.send(B[i]);
      i++;
  }while(B[i]!='\0');
  TinyWireS.send('\0'); // send terminal character
}

void flashLED(byte n){
    digitalWrite(LEDpin, LOW);
    for(byte i=0;i<n;i++){
        digitalWrite(LEDpin, HIGH);
        tws_delay(100);
        digitalWrite(LEDpin, LOW);
        tws_delay(100);
    }
}