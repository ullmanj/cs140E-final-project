// simple software 8n1 UART bit-banging implemention.
//   - look for todo() and implement!
//
// NOTE: using usec is going to be too slow for high baudrate, 
// but should be ok for 115200.
//
#include "rpi.h"
#include "cycle-count.h"
#include "sw-uart.h"
#include "cycle-util.h"

// simple putk to the given <uart>
void sw_uart_putk(sw_uart_t *uart, const char *msg) {
    for(; *msg; msg++)
        sw_uart_put8(uart, *msg);
}

// helper: cleans up the code: do a write for <usec> microseconds
//
// code that uses it will have a lot of compounding error, however.  
// if you switch to using cycles for faster baud rates, you should
// instead use
//      <write_cyc_until> in libpi/include/cycle-util.h
static inline void timed_write(int pin, int v, unsigned usec) {
    // gpio_write(pin,v);
    // delay_us(usec);
    // write_cyc_until(pin, v, cycle_cnt_read(), usec);  // I put this in here
}

// do this first: used timed_write to cleanup.
//  recall: time to write each bit (0 or 1) is in <uart->usec_per_bit>
void sw_uart_put8(sw_uart_t *uart, unsigned char c) {
    // write a 0 (start) for T.
    // write each bit value in the given byte for T (starting at bit 0, bit 1, ...).
    // write a 1 (stop) for at-least T.
    unsigned base_cycles = cycle_cnt_read();

    write_cyc_until(uart->tx, 0, base_cycles, uart->cycle_per_bit);  // Start bit

    for(int i = 0; i < 8; i++) {
        base_cycles += uart->cycle_per_bit;
        unsigned value = (c >> i) & 0b1;
        write_cyc_until(uart->tx, value, base_cycles, uart->cycle_per_bit);
    }
    base_cycles += uart->cycle_per_bit;
    write_cyc_until(uart->tx, 1, base_cycles, uart->cycle_per_bit);
}

// do this second: you can type in pi-cat to send stuff.
//      EASY BUG: if you are reading input, but you do not get here in 
//      time it will disappear.
int sw_uart_get8(sw_uart_t *uart) {
    // The GPIO pins (obvious) have no buffering, so if you are reading from the RX pin when the input arrives it will disappear.

   // To minimize problems with the edges of the transitions being off I'd have your code read until you see a start bit, delay T/2 and then start sampling the data bits so that you are right in the center of the bit transmission.
    todo("get8 is an extension");
    
}

// setup the GPIO pins
sw_uart_t sw_uart_mk_helper(unsigned tx, unsigned rx,
        unsigned baud,
        unsigned cyc_per_bit,
        unsigned usec_per_bit) {

    // implement:
    //  1. set rx and tx as input/output.
    //  2: what is the default value of tx for 8n1?  make sure
    //     this is set!! --- has to be 1, so that when it goes down to 0, it knows its a start bit
    gpio_set_input(rx);
    gpio_set_output(tx);
    gpio_set_on(tx);
    gpio_set_on(rx);

    gpio_write(tx, 1);

    // check that the given values make sense.
    //
    // we give you the rest.
    // make sure the value makes sense.
    unsigned mhz = 700 * 1000 * 1000;
    unsigned derived = cyc_per_bit * baud;
    assert((mhz - baud) <= derived && derived <= (mhz + baud));
    // panic("cyc_per_bit = %d * baud = %d\n", cyc_per_bit, cyc_per_bit * baud);

    return (sw_uart_t) {
            .tx = tx,
            .rx = rx,
            .baud = baud,
            .cycle_per_bit = cyc_per_bit ,
            .usec_per_bit = usec_per_bit
    };
}
