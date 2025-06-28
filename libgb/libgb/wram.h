#ifndef __LIBGB_WRAM__
#define __LIBGB_WRAM__ 1

#include <stdbool.h>
#include <stdint.h>

struct __GBGameboy;

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
    bool (*install)(void *wram, struct __GBGameboy *gameboy);

    void (*write)(struct __GBHighRAM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBHighRAM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    uint8_t memory[kGBHighRAMSize];
} GBHighRAM;

GBHighRAM *GBHighRAMCreate(void);
void GBHighRAMDestroy(GBHighRAM *this);

void __GBHighRAMWrite(GBHighRAM *this, uint16_t address, uint8_t byte);
uint8_t __GBHighRAMRead(GBHighRAM *this, uint16_t address);

#pragma mark - Work RAM

typedef struct __GBWorkRAM {
    bool (*install)(struct __GBWorkRAM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBWorkRAM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBWorkRAM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    uint8_t memory[kGBWorkRAMSize];
    GBHighRAM *hram;
} GBWorkRAM;

GBWorkRAM *GBWorkRAMCreate(void);
void GBWorkRAMDestroy(GBWorkRAM *this);

void __GBWorkRAMWrite(GBWorkRAM *this, uint16_t address, uint8_t byte);
uint8_t __GBWorkRAMRead(GBWorkRAM *this, uint16_t address);

bool __GBWorkRAMOnInstall(GBWorkRAM *this, struct __GBGameboy *gameboy);

#endif /* !defined(__LIBGB_WRAM__) */
