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

#define NEOPIX_PIN1 10
#define NEOPIX_PIN2 11
#define NEOPIX_LEN 30 // 16
// was 430 and 800
#define NEOPIX_MIN_FREQ 430
#define NEOPIX_MAX_FREQ 800



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
sending_no_ack(nrf_t *me, int verbose_p) {
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


    for (int i = 0; i < FFT_LEN; i++) {
            int32_t mag = fft_fixed_mul_q15(real[i], real[i]) + fft_fixed_mul_q15(imag[i], imag[i]);
            // attempt to reject harmonics by requiring higher frequencies to be some factor larger
            if (mag > data_max * MAX_THRESH_FACTOR) {
                data_max = mag;
                data_max_idx = i;
            }
    }
        int16_t freq = data_max_idx * FS / FFT_LEN;
        //output("freq %d\n", freq); 
        int neopix_idx = get_idx(freq);
        net_put32(me, friend_addr, neopix_idx);
        nrf_output("sending: %d \n, neopix_idx"); 
        delay_ms(100); 
    }

        // SEND
    //net_put32(me, friend_addr, neopix_idx);
    nrf_output("--me: successfully sent message \n");
}

void notmain(void) {
    // configure server
    // trace("send total=%d, %d-byte messages from server=[%x] to client=[%x]\n",
    //             ntrial, nbytes, server_addr, client_addr);
    trace("Begin friend ping-pong test\n");

    nrf_t *me = server_mk_noack(server_addr, nbytes);  // server_mk_ack vs. client_mk_ack only selects which chip to use on parthiv's board
    // nrf_t *c = client_mk_ack(client_addr, nbytes);

    nrf_stat_start(me);

    // run test.
    sending_no_ack(me, 1);

    // emit all the stats.
    // nrf_stat_print(me, "me: done with friend communication test");
}
