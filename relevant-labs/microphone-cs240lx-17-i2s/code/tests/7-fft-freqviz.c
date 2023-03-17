/*
 * Print out fundamental frequency and display
 * an increasing bar on the neopixel ring
 */

#include "rpi.h"
#include "fft.h"
#include "i2s.h"
// #include "neopixel.h"
#include "../../../lights-cs240lx-4-ws2812b/code/neopixel.h"


#define LOG2_FFT_LEN 10
#define FFT_LEN (1 << LOG2_FFT_LEN)

#define FS 44100
// attempt to reject harmonics. change this if you're
// seeing multiples of the fundamental frequency 
#define MAX_THRESH_FACTOR 5 / 4

#define NEOPIX_PIN 21 // from 2
#define NEOPIX_LEN 30 // 16
// was 430 and 800
#define NEOPIX_MIN_FREQ 430
#define NEOPIX_MAX_FREQ 800

int get_idx(int freq) {
    // TODO! FIX!
    if (freq < NEOPIX_MIN_FREQ || freq > NEOPIX_MAX_FREQ) {
        return -1;
    }

    return ((freq - NEOPIX_MIN_FREQ) * NEOPIX_LEN) / (NEOPIX_MAX_FREQ - NEOPIX_MIN_FREQ);
}

void notmain(void) {
    enable_cache();
    i2s_init();
    neo_t neo = neopix_init(NEOPIX_PIN, NEOPIX_LEN);
    neopix_clear(neo);

    int16_t real[FFT_LEN] = {0};
    int16_t imag[FFT_LEN] = {0};

    while (1) {

        // real samples: set imaginary part to 0
        for (int i = 0; i < FFT_LEN; i++) {
            real[i] = to_q15(i2s_read_sample());
            imag[i] = 0;
        }

        fft_fixed_cfft(real, imag, LOG2_FFT_LEN, 0);

        int16_t data_max = 0;
        int16_t data_max_idx = 0;

        for (int i = 0; i < FFT_LEN; i++) {
            int32_t mag = fft_fixed_mul_q15(real[i], real[i]) + fft_fixed_mul_q15(imag[i], imag[i]);
            // attempt to reject harmonics by requiring higher frequencies to be some factor larger
            if (mag > data_max * MAX_THRESH_FACTOR) {
                data_max = mag;
                data_max_idx = i;
            }
        }
        
        int16_t freq = data_max_idx * FS / FFT_LEN;
        //output("\tdata_max: %d\n\tdata_max_i=%d\n\tfreq=%d\n", data_max, data_max_idx, freq);

        int neopix_idx = get_idx(freq);
        for (int i = 0; i < neopix_idx; i++) {
            neopix_write(neo, i, 0x80, 0x80, 0x80);
        }
        // neopix_fancy_set(neo, neopix_idx);
        neopix_flush_up_to_keep(neo, neopix_idx);
        /// neopix_flush(neo);
        if (neopix_idx >= 0)
        printk("%dHz, index %d\n", freq, neopix_idx);

    }
    neopix_clear(neo);
    // for (int i = 0; i < NEOPIX_LEN; i++) {
    //     neopix_write(neo, i, 0x0, 0x0, 0x0);
    // }
    clean_reboot();
    
}