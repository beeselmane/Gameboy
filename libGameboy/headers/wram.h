#ifndef __gameboy_wram__
#define __gameboy_wram__ 1

#include "base.h"

#define kGBWorkRAMStart     0xC000
#define kGBWorkRAMEnd       0xDFFF
#define kGBWorkRAMSize      0x2000

#define kGBHighRAMStart     0xFF80
#define kGBHighRAMEnd       0xFFFE
#define kGBHighRAMSize      0x7F

#pragma mark - High RAM
// Note: This should theoretically be coupled with the CPU as it is on a real gameboy.
// This was done for speed, and since it will all be the same using a modern system,
// I've chosen to link hram to wram out of convienience.

typedef struct __GBHighRAM {
    bool (*install)(void *wram, GBGameboy *gameboy);

    void (*write)(struct __GBHighRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBHighRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 memory[kGBHighRAMSize];
} GBHighRAM;

GBHighRAM *GBHighRAMCreate(void);
void GBHighRAMDestroy(GBHighRAM *this);

void __GBHighRAMWrite(GBHighRAM *this, UInt16 address, UInt8 byte);
UInt8 __GBHighRAMRead(GBHighRAM *this, UInt16 address);

#pragma mark - Work RAM

typedef struct __GBWorkRAM {
    bool (*install)(struct __GBWorkRAM *this, GBGameboy *gameboy);

    void (*write)(struct __GBWorkRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBWorkRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 memory[kGBWorkRAMSize];
    GBHighRAM *hram;
} GBWorkRAM;

GBWorkRAM *GBWorkRAMCreate(void);
void GBWorkRAMDestroy(GBWorkRAM *this);

void __GBWorkRAMWrite(GBWorkRAM *this, UInt16 address, UInt8 byte);
UInt8 __GBWorkRAMRead(GBWorkRAM *this, UInt16 address);

bool __GBWorkRAMOnInstall(GBWorkRAM *this, GBGameboy *gameboy);

#endif /* __gameboy_wram__ */
