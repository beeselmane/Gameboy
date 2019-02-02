#ifndef __gameboy_mmu__
#define __gameboy_mmu__ 1

#include "base.h"
#include "mmio.h"
#include "wram.h"

// Note: There isn't really an mmu in a real gameboy.
// addresses are calculated internally without the flexibility an mmu would provide
// I tend to find this architecture cleaner, so I chose to give it some flexibility.
// This also allows us to "change" game without reinitializing everything.
// Just unmap the cart memory, load new cart memory, reinstall ROM, and rst 00h

// Having an emulated 'mmu' also makes timekeeping nice. This way, we can tick the mmu and have it access memory on time.
// Otherwise, we'd have to tick all blocks separately, and ensure each memory runs on time separately.
// Here, we pull timing up and syncronise it globally rather than locally.

// 0x0000 --> 0x0100 (bios) [masks cart]
// 0x0000 --> 0x7FFF (rom)
// 0x8000 --> 0x9FFF (vram)
// 0xA000 --> 0xBFFF (xram) [paged]
// 0xC000 --> 0xDFFF (wram)
// 0xE000 --> 0xFDFF (mirror)
// 0xFE00 --> 0xFE9F (oam)
// 0xFEA0 --> 0xFEFF (xxx)
// 0xFF00 --> 0xFF7F (mmio)
// 0xFF80 --> 0xFFFE (hram)
// 0xFFFF --- interrupt control

#define kGBMemoryBankSize 0x1000
#define kGBMemoryBankMask 0xF000

typedef struct __GBMemorySpace {
    bool (*install)(struct __GBMemorySpace *this, GBGameboy *gameboy);

    void (*write)(struct __GBMemorySpace *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBMemorySpace *this, UInt16 address);

    UInt16 start;
    UInt16 end;
} GBMemorySpace;

void __GBMemorySpaceNullWrite(GBMemorySpace *space, UInt16 address, UInt8 byte);
UInt8 __GBMemorySpaceNullRead(GBMemorySpace *space, UInt16 address);

typedef struct __GBMemoryManager {
    void *install;

    GBMemorySpace *highSpaces[0x10];
    GBMemorySpace *spaces[0x10];

    GBMemorySpace *romSpace;
    bool *romMasked;

    UInt8 *interruptControl;

    UInt16 *mar;
    UInt8 *mdr;
    bool *accessed;
    bool isWrite;

    void (*tick)(struct __GBMemoryManager *this);
} GBMemoryManager;

GBMemoryManager *GBMemoryManagerCreate(void);
bool GBMemoryManagerInstallSpace(GBMemoryManager *this, GBMemorySpace *space);
void GBMemoryManagerDestroy(GBMemoryManager *this);

// Call through to proper space. Access will take place on memory line tick.
void GBMemoryManagerWriteRequest(GBMemoryManager *this, UInt16 *mar, UInt8 *mdr, bool *accessed);
void GBMemoryManagerReadRequest(GBMemoryManager *this, UInt16 *mar, UInt8 *mdr, bool *accessed);

// Directly accesses memory. For use in tick().
void __GBMemoryManagerWrite(GBMemoryManager *this, UInt16 address, UInt8 byte);
UInt8 __GBMemoryManagerRead(GBMemoryManager *this, UInt16 address);

void __GBMemoryManagerTick(GBMemoryManager *this);

extern GBMemorySpace *gGBMemorySpaceNull;

#endif /* __gameboy_mmu__ */
