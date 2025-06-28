#ifndef __LIBGB_MMU__
#define __LIBGB_MMU__ 1

#include <libgb/mmio.h>
#include <libgb/wram.h>

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

// FE00
// FEA0
// FF00
// FF80

#define kGBMemoryBankSize 0x1000
#define kGBMemoryBankMask 0xF000

struct __GBGameboy;

typedef struct __GBMemorySpace {
    bool (*install)(struct __GBMemorySpace *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBMemorySpace *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBMemorySpace *this, uint16_t address);

    uint16_t start;
    uint16_t end;
} GBMemorySpace;

void __GBMemorySpaceNullWrite(GBMemorySpace *space, uint16_t address, uint8_t byte);
uint8_t __GBMemorySpaceNullRead(GBMemorySpace *space, uint16_t address);

typedef struct __GBMemoryManager {
    void *install;

    GBMemorySpace *highSpaces[0x10];
    GBMemorySpace *spaces[0x10];

    GBMemorySpace *romSpace;
    bool *romMasked;

    uint8_t *interruptControl;
    bool *dma;

    uint16_t *mar;
    uint8_t *mdr;
    bool *accessed;
    bool isWrite;

    void (*tick)(struct __GBMemoryManager *this, uint64_t tick);
} GBMemoryManager;

GBMemoryManager *GBMemoryManagerCreate(void);
bool GBMemoryManagerInstallSpace(GBMemoryManager *this, GBMemorySpace *space);
void GBMemoryManagerDestroy(GBMemoryManager *this);

// Call through to proper space. Access will take place on memory line tick.
void GBMemoryManagerWriteRequest(GBMemoryManager *this, uint16_t *mar, uint8_t *mdr, bool *accessed);
void GBMemoryManagerReadRequest(GBMemoryManager *this, uint16_t *mar, uint8_t *mdr, bool *accessed);

// Directly accesses memory. For use in tick().
void __GBMemoryManagerWrite(GBMemoryManager *this, uint16_t address, uint8_t byte);
uint8_t __GBMemoryManagerRead(GBMemoryManager *this, uint16_t address);

void __GBMemoryManagerTick(GBMemoryManager *this, uint64_t tick);

extern GBMemorySpace *gGBMemorySpaceNull;

#endif /* !defined(__LIBGB_MMU__) */
