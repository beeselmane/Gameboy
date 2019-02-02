#ifndef __gameboy__
#define __gameboy__ 1

#include "base.h"

#include "apu.h"
#include "bios.h"
#include "cart.h"
#include "cpu.h"
#include "mmio.h"
#include "mmu.h"
#include "lcd.h"
#include "wram.h"

// 0xFF00 --> input status
//

// WRAM, VRAM
// PPU, APU

// buttons
// link cable? not really necessary tho...

struct __GBGameboy {
    GBProcessor *cpu;
    GBIOMapper *mmio;
    GBWorkRAM *wram;

    GBCartridge *cart;
    bool cartInstalled;
};



#endif /* __gameboy__ */

// "internal" clock will run at 4 khz
// CPU will tick once per clock (and finish an instruction ~every 4 clocks)
// Memory will tick every 4 clocks. This will push a single pending read/write (cpu shouldn't req. more than 1 access per tick
// PPU will tick every 456 clocks. It will draw a singular scanline on each tick. (scanlines 144 --> 153 should be vblank. scanline == 144 requests vblank interrupt)
// I don't know when APU will tick...
