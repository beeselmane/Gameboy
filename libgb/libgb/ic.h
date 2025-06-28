#ifndef __LIBGB_IC__
#define __LIBGB_IC__ 1

#include <stdbool.h>
#include <stdint.h>

#define kGBInterruptCount               5

#define kGBInterruptVBlank              0
#define kGBInterruptLCDStat             1
#define kGBInterruptTimer               2
#define kGBInterruptSerial              3
#define kGBInterruptJoypad              4

#define kGBInterruptVBlankAddress       0x40
#define kGBInterruptLCDStatAddress      0x48
#define kGBInterruptTimerAddress        0x50
#define kGBInterruptSerialAddress       0x58
#define kGBInterruptJoypadAddress       0x60

#define kGBInterruptFlagAddress         0xFF0F

struct __GBProcessor;
struct __GBGameboy;

typedef struct __GBInterruptFlagPort {
    uint16_t address;

    void (*write)(struct __GBInterruptFlagPort *this, uint8_t byte);
    uint8_t (*read)(struct __GBInterruptFlagPort *this);

    uint8_t value;
} GBInterruptFlagPort;

GBInterruptFlagPort *GBInterruptFlagPortCreate(void);
void __GBInterruptFlagPortWrite(GBInterruptFlagPort *port, uint8_t byte);

typedef struct __GBInterruptController {
    bool (*install)(struct __GBInterruptController *this, struct __GBGameboy *gameboy);

    GBInterruptFlagPort *interruptFlagPort;
    uint8_t interruptControl;

    bool interruptPending;
    uint16_t destination;
    uint8_t interrupt;

    uint16_t lookup[kGBInterruptCount];

    struct __GBProcessor *cpu;
    void (*tick)(struct __GBInterruptController *this, uint64_t tick);
} GBInterruptController;

GBInterruptController *GBInterruptControllerCreate(struct __GBProcessor *cpu);
bool GBInterruptControllerCheck(GBInterruptController *this); // Secondary Check
void GBInterruptControllerReset(GBInterruptController *this);
void GBInterruptControllerDestroy(GBInterruptController *this);

bool __GBInterruptControllerInstall(GBInterruptController *this, struct __GBGameboy *gameboy);
void __GBInterruptControllerTick(GBInterruptController *this, uint64_t tick);

#endif /* !defined(__LIBGB_INTR__) */
