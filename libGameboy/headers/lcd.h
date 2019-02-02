#ifndef __gameboy_ppu__
#define __gameboy_ppu__ 1

#include "base.h"

// Here, we implement various pieces of the graphics subsystem.
// The gameboy doesn't have a proper GPU, rather, it implements graphics processing across a few separate components.
// Most of these reside on the CPU main chip, but we split them for simplicitly and modularity.
// There are basically just a few blocks of memory and some control registers to control the screen.
// The gameboy then draws to screen through a small LCD driver external to the main chip.
// We implement this same architecture, except the graphics driver simply draws to an RGB pixel buffer.

#pragma mark - Video RAM

#define kGBVideoRAMStart        0x8000
#define kGBVideoRAMEnd          0x9FFF
#define kGBVideoRAMSize         0x2000

struct __GBGameboy;

typedef struct __GBVideoRAM {
    bool (*install)(struct __GBVideoRAM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBVideoRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBVideoRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 memory[kGBVideoRAMSize];
} GBVideoRAM;

GBVideoRAM *GBVideoRAMCreate(void);
void GBVideoRAMDestroy(GBVideoRAM *this);

void __GBVideoRAMWrite(GBVideoRAM *this, UInt16 address, UInt8 byte);
UInt8 __GBVideoRAMRead(GBVideoRAM *this, UInt16 address);

bool __GBVideoRAMOnInstall(GBVideoRAM *this, struct __GBGameboy *gameboy);

#pragma mark - Sprite RAM

#define kGBSpriteRAMStart       0xFE00
#define kGBSpriteRAMEnd         0xFE9F
#define kGBSpriteRAMSize        0xA0

typedef struct __attribute__((packed)) {
    UInt8 y;
    UInt8 x;
    UInt8 pattern;
    UInt8 attributes;
} GBSpriteDescriptor;

typedef struct __GBSpriteRAM {
    bool (*install)(struct __GBSpriteRAM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBSpriteRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBSpriteRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    GBSpriteDescriptor memory[40];
} GBSpriteRAM;

GBSpriteRAM *GBSpriteRAMCreate(void);
void GBSpriteRAMDestroy(GBSpriteRAM *this);

void __GBSpriteRAMWrite(GBSpriteRAM *this, UInt16 address, UInt8 byte);
UInt8 __GBSpriteRAMRead(GBSpriteRAM *this, UInt16 address);

bool __GBSpriteRAMOnInstall(GBSpriteRAM *this, struct __GBGameboy *gameboy);

#pragma mark - LCD Driver

typedef struct __GBGraphicsDriver {
    bool (*install)(struct __GBGraphicsDriver *this, struct __GBGameboy *gameboy);

    GBVideoRAM *vram;
    GBSpriteRAM *oam;
} GBGraphicsDriver;

GBGraphicsDriver *GBGraphicsDriverCreate(void);
void GBGraphicsDriverDestroy(GBGraphicsDriver *this);

bool __GBGraphicsDriverInstall(GBGraphicsDriver *this, struct __GBGameboy *gameboy);

#endif /* __gameboy_ppu__ */
