I'm a bit more interested in using an ATtiny84 as an I2C slave device, because it's so small and has such potential.
For instance, it could be configured to read from a sensor and send the data back to the master via I2C.
For just sending and receiving small amounts of data of one type (e.g. an array, a float, a byte), the two programs that will be useful here are:<p>
Uno_I2C_master.ino and ATtiny84_I2C_slave.ino.<p>
However, what if you'd like to transfer more than one variable, and they are different types? To make life easier, you could pack all of the data you want to transfer into
a struct, and then create a union with a char array. Then, send the char array over I2C, and have it read in on the other end, to a char array that has a union with the same complicated structure.
Then, you can send many different variable types at once, all in one go.
Sounds interesting? Check out the following two programs:<p>
Uno_I2C_master-struct.ino and ATtiny84_I2C_slave-struct.ino.<p> The implementation was easier and faster than I thought. The master sends TXdata to the slave. The slave then reads it into RXdata, then sends it back to the master. The master receives the data in its own variable called RXdata, then prints it out to show the journey was successful. The return trip isn't necessary, but the code is there if you need it. Maybe just a "true" that it worked.
