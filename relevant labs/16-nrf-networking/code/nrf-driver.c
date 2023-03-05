#include "nrf.h"
#include "nrf-hw-support.h"

// enable crc, enable 2 byte
#   define set_bit(x) (1<<(x))
#   define enable_crc      set_bit(3)
#   define crc_two_byte    set_bit(2)
#   define mask_int         set_bit(6)|set_bit(5)|set_bit(4)
enum {
    // pre-computed: can write into NRF_CONFIG to enable TX.
    tx_config = enable_crc | crc_two_byte | set_bit(PWR_UP) | mask_int,
    // pre-computed: can write into NRF_CONFIG to enable RX.
    rx_config = tx_config  | set_bit(PRIM_RX)
} ;

nrf_t * nrf_init(nrf_conf_t c, uint32_t rxaddr, unsigned acked_p) {
    // nrf_t *n = staff_nrf_init(c, rxaddr, acked_p);
    dev_barrier();

    nrf_t *n = kmalloc(sizeof *n);
    n->config = c;
    nrf_stat_start(n);
    n->spi = pin_init(c.ce_pin, c.spi_chip);  // Setup GPIO and SPI first
    n->rxaddr = rxaddr;
    dev_barrier();
    gpio_set_on(c.ce_pin);  // turn GPIO pin on
    dev_barrier();
    // cq_init(&(n->recvq), 1);  // initialize queue
    
    // Put the chip in power down before configure
    nrf_set_pwrup_off(n);
    assert(!nrf_is_pwrup(n));

    nrf_put8_chk(n, NRF_EN_RXADDR, 0); // clean slate
    nrf_put8_chk(n, NRF_EN_AA, 0); // clean slate

    // enable pipes
    if(!acked_p) {
        nrf_bit_set(n, NRF_EN_RXADDR, 1);  // enable only data pipe 1
        // keep disabled auto-ack on all pipes (pipe 0 and 1)

        assert(!nrf_pipe_is_enabled(n, 0));
        assert(nrf_pipe_is_enabled(n, 1));
        assert(!nrf_pipe_is_acked(n, 0));
        assert(!nrf_pipe_is_acked(n, 1));
    } else {
        nrf_bit_set(n, NRF_EN_RXADDR, 0);  // enable data pipe 0
        nrf_bit_set(n, NRF_EN_RXADDR, 1);  // enable data pipe 1
        nrf_bit_set(n, NRF_EN_AA, 0);  // auto-ack on pipe 0
        nrf_bit_set(n, NRF_EN_AA, 1);  // auto-ack on pipe 1

        assert(nrf_pipe_is_enabled(n, 0));
        assert(nrf_pipe_is_enabled(n, 1));
        assert(nrf_pipe_is_acked(n, 0));
        assert(nrf_pipe_is_acked(n, 1));

        // set up retran
        unsigned retran_reg_input = nrf_default_retran_attempts | (nrf_default_retran_delay << 4);  // p. 58 (reg 4)
        nrf_put8_chk(n, NRF_SETUP_RETR, retran_reg_input);
    }

    // setup address size
    unsigned v = nrf_default_addr_nbytes - 2;  // subtract 2. don't know why. QUESTION
    nrf_put8_chk(n, NRF_SETUP_AW, v);

    // setup message size and address size
    nrf_put8_chk(n, NRF_RX_PW_P1, c.nbytes);
    nrf_set_addr(n, NRF_RX_ADDR_P1, n->rxaddr, nrf_default_addr_nbytes);
    assert(nrf_get_addr(n, NRF_RX_ADDR_P1, nrf_default_addr_nbytes) == n->rxaddr);
    
    // set message size = 0 for unused pipes. probably redundant
    nrf_put8_chk(n, NRF_RX_PW_P2, 0);
    nrf_put8_chk(n, NRF_RX_PW_P3, 0);
    nrf_put8_chk(n, NRF_RX_PW_P4, 0);
    nrf_put8_chk(n, NRF_RX_PW_P5, 0);

    // end of pipe setup ---

    // setup channel --- this is for all addresses.
    nrf_put8_chk(n, NRF_RF_CH, c.channel);

    // setup data rate and power
    assert(!bits_intersect(nrf_default_data_rate, nrf_default_db));
    nrf_put8_chk(n, NRF_RF_SETUP, nrf_default_data_rate | nrf_default_db);
    
    // flush FIFOs
    nrf_rx_flush(n);
    nrf_tx_flush(n);

    nrf_put8(n, NRF_STATUS, ~0);  // clear all interrupt bits // p. 59 (reg 7)
    assert(nrf_get8(n, NRF_STATUS) == (0b111 << 1));  // ensure that "RX FIFO Empty"

    // should be true after flushed rx and tx and enabled pipes
    assert(!nrf_tx_fifo_full(n));
    assert(nrf_tx_fifo_empty(n));
    assert(!nrf_rx_fifo_full(n));
    assert(nrf_rx_fifo_empty(n));

    assert(!nrf_has_rx_intr(n));
    assert(!nrf_has_tx_intr(n));
    assert(pipeid_empty(nrf_rx_get_pipeid(n)));
    assert(!nrf_rx_has_packet(n));
    // -- 

    // Dynamic payload (not used today)
    // reg=0x1c dynamic payload (next register --- don't set the others!) assert(nrf_get8(n, NRF_DYNPD) == 0);
    // reg 0x1d: feature register. we don't use it yet. nrf_put8_chk(n, NRF_FEATURE, 0);
    // --

// I think this is redundant
    // // flush and check again
    // nrf_tx_flush(&nic);
    // nrf_rx_flush(&nic);

    // // i think w/ the nic is off, this better be true.
    // assert(nrf_tx_fifo_empty(&nic));
    // assert(nrf_rx_fifo_empty(&nic));
// ---(but his solution had it)
    
    // power up and wait
    nrf_set_pwrup_on(n);
    assert(nrf_is_pwrup(n));
    delay_ms(2);

    // Finally put the device in RX mode
    nrf_put8_chk(n, NRF_CONFIG, rx_config);
    assert(nrf_is_rx(n));

    dev_barrier();
    gpio_write(n->config.ce_pin, 1);  // p. 22 - go to RX Settling state (must be after PRIM_RX is set)
    dev_barrier();
    // delay_ms(2);

    // should be true after setup.
    if(acked_p) {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    } else {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    }
    return n;
}

