// ping pong 4-byte packets back and forth.
#include "../../../16-nrf-networking/code/nrf-test.h" 
#include "../../../lights-cs240lx-4-ws2812b/code/neopixel.h"
#include "rpi.h"
#include "i2s.h"
#include "../../../16-nrf-networking/code/nrf.h"

#define NEOPIX_PIN1 24
#define NEOPIX_PIN2 25
#define NEOPIX_LEN 30
#define STRIP_ID1 4
#define STRIP_ID2 5

// useful to mess around with these. 
enum { ntrial = 100, timeout_usec = 10000000, nbytes = 4 };  // made a large timeout of 10s

// example possible wrapper to recv a 32-bit value.
static int net_get32(nrf_t *nic, uint32_t *out) {
    int ret = nrf_read_exact_timeout(nic, out, 4, timeout_usec);
    if(ret != 4) {
        //debug("receive failed: ret=%d\n", ret);
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

void notmain(void) {
    caches_enable();
    
    nrf_t *me = client_mk_noack(client_addr, nbytes);
    nrf_dump("before", me);
    dev_barrier();
    neo_t neo1 = neopix_init(NEOPIX_PIN1, NEOPIX_LEN);
    neo_t neo2 = neopix_init(NEOPIX_PIN2, NEOPIX_LEN);
    dev_barrier();
    // here can use nrf_dump(me) and neopix_dump(neo1)

    trace("Begin listening for microphone data over the network.\n");

    unsigned my_addr = me->rxaddr;
    unsigned your_addr = server_addr;  // Kate is using client_addr
    
    // run test.
    while (1) {
        uint32_t neopix_val = 0;
        // SHOULD WE DEV BARRIER HERE bt nrf and gpio stuff
        if(!net_get32(me, &neopix_val)) {
            // ntimeout++;
            nrf_output("me: timeout\n");
            neopix_clear(neo1);
            neopix_clear(neo2);
        } else {
            //nrf_output("me: received %d\n", neopix_idx);
            if (neopix_val >= 0) {
                // 1. locate the correct 5 bit value
                uint32_t strip_height_1 = (neopix_val >> 5 * STRIP_ID1) & 0b11111;
                uint32_t strip_height_2 = (neopix_val >> 5 * STRIP_ID2) & 0b11111;
                printk("strip id=%d height=%d\n", STRIP_ID1, strip_height_1);
                printk("strip id=%d height=%d\n", STRIP_ID2, strip_height_2);

                // COLOR KEY: 
                    // JM = Red for ID1=0, Orange for ID2=1
                    // KE = Yellow for ID1=2, Green for ID2=3
                    // EB = Cyan for ID1=4, Lilac for ID2=5
                    // Red - FF0808, Orange - FF8C08, Yellow - FFDE08
                    // Green - 00FF00, Cyan - 00FFFF, Lilac - CC99FF
                for (int i = 0; i < strip_height_1; i++) {
                    neopix_write(neo1, i, 0x0, 0x80, 0xFF); // 0xB2, 0x66, 0xFF
                }
  
                for (int i = 0; i < strip_height_2; i++) {
                    neopix_write(neo2, i, 0x66, 0x00, 0xCC); // 0x4C, 0x99, 0x0
                }

                neopix_flush(neo1);
                neopix_flush(neo2);
            }
            // demand(exp == got, "exp vs. got mismatch\n");
        }
    }

    // emit all the stats.
    nrf_stat_print(me, "me: done with friend communication test");
}