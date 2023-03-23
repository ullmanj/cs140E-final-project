// ping pong 4-byte packets back and forth.
// #include "nrf-test.h"
#include "../../../relevant-labs/16-nrf-networking/code/nrf-test.h"

// useful to mess around with these. 
enum { ntrial = 10, timeout_usec = 10000000, nbytes = 4 };
// enum { my_server = 0xd3d3d3, my_client = 0xe3e3e3 };
enum { my_server = server_addr, my_client = client_addr };

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
    int ret = nrf_send_ack(nic, txaddr, &x, 4);
    if(ret != 4)
        panic("ret=%d, expected 4\n");
}

// send 4 byte packets from <server> to <client>.  
//
// nice thing about loopback is that it's trivial to know what we are 
// sending, whether it got through, and do flow-control to coordinate
// sender and receiver.
static void
ping_pong_ack(nrf_t *s, int verbose_p) {
    // ! HERE JUST ADJUST THE ADDRESS TO ANOTHER, USE MORE TIMEOUTS
    // AND WAIT A BIT LONGER FOR IT TO WORK
    unsigned client_addr = my_client;
    unsigned server_addr = my_server; // s->rxaddr;
    unsigned npackets = 0, ntimeout = 0;
    uint32_t exp = 0, got = 0;

    for(unsigned i = 0; i < ntrial; i++) {
        if(verbose_p && i  && i % 100 == 0)
            trace("sent %d ack'd packets [timeouts=%d]\n", 
                    npackets, ntimeout);

        // net_put32(s, client_addr, ++exp);
        // if(!net_get32(c, &got))
        //     ntimeout++;
        // // could be a duplicate
        // else if(got != exp)
        //     nrf_output("client: received %d (expected=%d)\n", got,exp);
        // else
        //     npackets++;

        // net_put32(c, server_addr, ++exp);
        ++exp;
        if(!net_get32(s, &got))
            ntimeout++;
        else if(got != exp)
            nrf_output("server: received %d (expected=%d)\n", got,exp);
        else {
            nrf_output("server: successfully received %d\n",exp);
            npackets++;
        }

        net_put32(s, client_addr, ++exp);
        delay_us(1000); // to allow it to be received
    }
    trace("trial: total successfully sent %d ack'd packets lost [%d]\n",
        npackets, ntimeout);
}

void notmain(void) {
    // configure server
    // trace("send total=%d, %d-byte messages from server=[%x] to client=[%x]\n",
    //             ntrial, nbytes, server_addr, client_addr);
    trace("send total=%d, %d-byte messages from server=[%x] to client=[%x]\n",
    ntrial, nbytes, my_server, my_client);

    nrf_t *s = server_mk_ack(my_server, nbytes);
    // nrf_t *c = client_mk_ack(client_addr, nbytes);

    nrf_stat_start(s);
    // nrf_stat_start(c);

    // run test.
    ping_pong_ack(s, 1);

    // emit all the stats.
    nrf_stat_print(s, "server: done with one-way test");
    // nrf_stat_print(c, "client: done with one-way test");
}
