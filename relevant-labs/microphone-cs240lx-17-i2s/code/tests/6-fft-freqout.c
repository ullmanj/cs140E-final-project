/*
 * Print out fundamental frequency heard
 */

#include "rpi.h"
#include "fft.h"
#include "i2s.h"

#define FFT_LEN 1024
#define LOG2_FFT_LEN 10
#define FS 44100
// attempt to reject harmonics. change this if you're
// seeing multiples of the fundamental frequency 
#define MAX_THRESH_FACTOR 5 / 4

void notmain(void) {
    enable_cache();
    i2s_init();

    int16_t real[FFT_LEN] = {0};
    int16_t imag[FFT_LEN] = {0};

    while (1) {

        // real samples: set imaginary part to 0
        for (int i = 0; i < FFT_LEN; i++) {
            // printk("signal %d\n", to_q15(i2s_read_sample())); 
            real[i] = to_q15(i2s_read_sample()); 
            imag[i] = 0;
        }

        fft_fixed_cfft(real, imag, LOG2_FFT_LEN, 0);

        int16_t data_max = 0;
        int16_t data_max_idx = 0;

        for (int i = 0; i < FFT_LEN; i++) {
           // printk("\t\t real[i]: %d, imag[i]: %d\n", real[i], imag[i]);
            int32_t mag = fft_fixed_mul_q15(real[i], real[i]) + fft_fixed_mul_q15(imag[i], imag[i]);
            //printk("i %d: mag: %d    real: %d      imag: %d \n", i, mag, fft_fixed_mul_q15(real[i], real[i]), fft_fixed_mul_q15(imag[i], imag[i])); 

           //printk("mag %d\n", mag); 
            // attempt to reject harmonics by requiring higher frequencies to be some factor larger
            //printk("mag > data_max * MAX_THRESH_FACTOR %d", mag > data_max * MAX_THRESH_FACTOR); 
            if (mag > (data_max * MAX_THRESH_FACTOR)) {
                // printk("new mag %d", mag);
                data_max = mag;
                // printk("i %d\n", i); 
                data_max_idx = i;
            }
        }

        int16_t freq = data_max_idx * FS / FFT_LEN;
        // printk("data max idx %d\n", data_max_idx); 

        printk("%d\n", freq);

    }

    clean_reboot();

}