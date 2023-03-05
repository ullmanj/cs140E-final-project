// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"
#include "gpio.h"

enum {
    AUXENB = 0x20215004,
    AUX_MU_CNTL_REG = 0x20215060,
    AUX_MU_IIR_REG = 0x20215048,
    AUX_MU_LCR_REG = 0x2021504C,
    AUX_MU_IER_REG = 0x20215044,
    AUX_MU_BAUD = 0x20215068,
    AUX_MU_IO_REG = 0x20215040,
    AUX_MU_LSR_REG = 0x20215054,
    AUX_MU_STAT_REG = 0x20215064,
};

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.

//It should set the baud rate to 115,200 and leave the miniUART in its default 8n1 configuration.
void uart_init(void) {
    unsigned RX_pin = 15;
    unsigned TX_pin = 14;
    
    dev_barrier();

    // setup GPIO pins 
    gpio_set_function(RX_pin, GPIO_FUNC_ALT5);
    gpio_set_on(RX_pin);
    gpio_set_function(TX_pin, GPIO_FUNC_ALT5);
    gpio_set_on(TX_pin);

    dev_barrier();

    // enable miniUART using AUX
    unsigned value = GET32(AUXENB);// & 0b111;  // just want to preserve bottom 3 bits
    unsigned mask = 0b1;
    value |= mask;  // set on 0th bit to enable miniUART functionality
    PUT32(AUXENB, value);

    dev_barrier();

    // "disable" miniUART by disabling RX and TX
        // value = GET32(AUX_MU_CNTL_REG);
        // mask = 0b11;
        // value &= ~mask;  // clear on 0th and 1st bits to disable TX and RX through miniUART
        // PUT32(AUX_MU_CNTL_REG, value);
    PUT32(AUX_MU_CNTL_REG, 0);  // disable flow control and rx and TX. No read-modify.

    // check that Receiver is idle before continuing. (As suggeted in Broadcom p. 19)
    while(((GET32(AUX_MU_STAT_REG) >> 2) & 0b1) != 1) {}  // busy wait here to make sure no more coming in  // IMPORTANT: may need to comment out to match checksum ---------

    dev_barrier();  // just in case, but not required

    // clear FIFO queues
    PUT32(AUX_MU_IIR_REG, 0b11 << 1); // set 1st and 2nd bits without reading or modifying. (the rest of the reg is read-only)

    dev_barrier();  // just in case, but not required

    // dissable interrupts
        // set DLAB = 0. Should be 0 on reset, but doing manually just incase.
        value = GET32(AUX_MU_LCR_REG);
        mask = 0b1 << 7;  // POTENTIAL ISSUE: has protected regions
        value &= ~mask;  // clear on 0th bits to ensure DLAB = 0
        PUT32(AUX_MU_LCR_REG, value);
    
    dev_barrier();  // just in case, but not required
    value = GET32(AUX_MU_IER_REG);
    mask = 0b11;
    value &= ~mask;  // clear on 0th and 1st bits to disable TX and RX interrupts
    PUT32(AUX_MU_IER_REG, value);

    dev_barrier();  // just in case, but not required

    // Configure: 115200 Baud, 8 bits, 1 start bit, 1 stop bit. No flow control.
    // set baud rate to 115200 -› set baud reg to 270
    PUT32(AUX_MU_BAUD, 270); // no need to read. Just set.

    // configure 8 bits (1 start bit, 1 stop bit is implicit).
    value = GET32(AUX_MU_LCR_REG);
    mask = 0b11;
    value |= mask;  // set on 0th and 1st bits to put miniUART in 8-bit mode
    PUT32(AUX_MU_LCR_REG, value);

    // enable RX TX
    PUT32(AUX_MU_CNTL_REG, 0b11);  // can just blast

    dev_barrier();
}

// disable the uart.
// We do the following: flush queues, disable RX and TX, and then disable miniUART using AUX
void uart_disable(void) {
    dev_barrier();
    
    // wait for queues to flush
    uart_flush_tx();
    // disable RX TX
    PUT32(AUX_MU_CNTL_REG, 0);

    dev_barrier();

    // disable miniUART using AUX
    unsigned value = GET32(AUXENB);// & 0b111;  // just want to preserve bottom 3 bits
    unsigned mask = 0b1;
    value &= ~mask;  // clear 0th bit to disable miniUART functionality
    PUT32(AUXENB, value);

    dev_barrier();
}


// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
    while(uart_has_data() != 1) { } // hang until we have data

    return (int) (GET32(AUX_MU_IO_REG) & 0xff);  // return lower 8 bits of IO reg (bits 0 to 7).
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    return ((GET32(AUX_MU_LSR_REG) >> 5) & 0b1);
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    while(uart_can_put8() != 1) {} // busy wait until we can put a byte
    
    PUT32(AUX_MU_IO_REG, c);  // blast it in
    return 0; // ERROR?????
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    unsigned holds_valid_byte = 0b10;
    return (((GET32(AUX_MU_IIR_REG) & 0b110) >> 1) == holds_valid_byte);  // bits 1 and 2 hold the code. (p. 13)
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    return (GET32(AUX_MU_STAT_REG) >> 9) & 0b1;  // IMPORTANT: change to 8 to try to match checksum, potentially --------- --------- --------- --------- ---------
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        ;
}
