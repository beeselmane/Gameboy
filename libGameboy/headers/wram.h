#ifndef __gameboy_wram__
#define __gameboy_wram__ 1

#include "base.h"

#define kGBWorkRAMStart     0xC000
#define kGBWorkRAMEnd       0xDFFF
#define kGBWorkRAMSize      0x2000

// 0xFF80 --> 0xFFFE (hram)

typedef struct __GBWorkRAM {
    bool (*install)(struct __GBWorkRAM *this, GBGameboy *gameboy);

    void (*write)(struct __GBWorkRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBWorkRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 memory[kGBWorkRAMSize];
} GBWorkRAM;

GBWorkRAM *GBWorkRAMCreate(void);
void GBWorkRAMDestroy(GBWorkRAM *this);

void __GBWorkRAMWrite(GBWorkRAM *this, UInt16 address, UInt8 byte);
UInt8 __GBWorkRAMRead(GBWorkRAM *this, UInt16 address);

bool __GBWorkRAMOnInstall(GBWorkRAM *this, GBGameboy *gameboy);

// void (*tick)(struct __GBWorkRAM *this);

#endif /* __gameboy_wram__ */
