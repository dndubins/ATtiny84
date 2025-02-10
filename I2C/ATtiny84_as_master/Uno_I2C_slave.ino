/* Uno_I2C_slave.ino
   Uno as an I2C Slave, programmed for use with ATtiny85_I2C_master.ino.
   Author: David Dubins
   Date: 08-Feb-25
   Written to work with TinyWireM.h (by Adafruit, 1.1.3)
   Adapted from: https://pwbotics.wordpress.com/2021/05/05/programming-attiny85-and-i2c-communication-using-attiny85/
*/

#include <Wire.h>
#define I2C_ADDR 0x08    // I2C address of master (0x08)'

char arr[30]; // increase to hold size of transmitted data
int i=0;
int inum=0;   // to hold integer value read from I2C
float fnum=0.0; // to hold float number read from I2C

void setup() {
  Wire.begin(I2C_ADDR);          // Start as an I2C slave
  Wire.onReceive(receiveEvent);  // Register the event handler for data reception
  Wire.onRequest(requestEvent);  // Register the event handler for data requests
  Serial.begin(9600);            // Start the serial monitor
  Serial.println("Slave ready.");
}

void loop() { // Nothing really needs to be here. The calls are all interrupt driven.
  delay(50);  // small delay
}

// Event handler for receiving data from the master
void receiveEvent(){
  while(Wire.available()>0){
    char c=Wire.read();
    arr[i]=c;
    if(c=='\0'){
      i=0;
      Serial.print("Received from master: ");
      // Uncomment to print received char array:
      Serial.println(arr);
      // Uncomment for reading integer:
      //inum=atoi(arr);  // convert the array here as needed
      //Serial.println(inum); // print the converted value
      // Uncomment for reading float number:
      //fnum=atof(arr);  // convert the array here as needed
      //Serial.println(fnum); // print the converted value
    }else{
      i++;
    }
  }
}


void requestEvent() {    // this sends the number of flashes to the ATtiny85
// This could be re-written to send an error code or other useful info to the master.
  byte response = 3;     // Send 3 back to the master
  Wire.write(response);  // Send the response byte
  Serial.print("Sent to master: ");
  Serial.println(response, DEC);  // Print the sent byte as a number
}