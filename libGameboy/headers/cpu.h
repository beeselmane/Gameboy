#ifndef __gameboy_cpu__
#define __gameboy_cpu__ 1

#include "base.h"
#include "mmu.h"

#define GBRegisterPair(h, l)    \
    union {                     \
        struct                  \
        {                       \
            UInt8 h;            \
            UInt8 l;            \
        };                      \
                                \
        UInt16 val;             \
    } h ## l

struct __GBProcessor;

enum {
    kGBProcessorStateFetchStopped = 0
};

typedef struct __GBProcessorOP {
    const char *name;
    UInt8 value;

    void (*go)(struct __GBProcessorOP *this, struct __GBProcessor *cpu);
} GBProcessorOP;

typedef struct __GBProcessor {
    GBMemoryManager *mmu;

    struct __GBProcessorState {
        bool ime;
        UInt8 a;

        GBRegisterPair(b, c);
        GBRegisterPair(d, e);
        GBRegisterPair(h, l);

        UInt16 sp, pc;

        union {
            struct {
                UInt8 z:1;
                UInt8 n:1;
                UInt8 h:1;
                UInt8 c:1;
                UInt8 ext:4;
            } bit;
            UInt8 reg;
        } f;

        UInt16 mar;
        UInt16 mdr;
        bool accessed;
    } state;

    GBProcessorOP *decode_prefix[0xFF];
    GBProcessorOP *decode[0xFF];

    void (*tick)(struct __GBProcessor *this);

    void (**nextOP)(struct __GBProcessor *this);

    bool shouldJump;
    // effective we have these operations:
    // load, math (+/-), store (backward load), jump, halt
} GBProcessor;

typedef struct __GBProcessorState GBProcessorState;

#endif /* __gameboy_cpu__ */

// To add: interrupt controller, high memory
