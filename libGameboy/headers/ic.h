#ifndef __gameboy_ic__
#define __gameboy_ic__ 1

#include "base.h"

#define kGBInterruptCount               5

#define kGBInterruptVBlank              0
#define kGBInterruptLCDStat             1
#define kGBInterruptTimer               2
#define kGBInterruptSerial              3
#define kGBInterruptJoypad              4

#define kGBInterruptVBlankAddress       0x40
#define kGBInterruptLCDStatAddress      0x48
#define kGBInterruptTimerAddress        0x50
#define kGBInterruptSerialAddress       0x58
#define kGBInterruptJoypadAddress       0x60

#define kGBInterruptFlagAddress         0xFF0F

struct __GBProcessor;
struct __GBGameboy;

typedef struct __GBInterruptFlagPort {
    UInt16 address;

    void (*write)(struct __GBInterruptFlagPort *this, UInt8 byte);
    UInt8 (*read)(struct __GBInterruptFlagPort *this);

    UInt8 value;
} GBInterruptFlagPort;

GBInterruptFlagPort *GBInterruptFlagPortCreate(void);
void __GBInterruptFlagPortWrite(GBInterruptFlagPort *port, UInt8 byte);

typedef struct __GBInterruptController {
    bool (*install)(struct __GBInterruptController *this, struct __GBGameboy *gameboy);

    GBInterruptFlagPort *interruptFlagPort;
    UInt8 interruptControl;

    bool interruptPending;
    UInt16 destination;
    UInt8 interrupt;

    UInt16 lookup[kGBInterruptCount];

    struct __GBProcessor *cpu;
    void (*tick)(struct __GBInterruptController *this, UInt64 tick);
} GBInterruptController;

GBInterruptController *GBInterruptControllerCreate(struct __GBProcessor *cpu);
bool GBInterruptControllerCheck(GBInterruptController *this); // Secondary Check
void GBInterruptControllerReset(GBInterruptController *this);
void GBInterruptControllerDestroy(GBInterruptController *this);

bool __GBInterruptControllerInstall(GBInterruptController *this, struct __GBGameboy *gameboy);
void __GBInterruptControllerTick(GBInterruptController *this, UInt64 tick);

#endif /* !defined(__gameboy_intr__) */
