/*
 * "higher-level" neopixel stuff.
 * our model:
 *  1. you write to an array with as many entries as there are pixels as much as you 
 *     want
 *  2. you call <neopix_flush> when you want to externalize this.
 *
 * Note:
 *  - out of bound writes are ignored.
 *  - do not allow interrupts during the flush!
 */
#include "rpi.h"
#include "neopixel.h"
#include "WS2812B.h"

struct neo_pixel {
    uint8_t r,g,b;
};

// you can add other information to this if you want: this gives us a way to 
// control multiple light arrays concurrently, and to use different brands.
struct neo_handle {
    uint8_t pin;      // output pin
    uint32_t npixel;  // number of pixesl.

    // struct hack
    struct neo_pixel pixels[];
};

void neopix_flush(neo_t h) { 
    // needs the rest of the code.
    // 1. write out the pixel values in the array
    for (unsigned i = 0; i < h->npixel; i++) {
        pix_sendpixel(h->pin, h->pixels[i].r, h->pixels[i].g, h->pixels[i].b);
    }
    // 2. do a flush
    pix_flush(h->pin);
    // 3. Only then set the array to all 0s.
    for (unsigned i = 0; i < h->npixel; i++) {
        h->pixels[i].r = 0;
        h->pixels[i].g = 0;
        h->pixels[i].b = 0;
    }
}

// void neopix_fancy_set(neo_t h, unsigned neopix_idx) {
//     // 1. write out the pixel values in the array
//     unsigned r, g, b;
//     r = g = b = 0x80;
//     for (unsigned i = 0; i < neopix_idx; i++) {
//         pix_sendpixel(h->pin, r,g,b) ;  // 0x40 + 8 * i, 0x40 + 8 * i, 0x40 + 8 * i
//     }
//     // 2. do a flush
//     pix_flush(h->pin);
//     // // 3. Only then set the array to all 0s.
//     for (unsigned i = neo_pix; i < h->npixel; i++) {
//         h->pixels[i].r = 0;
//         h->pixels[i].g = 0;
//         h->pixels[i].b = 0;
//     }
// }

void neopix_flush_up_to_keep(neo_t h, unsigned neopix_idx) {
        // needs the rest of the code.
    // 1. write out the pixel values in the array
    for (unsigned i = 0; i < h->npixel; i++) {
        pix_sendpixel(h->pin, h->pixels[i].r, h->pixels[i].g, h->pixels[i].b);
    }
    // 2. do a flush
    pix_flush(h->pin);
    // 3. Only then set the array to all 0s.
    for (unsigned i = neopix_idx; i < h->npixel; i++) {
        h->pixels[i].r = 0;
        h->pixels[i].g = 0;
        h->pixels[i].b = 0;
    }
}

neo_t neopix_init(uint8_t pin, unsigned npixel) {
    neo_t h;
    unsigned nbytes = sizeof *h + sizeof h->pixels[0] * npixel;
    h = (void*)kmalloc(sizeof *h + sizeof h->pixels[0] * npixel);
    memset(h, 0, nbytes); 
    h->npixel = npixel;
    h->pin = pin;
    gpio_set_output(pin);

    return h;
}

void neopix_dump(neo_t h) {
    output("num pixels=%d\n", h->npixel);
    output("neo pin=%d\n", h->pin);
    for(int i = 0; i < h->npixel; i++) {
        output("%x", h->pixels[i]);
    }
    output("\n\n");
}

// this just makes the definition visible in other modules.
void neopix_sendpixel(neo_t h, uint8_t r, uint8_t g, uint8_t b) {
    pix_sendpixel(h->pin, r,g,b);
}

// doesn't need to be here.
void neopix_fast_clear(neo_t h, unsigned n) {
    for(int i = 0; i < n; i++)
        pix_sendpixel(h->pin, 0,0,0);
    neopix_flush(h);
}

void neopix_clear(neo_t h) {
    neopix_fast_clear(h, h->npixel);
}

// set pixel <pos> in <h> to {r,g,b}
void neopix_write(neo_t h, uint32_t pos, uint8_t r, uint8_t g, uint8_t b) {
    // update the state of neo (the pixel array) and call write   
    // if the position passed in (pos) is out of bounds, just return.
    if (pos > h->npixel || pos < 0) return;
    h->pixels[pos].r = r;
    h->pixels[pos].g = g;
    h->pixels[pos].b = b;
}
