// ping pong 4-byte packets back and forth.
#include "../../../16-nrf-networking/code/nrf-test.h" 
#include "../../../lights-cs240lx-4-ws2812b/code/neopixel.h"
#include "rpi.h"

#define NEOPIX_PIN1 10
#define NEOPIX_PIN2 11
#define NEOPIX_LEN 30 // 16

// useful to mess around with these. 
enum { ntrial = 100, timeout_usec = 10000000, nbytes = 4 };  // made a large timeout of 10s

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

// send 4 byte packets from <server> to <client>.  
//
// nice thing about loopback is that it's trivial to know what we are 
// sending, whether it got through, and do flow-control to coordinate
// sender and receiver.
static void
listen_no_ack(nrf_t *me, int verbose_p, neo_t neo1, neo_t neo2) {
    unsigned my_addr = me->rxaddr;
    unsigned your_addr = server_addr;  // Kate is using client_addr
    // unsigned npackets = 0, ntimeout = 0;
    // uint32_t exp = 0, got = 0;

    // RECEIVE
    // exp++;
    while (1) {
        uint32_t neopix_idx = 0;
        if(!net_get32(me, &neopix_idx)) {
            // ntimeout++;
            nrf_output("me: timeout\n");
        } else {
            nrf_output("me: received %d\n", neopix_idx);
            if (neopix_idx >= 0) {
                for (int i = 0; i < neopix_idx; i++) {
                    neopix_write(neo1, i, 0x80, 0x80, 0x0);
                    neopix_write(neo2, i, 0x0, 0x80, 0x80);
                }
                neopix_flush(neo1);
                neopix_flush(neo2);

                printk("index %d\n", neopix_idx);
            }
            // demand(exp == got, "exp vs. got mismatch\n");
        }
    }
}

void notmain(void) {
    caches_enable();

    neo_t neo1 = neopix_init(NEOPIX_PIN1, NEOPIX_LEN);
    neo_t neo2 = neopix_init(NEOPIX_PIN2, NEOPIX_LEN);
    neopix_clear(neo1);
    neopix_clear(neo2);

    trace("Begin listening for microphone data over the network.\n");

    nrf_t *me = server_mk_noack(client_addr, nbytes);  // server_mk_ack vs. client_mk_ack only selects which chip to use on parthiv's board
    // nrf_t *c = client_mk_ack(client_addr, nbytes);

    nrf_stat_start(me);

    // run test.
    listen_no_ack(me, 1, neo1, neo2);

    // emit all the stats.
    // nrf_stat_print(me, "me: done with friend communication test");
}
