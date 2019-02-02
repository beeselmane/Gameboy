#ifndef __gameboy_bios__
#define __gameboy_bios__ 1

#include "base.h"

#define kGBBIOSMaskPortAddress  0xFF50
#define kGBBIOSROMSize          0x0100

struct __GBBIOSROM;

typedef struct __GBBIOSMaskPort {
    UInt16 address; // 0xFF50

    void (*write)(struct __GBBIOSMaskPort *this, UInt8 byte);
    UInt8 (*read)(struct __GBBIOSMaskPort *this);

    UInt8 value;
    struct __GBBIOSROM *bios;
} GBBIOSMaskPort;

GBBIOSMaskPort *GBBIOSMaskPortCreate(struct __GBBIOSROM *bios);
void __GBBIOSMaskPortWrite(GBBIOSMaskPort *port, UInt8 byte);
UInt8 __GBBIOSMaskPortRead(GBBIOSMaskPort *port);

typedef struct __GBBIOSROM {
    bool (*install)(struct __GBBIOSROM *this, GBGameboy *gameboy);

    void (*write)(struct __GBBIOSROM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBBIOSROM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    bool masked;
    GBBIOSMaskPort *port;
    UInt8 data[kGBBIOSROMSize];
} GBBIOSROM;

GBBIOSROM *GBBIOSROMCreate(UInt8 data[kGBBIOSROMSize]);
void GBBIOSROMDestroy(GBBIOSROM *this);

bool __GBBIOSROMOnInstall(GBBIOSROM *this, GBGameboy *gameboy);

void __GBBIOSROMWrite(GBBIOSROM *this, UInt16 address, UInt8 byte);
UInt8 __GBBIOSROMRead(GBBIOSROM *this, UInt16 address);

#endif /* __gameboy_bios__ */
