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

struct __GBGraphicsDriver;
struct __GBGameboy;

typedef struct __GBVideoRAM {
    bool (*install)(struct __GBVideoRAM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBVideoRAM *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBVideoRAM *this, UInt16 address);

    UInt16 start;
    UInt16 end;

    UInt8 memory[kGBVideoRAMSize];
    struct __GBGraphicsDriver *driver;
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
    struct __GBGraphicsDriver *driver;
} GBSpriteRAM;

GBSpriteRAM *GBSpriteRAMCreate(void);
void GBSpriteRAMDestroy(GBSpriteRAM *this);

void __GBSpriteRAMWrite(GBSpriteRAM *this, UInt16 address, UInt8 byte);
UInt8 __GBSpriteRAMRead(GBSpriteRAM *this, UInt16 address);

bool __GBSpriteRAMOnInstall(GBSpriteRAM *this, struct __GBGameboy *gameboy);

#pragma mark - Ports

#define kGBLCDControlPortAddress        0xFF40
#define kGBLCDStatusPortAddress         0xFF41
#define kGBScrollPortXAddress           0xFF42
#define kGBScrollPortYAddress           0xFF43
#define kGBCoordPortAddress             0xFF44
#define kGBLineComparePortAddress       0xFF45
#define kGBLineWindowPortXAddress       0xFF4A
#define kGBLineWindowPortYAddress       0xFF4B
#define kGBPalettePortBGAddress         0xFF47
#define kGBPalettePortSprite0Address    0xFF48
#define kGBPalettePortSprite1Address    0xFF49

struct __GBCoordPort;

typedef struct __GBLCDControlPort {
    UInt16 address;

    void (*write)(struct __GBLCDControlPort *this, UInt8 byte);
    UInt8 (*read)(struct __GBLCDControlPort *this);

    UInt8 value;
    struct __GBCoordPort *coordPort;
    struct __GBGraphicsDriver *driver;
} GBLCDControlPort;

GBLCDControlPort *GBLCDControlPortCreate(struct __GBCoordPort *coordPort);
void __GBLCDControlPortWrite(GBLCDControlPort *this, UInt8 byte);

typedef struct __GBLCDStatusPort {
    UInt16 address;

    void (*write)(struct __GBLCDStatusPort *this, UInt8 byte);
    UInt8 (*read)(struct __GBLCDStatusPort *this);

    UInt8 value;
} GBLCDStatusPort;

GBLCDStatusPort *GBLCDStatusPortCreate(void);
void __GBLCDStatusPortWrite(GBLCDStatusPort *this, UInt8 byte);

typedef struct __GBCoordPort {
    UInt16 address;

    void (*write)(struct __GBCoordPort *this, UInt8 byte);
    UInt8 (*read)(struct __GBCoordPort *this);

    UInt8 value;
} GBCoordPort;

GBCoordPort *GBCoordPortCreate(void);
void __GBCoordPortWrite(GBCoordPort *this, UInt8 byte);

typedef struct __GBVideoPortGeneric {
    UInt16 address;

    void (*write)(struct __GBVideoPortGeneric *this, UInt8 byte);
    UInt8 (*read)(struct __GBVideoPortGeneric *this);

    UInt8 value;
} GBVideoPortGeneric;

GBVideoPortGeneric *GBVideoPortGenericCreate(UInt16 address);

#pragma mark - LCD Driver

#define kGBColorWhite       0xFFFFFF
#define kGBColorLightGray   0xBBBBBB
#define kGBColorDarkGray    0x555555
#define kGBColorBlack       0x000000

#define kGBFullHeight       256
#define kGBFullWidth        256

#define kGBScreenHeight     144
#define kGBScreenWidth      160

#define kGBCoordinateMaxY   153

// 1 sprite every 2 cycles
#define kGBDriverSpriteSearchClocks     80
// Depends on switches and such... split into fetch/write and hblank
#define kGBDriverHorizonalClocks        376
// Just stall... yey
//#define kGBDriverVerticalClocks         1540
#define kGBDriverVerticalClockUpdate    456

enum {
    kGBDriverStateSpriteSearch      = 2,
    kGBDriverStatePixelTransfer     = 3,
    kGBDriverStateHBlank            = 0,
    kGBDriverStateVBlank            = 1
};

enum {
    kGBFetcherStateFetchTile  = 0,
    kGBFetcherStateFetchByte0 = 1,
    kGBFetcherStateFetchByte1 = 2,
    kGBFetcherStateStall      = 3
};

typedef struct __GBGraphicsDriver {
    bool (*install)(struct __GBGraphicsDriver *this, struct __GBGameboy *gameboy);
    void (*tick)(struct __GBGraphicsDriver *this, UInt64 tick);

    GBLCDControlPort *control;
    GBLCDStatusPort *status;
    GBVideoPortGeneric *scrollX;
    GBVideoPortGeneric *scrollY;
    GBCoordPort *coordinate;
    GBVideoPortGeneric *compare;
    GBVideoPortGeneric *windowX;
    GBVideoPortGeneric *windowY;
    GBVideoPortGeneric *paletteBG;
    GBVideoPortGeneric *paletteSprite0;
    GBVideoPortGeneric *paletteSprite1;

    GBVideoRAM *vram;
    GBSpriteRAM *oam;

    bool displayOn;

    UInt32 screenData[kGBScreenHeight * kGBScreenWidth];
    UInt32 colorLookup[4];

    UInt8 fifoBuffer[16]; // Pixel palette values + palette number (low byte is palette low, high byte is color index)
    UInt8 fifoPosition;
    UInt8 fifoSize;

    UInt8 fetcherTile;
    UInt8 fetcherByte0;
    UInt8 fetcherByte1;
    UInt16 fetcherOffset;
    UInt8 fetcherPosition;
    UInt8 fetcherMode;

    // We can only draw 10 sprites per line.
    UInt8 lineSprites[10];
    UInt8 lineSpriteCount;
    UInt8 spriteIndex;

    UInt16 driverModeTicks;
    UInt8 driverMode;
    UInt8 driverX;
} GBGraphicsDriver;

GBGraphicsDriver *GBGraphicsDriverCreate(void);
void GBGraphicsDriverDestroy(GBGraphicsDriver *this);

bool __GBGraphicsDriverInstall(GBGraphicsDriver *this, struct __GBGameboy *gameboy);
void __GBGraphicsDriverTick(GBGraphicsDriver *this, UInt64 ticks);

#endif /* __gameboy_ppu__ */
