// ping pong 4-byte packets back and forth.
#include "../../../16-nrf-networking/code/nrf-test.h" 

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

#define NEOPIX_LEN 30 // 16
#define NUM_BUCKETS 6 
// was 430 and 800
#define NEOPIX_MIN_FREQ 430
#define NEOPIX_MAX_FREQ 800


#define NEOPIX_DIV 30 //???

#define START_IDX ((NEOPIX_MIN_FREQ * FFT_LEN)/ FS)
#define FREQS_PER_BUCKET ((NEOPIX_MAX_FREQ - NEOPIX_MIN_FREQ) / (NUM_BUCKETS) / (FS / FFT_LEN))



// useful to mess around with these. 
enum { ntrial = 1, timeout_usec = 10000000, nbytes = 4 };  // made a large timeout of 10s

// example possible wrapper to recv a 32-bit value.
static int net_get32(nrf_t *nic, uint32_t *out) {
    int ret = nrf_read_exact_timeout(nic, out, 4, timeout_usec);
    if(ret != 4) {
        debug("receive failed: ret=%d\n", ret);
        return 0;
    }
    return 1;
}
// example possible wrapper to send a 32-bit value.
static void net_put32(nrf_t *nic, uint32_t txaddr, uint32_t x) {
    int ret = nrf_send_noack(nic, txaddr, &x, 4);
    if(ret != 4)
        panic("ret=%d, expected 4\n");
}

int get_idx(int freq) {
    // TODO! FIX!
    if (freq < NEOPIX_MIN_FREQ || freq > NEOPIX_MAX_FREQ) {
        return -1;
    }

    return ((freq - NEOPIX_MIN_FREQ) * NEOPIX_LEN) / (NEOPIX_MAX_FREQ - NEOPIX_MIN_FREQ);
}



// send 4 byte packets from <server> to <client>.  
//
// nice thing about loopback is that it's trivial to know what we are 
// sending, whether it got through, and do flow-control to coordinate
// sender and receiver.
static void
sending_no_ack(nrf_t *me) {
    unsigned my_addr = me->rxaddr;
    unsigned friend_addr = client_addr;  // Kate is using client_addr
    unsigned npackets = 0, ntimeout = 0;


    caches_enable(); //enable_cache();
    i2s_init();
    int16_t real[FFT_LEN] = {0};
    int16_t imag[FFT_LEN] = {0};

    while (1) {
        for (int i = 0; i < FFT_LEN; i++) {
                real[i] = to_q15(i2s_read_sample());
                imag[i] = 0;
        }

        fft_fixed_cfft(real, imag, LOG2_FFT_LEN, 0);

        int16_t data_max = 0;
        int16_t data_max_idx = 0;

        uint32_t pkg = 0; 
        unsigned bucket_i = 0;
         for (int i = 0; i < NUM_BUCKETS; i++) {
             bucket_i = i;
            // for each bucket (1 per neopixel...)
            int32_t acc = 0;
            for (int j = 0; j < FREQS_PER_BUCKET; j++) {
                // accumulate the magnitude squared of all the frequencies in this bucket
                int fft_idx = START_IDX + (FREQS_PER_BUCKET*i) + j;
                acc += fft_fixed_mul_q15(real[fft_idx], real[fft_idx]) + fft_fixed_mul_q15(imag[fft_idx], imag[fft_idx]);
            }
            // clip at 0xFF
            int val = (acc > 0xFF * NEOPIX_DIV) ? 0xFF : acc / NEOPIX_DIV;
            pkg |= (val << (bucket_i * 5));  
            output("val %d\n", val); 
        }
 
        output("--- pkg contents %x\n", pkg); 
        net_put32(me, friend_addr, pkg);
        
        // unsigned bucket_i = 0;
        // for (int i = 0; i < FFT_LEN; i++) {
        //     bucket_i = i / (FFT_LEN / NUM_BUCKETS);
        //     if (bucket_i < 0 || bucket_i > 5) {
        //         output("Classic CS 140E whoopsie!\n");
        //         continue;
        //     }
        //     output("Bucket #%d\n", bucket_i);
        //     // for each bucket (1 per neopixel...)
        //     int32_t acc = 0;
        //     // for (int j = 0; j < FREQS_PER_BUCKET; j++) {
        //         // accumulate the magnitude squared of all the frequencies in this bucket
        //         // int fft_idx = START_IDX + (FREQS_PER_BUCKET*i) + j;
        //         // uint32_t mag = fft_fixed_mul_q15(real[i], real[i]) + fft_fixed_mul_q15(imag[i], imag[i]);
        //         // if (mag > NEOPIX_MIN_FREQ && mag < NEOPIX_MAX_FREQ) {
        //         //     acc += mag;
        //         // }
        //         acc += fft_fixed_mul_q15(real[i], real[i]) + fft_fixed_mul_q15(imag[i], imag[i]);
        //     // }
        //     // clip at 0xFF
        //     int val = 0; 
        //     // if (i ==0 && (acc > NEOPIX_LEN * NEOPIX_DIV)) {
        //     //     val = 0; 
        //     // } else {
        //         val = (acc > NEOPIX_LEN * NEOPIX_DIV) ? NEOPIX_LEN : acc / NEOPIX_DIV; // NEOPIX_LEN

        //     // }
        //     pkg |= (val << (bucket_i * 5));  
            
        // }
        // output("--- pkg contents %x\n", pkg); 
        // net_put32(me, friend_addr, pkg);
    }
 // will want to send here 
}

void notmain(void) {
    trace("Begin friend ping-pong test\n");

    nrf_t *me = server_mk_noack(server_addr, nbytes);  // server_mk_ack vs. client_mk_ack only selects which chip to use on parthiv's board
 
    sending_no_ack(me);
}