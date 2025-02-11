/* Uno_I2C_master.ino
   Uno as an I2C Master, with Attiny84 as slave. Programmed with ATtiny84_I2C_slave.ino.
   Author: David Dubins
   Date: 08-Feb-25
   Written to work with TinyWireS.h available here: https://github.com/rambo/TinyWire
   Adapted from: https://pwbotics.wordpress.com/2021/05/05/programming-attiny85-and-i2c-communication-using-attiny85/
*/

#include <Wire.h>         // start Wire.h as master (no address needed)
#define I2C_ADDR1 0x08    // I2C address of the ATtiny84 slave (0x08)
char arr[30]; // Buffer to hold the received data
int i = 0;    // Index for filling the buffer

void setup() {
  Wire.begin();  // Initialize I2C as master
  Serial.begin(9600);  // Start serial communication
  Serial.print("Ready to send/receive data."); // send welcome msg
}

void loop() {
  Wire.requestFrom(I2C_ADDR1,64);  // Request up to 64 bytes (adjust as needed)
  i=0;
  while(Wire.available()) {    
    char c = Wire.read();  // Read the next byte from the slave
    arr[i] = c;            // Store it in the buffer
    if (c == '\0') {       // If a null character is encountered, process the data
      arr[i] = '\0';       // Ensure null termination for the string
      // Print the received string
      Serial.print(F("Received from slave: "));
      Serial.println(arr);
      // Uncomment for integer conversion (if needed)
      // int inum = atoi(arr);
      // Serial.print("Integer value: ");
      // Serial.println(inum);

      // Uncomment for float conversion (if needed)
      // float fnum = atof(arr);
      // Serial.print("Float value: ");
      // Serial.println(fnum);
      i = 0;  // Reset index for the next message
    } else {
      i++;  // Move to the next position in the buffer
    }
  }
  while(Wire.available()>0){
    char c = Wire.read();
      Serial.print("Received from slave: ");
      Serial.println(c,DEC);
   }
  delay(500); // wait between receiving and sending
  // Now, send a response back to the slave (e.g., send the number of flashes)
  byte flashes = 4;  // Example value (you can change this or calculate based on received data)
  sendResponse(flashes);  // Send response to the slave
  delay(500);  // Small delay to avoid overloading the slave
}

void sendResponse(byte n) {
  Wire.beginTransmission(I2C_ADDR1);  // Start I2C transmission to slave
  Wire.write(n,1);  // Send the number of flashes (1 byte)
  Wire.endTransmission();  // End the transmission
  Serial.print("Sent to slave: ");
  Serial.println(n);
}
