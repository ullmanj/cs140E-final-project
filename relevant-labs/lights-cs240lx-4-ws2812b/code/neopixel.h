// need to pass context around since we can have multiple neopixel strings.
struct neo_handle;
typedef struct neo_handle *neo_t;

// dynamically allocate structure for addressable light strip.
neo_t neopix_init(uint8_t pin, unsigned npixels);
void neopix_dump(neo_t h);

// buffered write {r,g,b} into <h> at position <pos>: last write wins.  
// does not flush out to light array until you call <neopixel_flush>.
//
// ignores if out of bounds (or assert?)
void neopix_write(neo_t h, uint32_t pos, uint8_t r, uint8_t g, uint8_t b);
void neopix_flush(neo_t h);
void neopix_flush_up_to_keep(neo_t h, unsigned neopix_idx);
void neopix_fancy_set(neo_t h, unsigned neopix_idx);

// immediately write black/off (0,0,0) to every pixel upto 
// pixel number <upto>
void neopix_fast_clear(neo_t h, unsigned upto);
// same: but write all pixels in <h>
void neopix_clear(neo_t h);
