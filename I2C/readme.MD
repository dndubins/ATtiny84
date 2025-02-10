# I2C Communications for ATtiny84

I found two really great libraries and example sketches from BroHogan at: https://pwbotics.wordpress.com/2021/05/05/programming-attiny85-and-i2c-communication-using-attiny85/
I wrote some sketches connecting an Arduino Uno (or Mega, or other Wire.h compatible device) to the ATtiny85 via I2C. Then I discovered the libraries worked for ATtiny84 as well. Here are versions tailored to the ATtiny84.
They are the same essentially, other than wiring.
BroHogan's original TinyWireM.h was not compatible with the ATtiny84. However, Adafruit's version 1.1.3 worked with the same code and supported ATtiny84.
Links to the libraries are referred to in the sketches.<br>

Here is the library for setting the ATtiny85 as a slave I2C device:<br>
https://github.com/rambo/TinyWire<br>
Here is the library for setting the ATtiny85 as a master I2C device:<br>
https://github.com/adafruit/TinyWireM<br>

My goal for this project really was to get ATtiny84 to be a slave device, and report a value perhaps from a sensor. Both approaches allow for 2-way communication.<p>

This explanation is from Perplexity.ai, regarding master and slave roles:

In I2C communication, the master and slave roles define which device controls the communication and which responds.<p>
Master: The master device initiates all communication on the I2C bus and provides the clock signal (SCL) for all slave devices. It addresses the slave device it wants to communicate with. The master controls when data is sent and received.<p>
Slave: The slave device listens for its unique address from the master. It only responds when addressed by the master. The slave doesn't initiate communication; it waits for the master's instructions.<p>
The master-slave relationship is essential for coordinating data transfer on the I2C bus. The master controls the communication, ensuring that only one device transmits data at a time, preventing conflicts.
