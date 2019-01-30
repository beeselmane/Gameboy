#ifndef __gameboy_ppu__
#define __gameboy_ppu__ 1

#include "base.h"

// Here, we implement various pieces of the graphics subsystem.
// The gameboy doesn't have a proper GPU, rather, it implements graphics processing across a few separate components.
// Most of these reside on the CPU main chip, but we split them for simplicitly and modularity.
// There are basically just a few blocks of memory and some control registers to control the screen.
// The gameboy then draws to screen through a small LCD driver external to the main chip.
// We implement this same architecture, except the graphics driver simply draws to an RGB pixel buffer.

typedef struct {
    //
} GBVideoRAM;

typedef struct {
    //
} GBSpriteRAM;

typedef struct {
    //
} GBGraphicsDriver;

#endif /* __gameboy_ppu__ */
