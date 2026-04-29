# ATtiny84 Fast Pin Functions

The **ATtiny84** is a very capable and inexpensive little chip that I like to use in small projects.  

With a bit of help from **ChatGPT** and **Claude.AI**, I wrote a few barebones `#define` functions that help keep memory usage minimal:  

```cpp
// Fast pin modes, writes, and reads for the ATtiny84 (digital pins 0-11)
// Modes: 0 = INPUT, 1 = OUTPUT, 2 = INPUT_PULLUP
#define pinModeFast(p, m) \
  do { \
    if ((p) <= 7) { \
      if ((m) & 1) { \
        DDRA  |=  (1 << (p)); \
        PORTA &= ~(1 << (p)); \
      } else { \
        DDRA  &= ~(1 << (p)); \
        if ((m) & 2) PORTA |=  (1 << (p)); \
        else         PORTA &= ~(1 << (p)); \
      } \
    } else if ((p) >= 8 && (p) <= 10) { \
      if ((m) & 1) { \
        DDRB  |=  (1 << (10 - (p))); \
        PORTB &= ~(1 << (10 - (p))); \
      } else { \
        DDRB  &= ~(1 << (10 - (p))); \
        if ((m) & 2) PORTB |=  (1 << (10 - (p))); \
        else         PORTB &= ~(1 << (10 - (p))); \
      } \
    } \
  } while (0)

#define digitalWriteFast(p, v) \
  do { \
    if ((p) <= 7) { \
      (v) ? PORTA |= (1 << (p)) : PORTA &= ~(1 << (p)); \
    } else if ((p) >= 8 && (p) <= 10) { \
      (v) ? PORTB |= (1 << (10 - (p))) : PORTB &= ~(1 << (10 - (p))); \
    } \
  } while(0)

#define digitalReadFast(p) \
  (((p) <= 7) ? \
    ((PINA & (1 << (p))) ? 1 : 0) : \
   ((p) >= 8 && (p) <= 10) ? \
    ((PINB & (1 << (10 - (p)))) ? 1 : 0) : 0)

// Here are the classic bit functions:
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))
#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))
#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))
```

