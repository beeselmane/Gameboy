#ifndef __gameboy_cart__
#define __gameboy_cart__ 1

#include "base.h"
#include "mmu.h"

#define kGBCartROMBankLowStart      0x0000
#define kGBCartROMBankLowEnd        0x3FFF

#define kGBCartROMBankHighStart     0x4000
#define kGBCartROMBankHighEnd       0x7FFF

#define kGBCartRAMBankStart         0xA000
#define kGBCartRAMBankEnd           0xBFFF

typedef struct __GBCartROM {
    bool (*install)(struct __GBCartROM *this, GBGameboy *gameboy);

    void (*write)(struct __GBCartROM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBCartROM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 maxBank;
    UInt8 *romData;
    UInt8 bank;

    bool installed;
    bool (*eject)(struct __GBCartROM *this, GBGameboy *gameboy);
} GBCartROM;

GBCartROM *GBCartROMCreateWithNullMapper(UInt8 *romData, UInt8 banks);
void GBCartROMDestroy(GBCartROM *this);

void GBCartROMWriteNull(GBCartROM *this, UInt16 address, UInt8 byte);
UInt8 GBCartROMReadDirect(GBCartROM *this, UInt16 address);

bool GBCartROMOnInstall(GBCartROM *this, GBGameboy *gameboy);
bool GBCartROMOnEject(GBCartROM *this, GBGameboy *gameboy);

typedef struct __GBCartRAM {
    bool (*install)(struct __GBCartRAM *this, GBGameboy *gameboy);

    void (*write)(struct __GBCartRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBCartRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 maxBank;
    UInt8 *ramData;
    UInt8 bank;

    bool installed;
    bool (*eject)(struct __GBCartRAM *this, GBGameboy *gameboy);
} GBCartRAM;

GBCartRAM *GBCartRAMCreateWithNullMapper(UInt8 banks);
void GBCartRAMDestroy(GBCartRAM *this);

void GBCartRAMWriteDirect(GBCartRAM *this, UInt16 address, UInt8 byte);
UInt8 GBCartRAMReadDirect(GBCartRAM *this, UInt16 address);

bool GBCartRAMOnInstall(GBCartRAM *this, GBGameboy *gameboy);
bool GBCartRAMOnEject(GBCartRAM *this, GBGameboy *gameboy);

typedef struct __GBCartridge {
    GBCartROM *rom;
    GBCartRAM *ram;

    UInt8 mbcType;
    bool hasRAM;

    bool installed;
} GBCartridge;

GBCartridge *GBCartridgeCreate(UInt8 *romData, UInt32 romSize);
void GBCartridgeDestroy(GBCartridge *this);

bool GBCartridgeInsert(GBCartridge *this, GBGameboy *gameboy);
bool GBCartridgeEject(GBCartridge *this, GBGameboy *gameboy);

bool GBCartMemGenericOnEject(GBMemorySpace *this, GBGameboy *gameboy);

#endif /* __gameboy_cart__ */
