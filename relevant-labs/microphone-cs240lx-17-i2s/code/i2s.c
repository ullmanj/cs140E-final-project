#include "i2s.h"
#include "libc/bit-support.h"

volatile i2s_regs_t *i2s_regs = (volatile i2s_regs_t *)I2S_REGS_BASE;
volatile cm_regs_t *cm_regs = (volatile cm_regs_t *)CM_REGS_BASE;

#define addr(x) ((uint32_t)&(x))

enum {
    PCM_CTRL = 0x20101098,
    PCM_DIV = 0x2010109C,

    MODE_A = 0x20203008,
    RXC_A = 0x2020300C,
    CS_A = 0x20203000,
    FIFO_A = 0x20203004,
};

// wrapper function to set the MSB
uint32_t CM_reg_value(uint32_t val) {
    // if(bits_clr(val, 24, 31) != val) {
    //     panic("Setting the MSB of the CM reg would alter attempted input value (%x).\n", val);
    // }
    return val | CM_REGS_MSB;
}

void i2s_init(void) {
    assert(bit_is_off(GET32(CS_A), I2S_CS_EN)); // ensure unit is disabled

    // we use gpio pins pins 18 & 19 (clock stuff), and 20 (signal)
    // Set I2S pins to ALT0
    gpio_set_function(18, GPIO_FUNC_ALT0);
    gpio_set_function(19, GPIO_FUNC_ALT0);
    gpio_set_function(20, GPIO_FUNC_ALT0);

    gpio_set_on(18);
    gpio_set_on(19);
    gpio_set_on(20);

    dev_barrier();

    // part 1 step 3 substep 1
    unsigned val = GET32(PCM_CTRL);
    val = bits_set(val, 0, 3, 0b0001);  // use high res clock (19.2 MHz XTAL clock)
    val = bits_set(val, 9, 10, 0b11);  // enable the 3-stage MASH clock divider.
    val = CM_reg_value(val);
    PUT32(PCM_CTRL, val);

    // part 1 step 3 substep 2
    val = 0;
    val = bits_set(val, 12, 23, CM_DIV_INT);
    val = bits_set(val, 0, 11, CM_DIV_FRAC);
    val = CM_reg_value(val);
    PUT32(PCM_DIV, val);

    dev_barrier();

    // SELECTED Polling. May want to upgrade to interrupts or DMA
    
    val = GET32(MODE_A);
    val = bits_set(val, I2S_MODE_FLEN_LB, I2S_MODE_FLEN_UB, 63);  // page 131
    val = bits_set(val, I2S_MODE_FSLEN_LB, I2S_MODE_FSLEN_UB, 32);
    val = CM_reg_value(val);
    PUT32(MODE_A, val);

    val = GET32(RXC_A);
    val = bit_set(val, I2S_RXC_CH1EN);
    val = bits_set(val, I2S_RXC_CH1WID_LB, I2S_RXC_CH1WID_UB, 8);
    val = bit_set(val, I2S_RXC_CH1WEX);
    val = CM_reg_value(val);
    PUT32(RXC_A, val);

    val = GET32(CS_A);
    val = bit_set(val, I2S_CS_EN);
    val = bit_set(val, I2S_CS_STBY);
    val = bit_set(val, I2S_CS_RXCLR);
    val = bit_set(val, I2S_CS_RXON);
    val = CM_reg_value(val);
    PUT32(CS_A, val);

    dev_barrier();

    /* I2S should now be constantly sending the two clocks out to any peripherals connected to it,
     * and loading samples into the FIFO located at 0x20203004. */
}

int32_t i2s_read_sample(void) {
    dev_barrier();
    unsigned val = 0;

    while(bit_get((val = GET32(CS_A)), I2S_CS_RXD) != 1) { /*output("val: %x\n", val);*/ /*output("waiting\n");*/ }  // busy wait
    
    // unsigned sample = GET32(FIFO_A);
    // output("sample: %x\n", sample);
    // return sample;

    return GET32(FIFO_A);
}