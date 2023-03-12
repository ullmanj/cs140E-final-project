#ifndef FFT_H
#define FFT_H

#include "rpi.h"
// #include "libc/bit-support.h"
#define MAX_31 2147483648  // 2^31

// int16 * int16 -> int32 multiply -- specific to arm_none_eabi
inline int32_t fft_fixed_mul(int16_t a, int16_t b) {
    //a: r0
    //b: r1
    // SMLABB // bottom bottom
    int32_t ret = 0;
    // dev_barrier();  // multiplier is different device I suppose
    asm volatile("SMULBB r2, r0, r1");
    // dev_barrier();
    asm volatile("mov r0, r2" : "=r" (ret));
    
    return ret;
}

// converts uint32 centered at INT_MAX into int16 (Q.15) centered at 0
inline int16_t to_q15(uint32_t x) {
    // keeping: X---------------XX00000000000000
    // x = x >> 1; 
    unsigned mask = 1 << 31;
    x = x & ~mask; // we want to lose the MSB because it will rarely change
    int32_t signed_val = (int32_t)x; // all positive
    signed_val -= MAX_31;  // Now we have a value ranging from [-2^31, 2^31 - 1] // for us: [-2^30, 2^30 - 1]
    output("signed val %d", signed_val); 

    // step 2 fit into 16 bits
    return (int16_t)(signed_val >> (14 + 2)); // lose the 2 LSB + 14 0's.
}

// multiplies Q.15 * Q.15 into Q.15
inline int16_t fft_fixed_mul_q15(int16_t a, int16_t b) {
    int32_t c = fft_fixed_mul(a, b);
    // save the most significant bit that's lost (round up if set)
    int32_t round = (c >> 14) & 1;
    return (c >> 15) + round;
}

int32_t fft_fixed_cfft(int16_t *real, int16_t *imag, int16_t log2_len, unsigned inverse);
int32_t fft_fixed_rfft(int16_t *data, int32_t log2_len, unsigned inverse);

#endif // FFT_H