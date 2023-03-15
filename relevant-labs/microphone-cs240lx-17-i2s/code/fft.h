#ifndef FFT_H
#define FFT_H

#include "rpi.h"
// #include "libc/bit-support.h"
#define MAX_31 2147483648  // 2^31

// int16 * int16 -> int32 multiply -- specific to arm_none_eabi
inline int32_t fft_fixed_mul(int16_t a, int16_t b) {
    // SMLABB // bottom bottom
    int32_t ret = 0;
    asm volatile("SMULBB %0, %1, %2" : "=r" (ret) : "r" (a), "r" (b)); // first param is where result is stored
                // output (assign to this): inputs (read from these)
    return ret;
}

// converts uint32 centered at INT_MAX into int16 (Q.15) centered at 0
inline int16_t to_q15(uint32_t x) {
<<<<<<< HEAD
    // first shift by 1 to the right
    // x = x >> 1;
    // // then cast into a int32_t
    // int32_t y = (int32_t) x;
    // // then we subtract 2^32-1 to get it into the right range
    // y -= 2^32 - 1;
    // // then shift over by 15 to the right 12
    // y = y >> 2;
    // return (int16_t)((x << 2) >> 15); // 
    unimplemented();
=======
    uint32_t val = x - (1 << 31); // clear msb (since it is always 1)
    return (uint16_t)(val >> 10); // 10
>>>>>>> f50bd4303a210ef7d36305d0efb59bb1ff465fc1
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