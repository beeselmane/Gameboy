#ifndef __LIBGB_PPU__
#define __LIBGB_PPU__ 1

#include <stdbool.h>
#include <stdint.h>

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

    void (*write)(struct __GBVideoRAM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBVideoRAM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    uint8_t memory[kGBVideoRAMSize];
    struct __GBGraphicsDriver *driver;
} GBVideoRAM;

GBVideoRAM *GBVideoRAMCreate(void);
void GBVideoRAMDestroy(GBVideoRAM *this);

void __GBVideoRAMWrite(GBVideoRAM *this, uint16_t address, uint8_t byte);
uint8_t __GBVideoRAMRead(GBVideoRAM *this, uint16_t address);

bool __GBVideoRAMOnInstall(GBVideoRAM *this, struct __GBGameboy *gameboy);

#pragma mark - Sprite RAM

#define kGBSpriteRAMStart       0xFE00
#define kGBSpriteRAMEnd         0xFE9F
#define kGBSpriteRAMSize        0xA0

typedef struct __attribute__((packed)) {
    uint8_t y;
    uint8_t x;
    uint8_t pattern;
    uint8_t attributes;
} GBSpriteDescriptor;

typedef struct __GBSpriteRAM {
    bool (*install)(struct __GBSpriteRAM *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBSpriteRAM *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBSpriteRAM *this, uint16_t address);

    uint16_t start;
    uint16_t end;

    GBSpriteDescriptor memory[40];
    struct __GBGraphicsDriver *driver;
} GBSpriteRAM;

GBSpriteRAM *GBSpriteRAMCreate(void);
void GBSpriteRAMDestroy(GBSpriteRAM *this);

void __GBSpriteRAMWrite(GBSpriteRAM *this, uint16_t address, uint8_t byte);
uint8_t __GBSpriteRAMRead(GBSpriteRAM *this, uint16_t address);

bool __GBSpriteRAMOnInstall(GBSpriteRAM *this, struct __GBGameboy *gameboy);

#pragma mark - Ports

#define kGBLCDControlPortAddress        0xFF40
#define kGBLCDStatusPortAddress         0xFF41
#define kGBScrollPortYAddress           0xFF42
#define kGBScrollPortXAddress           0xFF43
#define kGBCoordPortAddress             0xFF44
#define kGBLineComparePortAddress       0xFF45
#define kGBLineWindowPortXAddress       0xFF4A
#define kGBLineWindowPortYAddress       0xFF4B
#define kGBPalettePortBGAddress         0xFF47
#define kGBPalettePortSprite0Address    0xFF48
#define kGBPalettePortSprite1Address    0xFF49

struct __GBCoordPort;

typedef struct __GBLCDControlPort {
    uint16_t address;

    void (*write)(struct __GBLCDControlPort *this, uint8_t byte);
    uint8_t (*read)(struct __GBLCDControlPort *this);

    uint8_t value;
    struct __GBCoordPort *coordPort;
    struct __GBGraphicsDriver *driver;
} GBLCDControlPort;

GBLCDControlPort *GBLCDControlPortCreate(struct __GBCoordPort *coordPort);
void __GBLCDControlPortWrite(GBLCDControlPort *this, uint8_t byte);

typedef struct __GBLCDStatusPort {
    uint16_t address;

    void (*write)(struct __GBLCDStatusPort *this, uint8_t byte);
    uint8_t (*read)(struct __GBLCDStatusPort *this);

    uint8_t value;
} GBLCDStatusPort;

GBLCDStatusPort *GBLCDStatusPortCreate(void);
void __GBLCDStatusPortWrite(GBLCDStatusPort *this, uint8_t byte);

typedef struct __GBCoordPort {
    uint16_t address;

    void (*write)(struct __GBCoordPort *this, uint8_t byte);
    uint8_t (*read)(struct __GBCoordPort *this);

    uint8_t value;
} GBCoordPort;

GBCoordPort *GBCoordPortCreate(void);
void __GBCoordPortWrite(GBCoordPort *this, uint8_t byte);

