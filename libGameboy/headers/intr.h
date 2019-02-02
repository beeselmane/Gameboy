#ifndef __gameboy_intr__
#define __gameboy_intr__ 1

#include "base.h"

#define kGBInterruptVBlank      0
#define kGBInterruptLCDStat     1
#define kGBInterruptTimer       2
#define kGBInterruptSerial      3
#define kGBInterruptJoypad      4

#define kGBInterruptFlagAddress 0xFF0F

struct __GBProcessor;

typedef struct __GBInterruptFlagPort {
    UInt16 address;

    void (*write)(struct __GBInterruptFlagPort *this, UInt8 byte);
    UInt8 (*read)(struct __GBInterruptFlagPort *this);

    UInt8 interruptFlag;
} GBInterruptFlagPort;

typedef struct __GBInterruptController {
    GBInterruptFlagPort *interruptFlagPort;
    UInt8 interruptControl;

    struct __GBProcessor *cpu;
} GBInterruptController;

GBInterruptController *GBInterruptControllerCreate(struct __GBProcessor *cpu);
void GBInterruptControllerCheckForInterrupts(GBInterruptController *controller);
void GBInterruptControllerDestroy(GBInterruptController *controller);

#endif /* !defined(__gameboy_intr__) */
