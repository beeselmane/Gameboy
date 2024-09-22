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

#define kGBCartHeaderStart          0x0100

typedef struct {
    UInt8 entry[3];
    UInt8 logo[48];
    UInt8 title[11];
    UInt8 maker[4];
    UInt8 cgb;
    UInt8 newLicenseCode[2];
    UInt8 sgb;
    UInt8 type;
    UInt8 romSize;
    UInt8 ramSize;
    UInt8 destination;
    UInt8 licenseCode;
    UInt8 version;
    UInt8 headerChecksum;
    UInt16 checksum;
} GBCartHeader;

#pragma mark - ROM

typedef struct __GBCartROM {
    bool (*install)(struct __GBCartROM *this, GBGameboy *gameboy);

    void (*write)(struct __GBCartROM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBCartROM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 maxBank;
    UInt8 *romData;
    UInt8 bank;

    // Some MBCs have the ability to enable/disable their on-cart (built in) RAM
    bool *ramEnable;

    bool installed;
    bool (*eject)(struct __GBCartROM *this, GBGameboy *gameboy);
} GBCartROM;

GBCartROM *GBCartROMCreateGeneric(UInt8 *romData, UInt8 banks, UInt8 (*readFunc)(struct __GBCartROM *this, UInt16 address), void (*writeFunc)(struct __GBCartROM *this, UInt16 address, UInt8 byte));

GBCartROM *GBCartROMCreateWithNullMapper(UInt8 *romData, UInt8 banks);
GBCartROM *GBCartROMCreateWithMBC1(UInt8 *romData, UInt8 banks);
GBCartROM *GBCartROMCreateWithMBC2(UInt8 *romData, UInt8 banks);
GBCartROM *GBCartROMCreateWithMBC3(UInt8 *romData, UInt8 banks);
GBCartROM *GBCartROMCreateWithMBC4(UInt8 *romData, UInt8 banks);
GBCartROM *GBCartROMCreateWithMBC5(UInt8 *romData, UInt8 banks);
void GBCartROMDestroy(GBCartROM *this);

void GBCartROMWriteNull(GBCartROM *this, UInt16 address, UInt8 byte);
void GBCartROMWriteMBC1(GBCartROM *this, UInt16 address, UInt8 byte);
UInt8 GBCartROMReadBanked(GBCartROM *this, UInt16 address);

bool GBCartROMOnInstall(GBCartROM *this, GBGameboy *gameboy);
bool GBCartROMOnEject(GBCartROM *this, GBGameboy *gameboy);

#pragma mark - RAM

typedef struct __GBCartRAM {
    bool (*install)(struct __GBCartRAM *this, GBGameboy *gameboy);

    void (*write)(struct __GBCartRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBCartRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 maxBank;
    UInt8 *ramData;
    UInt8 bank;

    bool enabled;
    bool installed;
    bool (*eject)(struct __GBCartRAM *this, GBGameboy *gameboy);
} GBCartRAM;

GBCartRAM *GBCartRAMCreateWithBanks(UInt8 banks);
void GBCartRAMDestroy(GBCartRAM *this);

void GBCartRAMWriteDirect(GBCartRAM *this, UInt16 address, UInt8 byte);
UInt8 GBCartRAMReadDirect(GBCartRAM *this, UInt16 address);

bool GBCartRAMOnInstall(GBCartRAM *this, GBGameboy *gameboy);
bool GBCartRAMOnEject(GBCartRAM *this, GBGameboy *gameboy);

#pragma mark - Cartridge Info

enum {
    kGBCartMBCTypeNone,
    kGBCartMBCType1,
    kGBCartMBCType2,
    kGBCartMBCType3,
    kGBCartMBCType4,
    kGBCartMBCType5,
    kGBCartMBCTypeMMM01
};

typedef struct {
    UInt8 title[17];
    UInt8 maker[4];
    UInt8 licensee[2];
    UInt8 mbcType;
    UInt32 romSize;
    UInt32 ramSize;
    UInt8 romVersion;
    UInt16 romChecksum;

    bool hasBattery;
    bool hasRumble;
    bool hasTimer;

    bool isJapanese;
} GBCartInfo;

GBCartInfo *GBCartInfoCreateWithHeader(GBCartHeader *header, bool validate);
void GBCartInfoDestroy(GBCartInfo *this);

#pragma mark - Cartridge

typedef struct __GBCartridge {
    GBCartInfo *info;

    GBCartROM *rom;
    GBCartRAM *ram;

    bool installed;
} GBCartridge;

GBCartridge *GBCartridgeCreate(UInt8 *romData, UInt32 romSize);
GBCartHeader *GBCartridgeGetHeader(GBCartridge *cart);

bool GBCartridgeChecksumIsValid(GBCartridge *this);
void GBCartridgeDestroy(GBCartridge *this);

bool GBCartridgeUnmap(GBCartridge *this, GBGameboy *gameboy);
bool GBCartridgeMap(GBCartridge *this, GBGameboy *gameboy);

bool GBCartMemGenericOnEject(GBMemorySpace *this, GBGameboy *gameboy);

#endif /* __gameboy_cart__ */
