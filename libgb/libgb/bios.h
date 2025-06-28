#ifndef __LIBGB_BIOS__
#define __LIBGB_BIOS__ 1

#include <stdbool.h>
#include <stdint.h>

#define kGBBIOSMaskPortAddress  0xFF50
#define kGBBIOSROMSize          0x0100

struct __GBGameboy;
struct __GBBIOSROM;

typedef struct __GBBIOSMaskPort {
    uint16_t address; // 0xFF50

    void (*write)(struct __GBBIOSMaskPort *this, uint8_t byte);
    uint8_t (*read)(struct __GBBIOSMaskPort *this);

    uint8_t value;
    struct __GBBIOSROM *bios;
} GBBIOSMaskPort;

GBBIOSMaskPort *GBBIOSMaskPortCreate(struct __GBBIOSROM *bios);
void __GBBIOSMaskPortWrite(GBBIOSMaskPort *port, uint8_t byte);
uint8_t __GBBIOSMaskPortRead(GBBIOSMaskPort *port);

typedef struct __GBBIOSROM {
    bool (*install)(struct __GBBIOSROM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBBIOSROM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBBIOSROM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    bool masked;
    GBBIOSMaskPort *port;
    uint8_t data[kGBBIOSROMSize];
} GBBIOSROM;

GBBIOSROM *GBBIOSROMCreate(uint8_t data[kGBBIOSROMSize]);
void GBBIOSROMDestroy(GBBIOSROM *this);

bool __GBBIOSROMOnInstall(GBBIOSROM *this, struct __GBGameboy *gameboy);

void __GBBIOSROMWrite(GBBIOSROM *this, uint16_t address, uint8_t byte);
uint8_t __GBBIOSROMRead(GBBIOSROM *this, uint16_t address);

#endif /* !defined(__LIBGB_BIOS__) */
