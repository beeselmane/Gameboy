#ifndef __gameboy_cpu__
#define __gameboy_cpu__ 1

#include "base.h"
#include "mmu.h"

#define GBRegisterPair(h, l)    \
    union {                     \
        struct                  \
        {                       \
            UInt8 l;            \
            UInt8 h;            \
        };                      \
                                \
        UInt16 h ## l;          \
    }

struct __GBProcessor;

enum {
    kGBProcessorModeHalted     = -2,
    kGBProcessorModeStopped    = -1,
    kGBProcessorModeOff        =  0,
    kGBProcessorModeFetch      =  1,
    kGBProcessorModePrefix     =  2,
    kGBProcessorModeStalled    =  3,
    kGBProcessorModeRun        =  4,
    kGBProcessorModeWait1      =  5,
    kGBProcessorModeWait2      =  6,
    kGBProcessorModeWait3      =  7,
    kGBProcessorModeWait4      =  8
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
                UInt8 ext:4;
                UInt8 c:1;
                UInt8 h:1;
                UInt8 n:1;
                UInt8 z:1;
            };
            UInt8 reg;
        } f;

        UInt16 mar;
        UInt8 mdr;
        bool accessed;

        UInt8 op;
        bool prefix;

        UInt16 data; // misc. data for some instructions
        SInt8 mode;
    } state;

    GBProcessorOP *decode_prefix[0x100];
    GBProcessorOP *decode[0x100];

    void (*tick)(struct __GBProcessor *this);
} GBProcessor;

GBProcessor *GBProcessorCreate(void);
void GBDispatchOP(GBProcessor *this);
void GBProcessorDestroy(GBProcessor *this);

void __GBProcessorTick(GBProcessor *this);

typedef struct __GBProcessorState GBProcessorState;

#endif /* __gameboy_cpu__ */

// To add: interrupt controller, high memory
