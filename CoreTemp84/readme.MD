Interested in the onboard temperature sensor of the ATtiny84, I put this code together to calculate and report core temperature in degrees Celsius.

I adapted this from: 
https://github.com/mharizanov/TinySensor/blob/master/TinySensor_InternalTemperatureSensor/TinySensor_InternalTemperatureSensor.ino
(Martin Harizanov)

I was a bit confused by the ATtiny84 datasheet as I initially thought the formula given on Page 149 was in Celsius. However, closer inspection of Table 16.2 reveals that 25 degrees C will give a Vout of 300 LSB.
Since 300-273.15=26.85, this looks a lot like Kelvin, doesn't it?!?!?

