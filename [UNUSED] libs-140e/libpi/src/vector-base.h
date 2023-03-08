#ifndef __VECTOR_BASE_SET_H__
#define __VECTOR_BASE_SET_H__
#include "libc/bit-support.h"
#include "asm-helpers.h"

/*
 * vector base address register:
 *   3-121 --- let's us control where the exception jump table is!
 *
 * defines: 
 *  - vector_base_set  
 *  - vector_base_get
 */

cp_asm_get(vector_base_asm, p15, 0, c12, c0, 0);
cp_asm_set(vector_base_asm, p15, 0, c12, c0, 0);

// return the current value vector base is set to.
static inline void *vector_base_get(void) {
    uint32_t vector_base_value = vector_base_asm_get();
    return (void *)vector_base_value;
}

// check that not null and alignment is good.
static inline int vector_base_chk(void *vector_base) {
    if (vector_base == NULL)
        return 0;
    if ((((int)vector_base) & (~0b11111)) != (int)vector_base)  // not 32 byte aligned
        return 0;
    return 1;
}

// set vector base: must not have been set already.
static inline void vector_base_set(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    void *v = vector_base_get();
    if(v) 
        panic("vector base register already set=%p\n", v);

    vector_base_asm_set((unsigned)vec);

    v = vector_base_get();
    if(v != vec)
        panic("set vector=%p, but have %p\n", vec, v);
}

// reset vector base and return old value: could have
// been previously set.
static inline void *
vector_base_reset(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    void *old_vec = 0;
    // todo("check that <vec> is good reset vector base");  // I believe this is done already
    // return old value. put in new value.
    old_vec = vector_base_get();
    vector_base_asm_set((unsigned)vec);

    assert(vector_base_get() == vec);
    return old_vec;
}
#endif
