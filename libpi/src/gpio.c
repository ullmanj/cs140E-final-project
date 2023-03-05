/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34)
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

void gpio_set_function(unsigned pin, gpio_func_t function) {
    if (pin >= 32)
        return;
    if (function > 0b111)  // From broadcom document.
        return;
    
    unsigned register_idx = pin / 10;
    unsigned shift = (pin % 10) * 3;
    unsigned our_set = GPIO_BASE + register_idx * 4;
    unsigned mask = 0b111;

    unsigned value = GET32(our_set);
    value &= ~(mask << shift);
    value |= function << shift;

    PUT32(our_set, value);
}

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
    gpio_func_t function = 0b001;
    gpio_set_function(pin, function);
}

// set GPIO <pin> on
void gpio_set_on(unsigned pin) {
    if(pin >= 32)
        return;
    // implement this, use <gpio_set0>
    PUT32(gpio_set0, 0x1 << pin);
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    if(pin >= 32)
        return;
    // implement this, use <gpio_clr0>
    PUT32(gpio_clr0, 0x1 << pin);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(pin >= 32)
        return;
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
    gpio_func_t function = 0b000;
    gpio_set_function(pin, function);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
  if(pin >= 32)
      return -1;
  // unsigned v = 0;
  unsigned level_set = GET32(gpio_lev0);
  unsigned x = (level_set >> pin) & 0b1;
  return DEV_VAL32(x);
}
