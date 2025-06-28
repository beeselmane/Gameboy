#ifndef __LIBGB_DMA__
#define __LIBGB_DMA__ 1

#include <stdbool.h>
#include <stdint.h>

#define kGBDMARegisterAddress       0xFF46
#define kGBDMARegisterInitClocks    4
#define kGBDMARegisterTotalClocks   (160 * 4) + kGBDMARegisterInitClocks

struct __GBProcessor;
struct __GBGameboy;

typedef struct  __GBDMARegister {
    uint16_t address; // 0xFF46

    void (*write)(struct __GBDMARegister *this, uint8_t byte);
    uint8_t (*read)(struct __GBDMARegister *this);

    uint8_t value;
    bool inProgress;

    uint16_t startAddress;
    uint8_t offset;

    uint16_t ticks;
    uint8_t byte;

    bool (*install)(struct __GBDMARegister *this, struct __GBGameboy *gameboy);
    void (*tick)(struct __GBDMARegister *this, uint64_t ticks);

    // For CPU state and MMU
    struct __GBProcessor *cpu;
} GBDMARegister;

GBDMARegister *GBDMARegisterCreate(void);
void GBDMARegisterDestroy(GBDMARegister *this);

void __GBDMARegisterWrite(GBDMARegister *this, uint8_t byte);

bool __GBDMARegisterInstall(GBDMARegister *this, struct __GBGameboy *gameboy);
void __GBDMARegisterTick(GBDMARegister *this, uint64_t ticks);

#endif /* !defined(__LIBGB_DMA__) */