int nrf_tx_send_ack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {
    dev_barrier();
    // default config for acked state.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n));

    nrf_set_addr(n, NRF_RX_ADDR_P0, txaddr, nrf_default_addr_nbytes);  // set the P0 pipe to the same address you are sending to (for acks).

    // TODO: you would implement the rest of the code at this point.
    // int res = staff_nrf_tx_send_ack(n, txaddr, msg, nbytes);
    dev_barrier();
    gpio_write(n->config.ce_pin, 0);  // go to Standby-1 mode (p. 22)
    dev_barrier();

    nrf_put8_chk(n, NRF_CONFIG, tx_config);
    assert(nrf_is_tx(n));
    // Set the TX address.
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_default_addr_nbytes);  // want to overwrite all 40 bits of the register
    // Use nrf_putn and NRF_W_TX_PAYLOAD to write the message to the device
    unsigned status = nrf_putn(n, W_TX_PAYLOAD_NO_ACK, msg, nbytes);
    // Turn the CE pin on
    dev_barrier();
    gpio_write(n->config.ce_pin, 1);  //(p. 22) go to TX Settling transition state into the TX Mode
    dev_barrier();
    // delay_us(15); // needed more than 10µs
    // gpio_write(n->config.ce_pin, 0);  // turn off before empty allowing us to return to Standby-I when packet finishes
    // Wait until the TX fifo is empty.
    while(!nrf_tx_fifo_empty(n)) {
        //MAX_RT
        if(nrf_has_max_rt_intr(n)) {  // hit maximum number of retransmits
            nrf_rt_intr_clr(n);
            panic("Hit max number of retransmits.\n");
        }
        if(nrf_has_rx_intr(n))  // redundancy check
            break;
    }
    dev_barrier();
    gpio_write(n->config.ce_pin, 0);  // head back to Standby-1
    dev_barrier();
    // Clear the TX interrupt.
    nrf_tx_intr_clr(n);
    // When you are done, don't forget to set the device back in RX mode.
    nrf_put8_chk(n, NRF_CONFIG, rx_config);
    assert(nrf_is_rx(n));
    dev_barrier();
    gpio_write(n->config.ce_pin, 1);  // go to RX Settling mode -> RX mode (p. 22)
    dev_barrier();
    // delay_ms(2);

    // uint8_t cnt = nrf_get8(n, NRF_OBSERVE_TX);
    // n->tot_retrans  += bits_get(cnt,0,3);

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return nbytes;  // was "res"
}

