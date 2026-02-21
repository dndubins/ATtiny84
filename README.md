# ATtiny84 Fast Pin Functions

The **ATtiny84** is a very capable and inexpensive little chip that I like to use in small projects.  

With a bit of help from **ChatGPT**, I wrote a few barebones `#define` functions that help keep memory usage minimal:  

```cpp
// Fast pin modes, writes, and reads for the ATtiny84 (digital pins 0-8)
// Modes: 0 = INPUT, 1 = OUTPUT, 2 = INPUT_PULLUP

// For fast pin modes, writes, and reads for the ATtiny84:
#define pinModeFast(p, m) \
  if ((p) < 8) { \
    if ((m)&1) DDRA |= 1 << (p); \
    else DDRA &= ~(1 << (p)); \
    if (!((m)&1)) ((m)& 2 ? PORTA |= 1 << (p) : PORTA &= ~(1 << (p))); \
  } else { \
    if ((m)&1) DDRB |= 1; \
    else DDRB &= ~1; \
    if (!((m)&1)) ((m)& 2 ? PORTB |= 1 : PORTB &= ~1); \
  }

#define digitalWriteFast(p, v) \
  if ((p) < 8) { \
    (v) ? PORTA |= 1 << (p) : PORTA &= ~(1 << (p)); \
  } else { \
    (v) ? PORTB |= 1 : PORTB &= ~1; \
  }

#define digitalReadFast(p) \
  ((p) < 8 ? ((PINA & (1 << (p))) ? 1 : 0) : ((PINB & 1) ? 1 : 0))
```
**Caveat:** Be careful when putting these inside if() statements, because they themselves contain if statements and the compiler might get confused.