typedef struct __GBVideoPortGeneric {
    uint16_t address;

    void (*write)(struct __GBVideoPortGeneric *this, uint8_t byte);
    uint8_t (*read)(struct __GBVideoPortGeneric *this);

    uint8_t value;
} GBVideoPortGeneric;

GBVideoPortGeneric *GBVideoPortGenericCreate(uint16_t address);

#pragma mark - LCD Driver

#define kGBColorWhite                   0xFFFFFF
#define kGBColorLightGray               0xBBBBBB
#define kGBColorDarkGray                0x555555
#define kGBColorBlack                   0x000000

// In Pixels
#define kGBFullHeight                   256
#define kGBFullWidth                    256

// In Tiles
#define kGBMapHeight                    32
#define kGBMapWidth                     32

#define kGBTileHeight                   8
#define kGBTileWidth                    8

#define kGBScreenHeight                 144
#define kGBScreenWidth                  160

#define kGBCoordinateMaxY               153

#define kGBDriverSpriteSearchClocks     80
#define kGBDriverHorizonalClocks        376
//#define kGBDriverVerticalClocks         4560
#define kGBDriverVerticalClockUpdate    456

#define kGBVideoInterruptOnLine         (1 << 6)
#define kGBVideoInterruptSpriteSearch   (1 << 5)
#define kGBVideoInterruptVBlank         (1 << 4)
#define kGBVideoInterruptHBlank         (1 << 3)

#define kGBVideoStatMatchFlag           (1 << 2)

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
    void (*tick)(struct __GBGraphicsDriver *this, uint64_t tick);

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

    uint8_t *interruptRequest;
    bool displayOn;

    uint32_t screenData[kGBScreenHeight * kGBScreenWidth];
    uint32_t *linePointer; // Points to the head of the current line while drawing
    uint32_t linePosition; // Offset into current line to place the next pixel

    uint32_t colorLookup[4]; // Map of 'white, light, dark, black' to real RGB values.
    uint8_t nullColor; // The screen buffer is flushed with this value when it is disabled.

    uint8_t fifoBuffer[16]; // Pixel palette values + palette number (low byte is palette, high byte is color index)
    uint8_t fifoPosition; // The next position to draw from when pulling pixel info. Wraps from 15 back to 0.
    uint8_t fifoSize; // The number of pixels left in the FIFO buffer.
    // Note: The state of the FIFO is modulated by monitoring the value of fifoSize above.
    bool drawingWindow; // This is enabled after switching to window to prevent locking up

    uint8_t fetchBuffer[8]; // Stores the next 8 pixels to be pushed into FIFO
    uint8_t fetcherTile; // The index into the tileset of the last fetched tile
    uint8_t fetcherByte0; // The first byte of the last fetched tile
    uint8_t fetcherByte1; // The second byte of the last fetched tile
    uint16_t fetcherBase; // The base address of the fetcher
    uint16_t fetcherPosition; // The position in the map of the start of the current line
    uint8_t fetcherOffset; // The current offset into the current line being rendered
    uint8_t fetcherMode; // The state of memory access for the fetcher.

    // We can only draw 10 sprites per line.
    uint8_t lineSprites[10]; // Index of the sprites to be drawn in the next line
    uint8_t lineSpriteCount; // Number of spites in the next line
    uint8_t spriteIndex; // Iterator for the sprite table in RAM
    uint8_t nextSpriteY; // Y coordinate of the sprite we are checking
    uint8_t nextSpriteX; // X coordinate of the sprite we are checking

    uint16_t driverModeTicks; // Ticks in the current mode
    uint8_t driverMode; // The current driver mode

    uint8_t lineMod8; // Tracks the current line number mod 8. This is used to fetch the right lines of tiles.
    uint8_t driverX; // Track effective position for scrollX and windowX
} GBGraphicsDriver;

GBGraphicsDriver *GBGraphicsDriverCreate(void);
void GBGraphicsDriverDestroy(GBGraphicsDriver *this);

bool __GBGraphicsDriverInstall(GBGraphicsDriver *this, struct __GBGameboy *gameboy);
void __GBGraphicsDriverTick(GBGraphicsDriver *this, uint64_t ticks);

#endif /* !defined(__LIBGB_PPU__) */