int nrf_tx_send_noack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {
    dev_barrier();
    // default state for no-ack config.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n));

    // TODO: you would implement the code here.
    // int res = staff_nrf_tx_send_noack(n, txaddr, msg, nbytes);
    // Set the device to TX mode.
    dev_barrier();
    gpio_write(n->config.ce_pin, 0);  // go to Standby-1 mode (p. 22)
    dev_barrier();

    nrf_put8_chk(n, NRF_CONFIG, tx_config);
    assert(nrf_is_tx(n));
    // Set the TX address.
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_default_addr_nbytes);  // want to overwrite all 40 bits of the register
    // Use nrf_putn and NRF_W_TX_PAYLOAD to write the message to the device
    unsigned status = nrf_putn(n, W_TX_PAYLOAD_NO_ACK, msg, nbytes);
    // Turn the CE pin on
    dev_barrier();
    gpio_write(n->config.ce_pin, 1);  //(p. 22) go to TX Settling transition state into the TX Mode
    dev_barrier();
    // delay_us(15); // needed more than 10µs
    // gpio_write(n->config.ce_pin, 0);  // turn off before empty allowing us to return to Standby-I when packet finishes
    // Wait until the TX fifo is empty.
    while(!nrf_tx_fifo_empty(n));  // busy wait
    dev_barrier();
    gpio_write(n->config.ce_pin, 0);  // head back to Standby-1
    dev_barrier();
    // Clear the TX interrupt.
    nrf_tx_intr_clr(n);
    // When you are done, don't forget to set the device back in RX mode.
    nrf_put8_chk(n, NRF_CONFIG, rx_config);
    assert(nrf_is_rx(n));
    dev_barrier();
    gpio_write(n->config.ce_pin, 1);  // go to RX Settling mode -> RX mode (p. 22)
    dev_barrier();
    // delay_ms(2);

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));
    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return nbytes;//status;  // was "res"
}

int nrf_get_pkts(nrf_t *n) {
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    // TODO:
    // data sheet gives the sequence to follow to get packets.
    // p63: 
    //    1. read packet through spi.
    //    2. clear IRQ.
    //    3. read fifo status to see if more packets: 
    //       if so, repeat from (1) --- we need to do this now in case
    //       a packet arrives b/n (1) and (2)
    // done when: nrf_rx_fifo_empty(n)
    // int res = staff_nrf_get_pkts(n);
    unsigned num_packets_received = 0;
    dev_barrier();
    while(!nrf_rx_fifo_empty(n)) {  // While the RX fifo is not empty, spin a loop pulling packets.
        unsigned pipeID = nrf_rx_get_pipeid(n);
        if(pipeID != 1)
            panic("Received a packet for pipe %d. Expected 1.\n", pipeID);

        unsigned num_bytes = nrf_get8(n, NRF_RX_PW_P1); // RX_PW_P1 has num bytes in RX payload  //was: "n->config.nbytes;"
        #define NRF_MAX_PKT 32 // lots of extra copies: temp buffer.
        uint8_t received_msg[NRF_MAX_PKT];

        unsigned status = nrf_getn(n, NRF_R_RX_PAYLOAD, received_msg, num_bytes);
        assert(pipeID = pipeid_get(status)); // extra check
        
        //and push it onto the recvq
        if(!cq_push_n(&(n->recvq), received_msg, num_bytes))
            panic("cqueue ran out of space for pipe #%d\n", pipeID);

        n->tot_recv_msgs++;
        n->tot_recv_bytes += num_bytes;
        num_packets_received++;
        
        nrf_rx_intr_clr(n);  // clear the RX interrupt
    }

    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    dev_barrier();
    return num_packets_received;
}
