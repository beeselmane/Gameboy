#ifndef __gameboy_dma__
#define __gameboy_dma__ 1

#include "base.h"

#define kGBDMARegisterAddress       0xFF46
#define kGBDMARegisterInitClocks    4
#define kGBDMARegisterTotalClocks   (160 * 4) + kGBDMARegisterInitClocks

struct __GBProcessor;
struct __GBGameboy;

typedef struct  __GBDMARegister {
    UInt16 address; // 0xFF46

    void (*write)(struct __GBDMARegister *this, UInt8 byte);
    UInt8 (*read)(struct __GBDMARegister *this);

    UInt8 value;
    bool inProgress;

    UInt16 startAddress;
    UInt8 offset;

    UInt16 ticks;
    UInt8 byte;

    bool (*install)(struct __GBDMARegister *this, struct __GBGameboy *gameboy);
    void (*tick)(struct __GBDMARegister *this, UInt64 ticks);

    // For CPU state and MMU
    struct __GBProcessor *cpu;
} GBDMARegister;

GBDMARegister *GBDMARegisterCreate(void);
void GBDMARegisterDestroy(GBDMARegister *this);

void __GBDMARegisterWrite(GBDMARegister *this, UInt8 byte);

bool __GBDMARegisterInstall(GBDMARegister *this, struct __GBGameboy *gameboy);
void __GBDMARegisterTick(GBDMARegister *this, UInt64 ticks);

#endif /* !defined(__gameboy_dma__) */
