#ifndef __LIBGB_CART__
#define __LIBGB_CART__ 1

#include <libgb/mmu.h>

#include <stdbool.h>
#include <stdint.h>

struct __GBGameboy;

#define kGBCartROMBankLowStart      0x0000
#define kGBCartROMBankLowEnd        0x3FFF

#define kGBCartROMBankHighStart     0x4000
#define kGBCartROMBankHighEnd       0x7FFF

#define kGBCartRAMBankStart         0xA000
#define kGBCartRAMBankEnd           0xBFFF

#define kGBCartHeaderStart          0x0100

typedef struct {
    uint8_t entry[3];
    uint8_t logo[48];
    uint8_t title[11];
    uint8_t maker[4];
    uint8_t cgb;
    uint8_t newLicenseCode[2];
    uint8_t sgb;
    uint8_t type;
    uint8_t romSize;
    uint8_t ramSize;
    uint8_t destination;
    uint8_t licenseCode;
    uint8_t version;
    uint8_t headerChecksum;
    uint8_t checksum[2];
} GBCartHeader;

#pragma mark - ROM

typedef struct __GBCartROM {
    bool (*install)(struct __GBCartROM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBCartROM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBCartROM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    uint8_t maxBank;
    uint8_t *romData;
    uint8_t bank;

    // Some MBCs have the ability to enable/disable their on-cart (built in) RAM
    bool *ramEnable;

    bool installed;
    bool (*eject)(struct __GBCartROM *this, struct __GBGameboy *gameboy);
} GBCartROM;

GBCartROM *GBCartROMCreateGeneric(uint8_t *romData, uint8_t banks, uint8_t (*readFunc)(struct __GBCartROM *this, uint16_t address), void (*writeFunc)(struct __GBCartROM *this, uint16_t address, uint8_t byte));

GBCartROM *GBCartROMCreateWithNullMapper(uint8_t *romData, uint8_t banks);
GBCartROM *GBCartROMCreateWithMBC1(uint8_t *romData, uint8_t banks);
GBCartROM *GBCartROMCreateWithMBC2(uint8_t *romData, uint8_t banks);
GBCartROM *GBCartROMCreateWithMBC3(uint8_t *romData, uint8_t banks);
GBCartROM *GBCartROMCreateWithMBC4(uint8_t *romData, uint8_t banks);
GBCartROM *GBCartROMCreateWithMBC5(uint8_t *romData, uint8_t banks);
void GBCartROMDestroy(GBCartROM *this);

void GBCartROMWriteNull(GBCartROM *this, uint16_t address, uint8_t byte);
void GBCartROMWriteMBC1(GBCartROM *this, uint16_t address, uint8_t byte);
uint8_t GBCartROMReadBanked(GBCartROM *this, uint16_t address);

bool GBCartROMOnInstall(GBCartROM *this, struct __GBGameboy *gameboy);
bool GBCartROMOnEject(GBCartROM *this, struct __GBGameboy *gameboy);

#pragma mark - RAM

typedef struct __GBCartRAM {
    bool (*install)(struct __GBCartRAM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBCartRAM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBCartRAM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    uint8_t maxBank;
    uint8_t *ramData;
    uint8_t bank;

    bool enabled;
    bool installed;
    bool (*eject)(struct __GBCartRAM *this, struct __GBGameboy *gameboy);
} GBCartRAM;

GBCartRAM *GBCartRAMCreateWithBanks(uint8_t banks);
void GBCartRAMDestroy(GBCartRAM *this);

void GBCartRAMWriteDirect(GBCartRAM *this, uint16_t address, uint8_t byte);
uint8_t GBCartRAMReadDirect(GBCartRAM *this, uint16_t address);

bool GBCartRAMOnInstall(GBCartRAM *this, struct __GBGameboy *gameboy);
bool GBCartRAMOnEject(GBCartRAM *this, struct __GBGameboy *gameboy);

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
    uint8_t title[17];
    uint8_t maker[4];
    uint8_t licensee[2];
    uint8_t mbcType;
    uint32_t romSize;
    uint32_t ramSize;
    uint8_t romVersion;
    uint16_t romChecksum;

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

GBCartridge *GBCartridgeCreate(uint8_t *romData, uint32_t romSize);
GBCartHeader *GBCartridgeGetHeader(GBCartridge *cart);

bool GBCartridgeChecksumIsValid(GBCartridge *this);
void GBCartridgeDestroy(GBCartridge *this);

bool GBCartridgeUnmap(GBCartridge *this, struct __GBGameboy *gameboy);
bool GBCartridgeMap(GBCartridge *this, struct __GBGameboy *gameboy);

bool GBCartMemGenericOnEject(GBMemorySpace *this, struct __GBGameboy *gameboy);

#endif /* !defined(__LIBGB_CART__) */
