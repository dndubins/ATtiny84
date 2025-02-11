/* Uno_I2C_master-struct.ino
   Uno as an I2C Master, with Attiny84 as slave. Programmed with ATtiny84_I2C_slave-struct.ino.
   Author: David Dubins
   Date: 10-Feb-25
   Written to work with TinyWireS.h available here: https://github.com/rambo/TinyWire
   Adapted from: https://pwbotics.wordpress.com/2021/05/05/programming-attiny85-and-i2c-communication-using-attiny85/
*/

#include <Wire.h>         // start Wire.h as master (no address needed)
#define I2C_ADDR1 0x08    // I2C address of the ATtiny84 slave (0x08)
bool rcv=false;           // flag for new received data

// Example of a structure to be sent over I2C (10 bytes total)
struct myStruct { // example structure to send over I2C. This was for a servo.
  float PVAL; // 4 bytes (just for fun)
  byte Pin;  // 1 byte (pin number)
  byte MIN;  // 1 byte (minimum angle)
  byte MAX; // 1 byte (max angle)
  byte HOME;  // 1 byte (home position)
  int POS;   // 2 bytes (current position)
};  

// Declare a union to help decode the structure once received or sent
union myUnion { //declare union in global space
  char myCharArr[10]; // char array to be shared with sData
  myStruct sData; //occupies same memory as myCharArr
}TXdata,RXdata; //create two union instances called TXdata (to transmit) and RXdata (to receive)

void setup() {
  Wire.begin();  // Initialize I2C as master
  Serial.begin(9600);  // Start serial communication
  Serial.print("Master ready."); // send welcome msg
  // initialize TXdata.sData with values
  TXdata.sData.PVAL=0.05; // significant data
  TXdata.sData.Pin=8;
  TXdata.sData.MIN=5;
  TXdata.sData.MAX=174;
  TXdata.sData.HOME=89;
  TXdata.sData.POS=95;    // keep track of current servo position
}

void loop() {
  // Send data to the slave:
  sendToSlave(TXdata.myCharArr);  // Send TXdata to slave
  delay(500); // wait between receiving and sending
  // Receive data from slave
  receiveFromSlave(RXdata.myCharArr);  // Receive RXdata from slave
  delay(500);  // Small delay to avoid overloading the slave
  if(rcv){ // if new data has been received:
    // Print the received struct
    Serial.println(F("Received from slave: "));
    Serial.println(RXdata.sData.PVAL);
    Serial.println(RXdata.sData.Pin);
    Serial.println(RXdata.sData.MIN);
    Serial.println(RXdata.sData.MAX);
    Serial.println(RXdata.sData.HOME);
    Serial.println(RXdata.sData.POS);
  }else{
    Serial.println("Data communications error.");
  }
  rcv=false; // reset received flag
}

void receiveFromSlave(char* a){
  // Use this if you want to receive the whole struct:
  Wire.requestFrom(I2C_ADDR1,sizeof(myStruct));  // Request of size of struct (10 bytes here)
  int i=0;
  while(Wire.available()) {    
    RXdata.myCharArr[i++] = Wire.read();  // Read the next byte from the slave
  }
  // Use this if you only want to receive a single response character from the slave:
  /*Wire.requestFrom(I2C_ADDR1,1);  // Request 1 byte
  while(Wire.available()) {    
    char c = Wire.read();  // Read byte from the slave
    Serial.print(F("Received from slave: "));
    Serial.println(c);  
  }*/
  if(i==sizeof(myStruct))rcv=true;  // new data has been received of the correct size
}

void sendToSlave(char* a) {
  Wire.beginTransmission(I2C_ADDR1);  // Start I2C transmission to slave
  Wire.write(a,1);  // Send the data as the char array myCharArr (1 byte long)
  Wire.endTransmission();  // End the transmission
  Serial.println("Data sent to slave.");
}
