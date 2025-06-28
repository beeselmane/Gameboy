#ifndef __LIBGB__
#define __LIBGB__ 1

#include <libgb/apu.h>
#include <libgb/bios.h>
#include <libgb/cart.h>
#include <libgb/cpu.h>
#include <libgb/dma.h>
#include <libgb/mmio.h>
#include <libgb/mmu.h>
#include <libgb/lcd.h>
#include <libgb/wram.h>
#include <libgb/clock.h>
#include <libgb/gamepad.h>

// 0xFF00 --> input status
//

// WRAM, VRAM
// PPU, APU

// buttons
// link cable? not really necessary tho...

typedef struct __GBGameboy {
    GBProcessor *cpu;
    GBIOMapper *mmio;
    GBWorkRAM *wram;
    GBVideoRAM *vram;
    GBGraphicsDriver *driver;
    GBGamepad *gamepad;
    GBDMARegister *dma;
    GBClock *clock;

    GBCartridge *cart;
    bool cartInstalled;

    GBBIOSROM *bios;
    bool biosInstalled;
} GBGameboy;

GBGameboy *GBGameboyCreate(void);

bool GBGameboyIsPoweredOn(GBGameboy *this);
void GBGameboyPowerOff(GBGameboy *this);
void GBGameboyPowerOn(GBGameboy *this);

void GBGameboyInstallBIOS(GBGameboy *this, GBBIOSROM *bios);
void GBGameboyMaskBIOS(GBGameboy *this, GBBIOSROM *bios);

bool GBGameboyInsertCartridge(GBGameboy *this, GBCartridge *cart);
bool GBGameboyEjectCartridge(GBGameboy *this, GBCartridge *cart);

#endif /* !defined(__LIBGB__) */

// "internal" clock will run at 4 khz
// CPU will tick once per clock (and finish an instruction ~every 4 clocks)
// Memory will tick every 4 clocks. This will push a single pending read/write (cpu shouldn't req. more than 1 access per tick
// PPU will tick every 456 clocks. It will draw a singular scanline on each tick. (scanlines 144 --> 153 should be vblank. scanline == 144 requests vblank interrupt)
// I don't know when APU will tick...
