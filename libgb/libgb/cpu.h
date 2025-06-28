#ifndef __LIBGB_CPU__
#define __LIBGB_CPU__ 1

#include <stdbool.h>
#include <stdint.h>

#include <libgb/mmu.h>
#include <libgb/ic.h>

#define GBRegisterPair(h, l)        \
    union {                         \
        struct { uint8_t l, h; };   \
        uint16_t h ## l;            \
    }

struct __GBProcessor;

enum {
    kGBProcessorModeHalted      = -2,
    kGBProcessorModeStopped     = -1,
    kGBProcessorModeOff         =  0,
    kGBProcessorModeFetch       =  1,
    kGBProcessorModePrefix      =  2,
    kGBProcessorModeInterrupted =  3,
    kGBProcessorModeStalled     =  4,
    kGBProcessorModeRun         =  5,
    kGBProcessorModeWait1       =  6,
    kGBProcessorModeWait2       =  7,
    kGBProcessorModeWait3       =  8,
    kGBProcessorModeWait4       =  9
};

typedef struct __GBProcessorOP {
    const char *name;
    uint8_t value;

    void (*go)(struct __GBProcessorOP *this, struct __GBProcessor *cpu);
} GBProcessorOP;

typedef struct __GBProcessor {
    GBInterruptController *ic;
    GBMemoryManager *mmu;

    struct __GBProcessorState {
        bool enableIME;
        bool ime;
        uint8_t a;

        GBRegisterPair(b, c);
        GBRegisterPair(d, e);
        GBRegisterPair(h, l);

        uint16_t sp, pc;

        union {
            struct {
                uint8_t ext:4;
                uint8_t c:1;
                uint8_t h:1;
                uint8_t n:1;
                uint8_t z:1;
            };

            uint8_t reg;
        } f;

        uint16_t mar;
        uint8_t mdr;
        bool accessed;

        uint16_t data; // misc. data for some instructions
        bool prefix;
        uint8_t op;

        int8_t mode;
        bool bug;
    } state;

    GBProcessorOP *decode_prefix[0x100];
    GBProcessorOP *decode[0x100];

    void (*tick)(struct __GBProcessor *this, uint64_t tick);
} GBProcessor;

GBProcessor *GBProcessorCreate(void);
void GBDispatchOP(GBProcessor *this);
void GBProcessorDestroy(GBProcessor *this);

void __GBProcessorTick(GBProcessor *this, uint64_t tick);

typedef struct __GBProcessorState GBProcessorState;

#endif /* !defined(__LIBGB_CPU__) */

// To add: interrupt controller, high memory
