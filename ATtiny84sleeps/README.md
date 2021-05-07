This file is the ATtiny84 version of the sleep sketch I wrote for the ATtiny85. I used to think that pin 0 was the only pin on the ATtiny84 capable of waking it from deep sleep. Not true! Have a look at these sweet routines.

Normally with a microprocessor plugged in to a laptop with a USB cable, you typically might not care at all about conserving power. However, the ATtiny chips are so small that they are fantastic for battery-powered projects. Since an MCU spends most of its time waiting anyway, it makes sense to put it to sleep rather than to have it wasting precious battery life in a delay() statement. In deep sleep mode, the ATtiny85 uses only about 85 microamps. This means that your battery-powered project really doesn't even need a power switch - you can put it to sleep instead.

This sketch shows two routines. The first is a "wake on interrupt" routine called sleep_interrupt(pin#), where you specify the pin number. When this pin changes (from LOW to HIGH or from HIGH to LOW), the microprocessor will wake up again.

The second routine is a "wake on timeout" strategy, where you specify how long you want the MCU to sleep for, and then it will wake up after that time period has elapsed. If it's important in your sketch just to capture the "falling" edge (from HIGH to LOW as you push the switch down), you could read the state of the pin inside ISR(PCINT0_vect)() to a volatile bool declared in global space. 

Remember that during sleep, millis() will stop counting, and delay() won't work. Before you put your MCU to sleep, set all pins to INPUT mode for ultimate power savings. When you wake the chip up, give it at least a ~20 msec delay before you start taking ADC readings again, because the chip will be a little groggy.
