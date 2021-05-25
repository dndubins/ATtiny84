# User Guide for the LabToy84
  
- There are two momentary switches on this device: a MODE button (called sw2 in the program) and a SET button (called sw1 in the program).
- The program distinguishes between a short push (<0.5 sec) and a long push (>0.5 sec) on each button.

## To Change Modes: 
- Pressing the MODE button (short push) will cycle between the following modes: <b>Clock --> Temperature --> Timer --> Stopwatch</b>
- Pressing the MODE button (long push) will allow you to set the brightness of the LED 4-digit 7-segment display from (0-7), where 0: dimmest, 7: brightest
- A short push on SET cycles between the options 0-7 then OFF
- A long push on SET accepts the current option displayed on the screen
- "OFF" is a battery saving mode, where the LED display will use a brightness of 3, but only light up momentarily when you press a button. This prolongs battery life and would also be useful if the dimmest setting of the screen is keeping you awake at night

The method described here to set a number is true for the rest of the options in general: a short SET button push cycles the options, and a long SET button push accepts the current option displayed.
  
## To Set The Clock:
   - While in Clock mode, a long push on the SET button enters "set all" mode
   - Use the SET button for each push below: (short push to advance number, long push to accept)
   - set HRS with short push, accept with long push (screen will prompt with am and pm)
   - set MIN with short push, accept with long push
   - set DAY with short push, accept with long push
   - set MONTH with short push, accept with long push
   - set ALARM on/off with short push, accept with long push
   - if ALARM on:
      - set ALARM HRS with short push, accept with long push
      - set ALARM MIN with short push, accept with long push

### To Snooze or Silence Clock Alarm:
   - When the ALARM rings, the MODE button will stop the alarm from ringing. Alarm mode stays ON (will ring the next day). 
   - The SET button is a 5-minute snooze bar. You can keep pressing the SET button to sound the alarm 5 minutes later.
   - If you allow the alarm to ring and miss the snooze bar, alarm mode will stay on, and ring the next day.

### Brightness Menu: to Set LED brightness, and to Disable Clock Mode
Setting the brightness lower will save battery life.
Setting the clock mode to "OFF" will put the microprocessor to sleep when not using the other modes. This will also save battery life.

   - A long push on the MODE button enters "set brightness and LED on/off" menu. This menu is accessible from Clock and Timer modes.  
   - Set LED BRIGHTNESS with short push (SET button), accept with long push (SET button)
   - Options: 0(most dim) to 7 (most bright), then OFF
   - OFF option keeps LED off unless a button is pushed (battery saving)
   - Set CLOCK on/off with short push, accept with long push
   - BATT status will display (in %). If using USB, this will just display 100%.

## Temperature Mode
  - Temperature will report every 5 seconds.
  - To leave this mode, press the MODE button.
  - The SET button does nothing here.

## Timer Mode
  - "PUSH" will be displayed on the LED screen.
  - To start the timer, add time by pressing the SET button.
  - Each time you press the SET button, more time is added (+1 min increments, then as you keep pressing, the increments increase). 
  - The timer will automatically start when you let go.
  - You can add time by pressing the SET button at any time while the timer is counting down.
  - The timer will ring, and reset when the time is up.
  - If you would like to silence the timer alarm while it is ringing, press any button.
  - To reset the timer, hold down the SET button (long push), or press the MODE button to change modes.
  - The timer will not continue if you change modes. You need to stay in the Timer mode to use the timer.

## Stopwatch Mode
  - "00" will be displayed on the LED screen.
  - To start the stopwatch, short press the SET button.
  - To reset the stopwatch, long press the SET button.
  - brightness menu cannot be accessed from this mode.