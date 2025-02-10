/* ATtiny84_I2C_master.ino
   ATtiny84 as an I2C Master, programmed for use with Uno_I2C_slave.ino.
   Author: David Dubins
   Date: 08-Feb-25
   Written to work with TinyWireM.h (by Adafruit, 1.1.3)
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

#include "TinyWireM.h"                  // I2C Master Library (by Adafruit, 1.1.3)
char message[] = "Test message to send.";
int myInt=0;

#define I2C_ADDR1 0x08                  // I2C address (0x08)
byte LEDpin=3;                          // PB3 is physical pin 10
byte Vpin=A2;                           // to take a reading on A2 (physical pin 11)

void setup(){
  pinMode(LEDpin,OUTPUT);               // just for visual feedback
  flashLED(2);                          // two flashes on powerup
  TinyWireM.begin();                    // Start TinyWireM
}

void loop() {
  // Send information to slave
  int reading=analogRead(Vpin);
  sendInt(reading);
  //sendString("This is a long string.");
  //sendArr("This is a test."); // send random text
  //sendArr(message); // send the array stored in message
  //sendChar('h'); // send the letter 'h'
  //sendFloat(3.141,2); // send float number with 2 decimal places
  delay(500); // wait a bit between sending and receiving

  // Ask for information from slave
  TinyWireM.requestFrom(I2C_ADDR1,1); // Request 1 byte from slave
  myInt = TinyWireM.receive();          // get the number of flashes
  flashLED(myInt); // show you sent something
  delay(500); // wait a bit between sending and receiving
}

void sendArr(char* arr){
  int i=0;
  do{
      TinyWireM.beginTransmission(I2C_ADDR1); // Start the transmission
      TinyWireM.send(arr[i++]);
      TinyWireM.endTransmission();         // end the transmission
  }while(arr[i]!='\0');
  TinyWireM.beginTransmission(I2C_ADDR1);  // Start the transmission
  TinyWireM.send('\0');                    // send terminal character
  TinyWireM.endTransmission();             // end the transmission
}

void sendString(String str){
  int n=str.length();
  for(int i=0;i<n+1;i++){
      TinyWireM.beginTransmission(I2C_ADDR1); // Start the transmission
      TinyWireM.send(str[i]);
      TinyWireM.endTransmission();           // end the transmission
  }
}

void sendChar(char c){
  TinyWireM.beginTransmission(I2C_ADDR1); // Start the transmission
  TinyWireM.send(c);
  TinyWireM.endTransmission();           // end the transmission
}

void sendByte(byte b){
  TinyWireM.beginTransmission(I2C_ADDR1); // Start the transmission
  TinyWireM.send(b);
  TinyWireM.endTransmission();           // end the transmission
}

void sendFloat(float f, byte dec){     // float number, number of decimals
  byte n=sizeof(f);
  char B[n];
  dtostrf(f,n,dec,B); // 3 is number of decimals to send
  TinyWireM.beginTransmission(I2C_ADDR1); // Start the transmission
  int i=0;
  do{
      TinyWireM.send(B[i++]);
  }while(B[i]!='\0');
  TinyWireM.send('\0'); // send terminal character
  TinyWireM.endTransmission();           // end the transmission
}

void sendInt(int j){      // integer to send
  byte n=(1+log10(j)); // count the number of digits in the integer
  char B[n+1];              // create a char array of length #digits+1
  itoa(j,B,10); // convert integer to char array. 10 is for base10 format
  TinyWireM.beginTransmission(I2C_ADDR1); // Start the transmission
  int i=0;
  do{
      TinyWireM.send(B[i]);
      i++;
  }while(B[i]!='\0');
  TinyWireM.send('\0'); // send terminal character
  TinyWireM.endTransmission();           // end the transmission
}

void flashLED(byte n){
  for (int i=0; i< n; i++){
    digitalWrite(LEDpin,HIGH);
    delay(100);
    digitalWrite(LEDpin,LOW);
    delay(100);
  }
}
