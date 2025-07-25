#ifndef __LIBGB_CLOCK__
#define __LIBGB_CLOCK__ 1

#include <stdbool.h>
#include <stdint.h>

struct __GBGameboy;

#pragma mark - Timer Registers

#define kGBDividerAddress       0xFF04
#define kGBTimerAddress         0xFF05
#define kGBTimerModuloAddress   0xFF06
#define kGBTimerControlAddress  0xFF07

#define kGBTimerEnableFlag      (1 << 2)

typedef struct __GBTimerPort {
    uint16_t address;

    void (*write)(struct __GBTimerPort *this, uint8_t byte);
    uint8_t (*read)(struct __GBTimerPort *this);

    uint8_t value;
} GBTimerPort;

GBTimerPort *GBTimerPortCreate(uint16_t address);
void __GBDividerPortWrite(GBTimerPort *port, uint8_t byte);
uint8_t __GBTimerControlPortRead(GBTimerPort *port);

// 0, 1, 2, 3
// 0, 4, 3, 2
// 9, 3, 5, 7

// 0x1000  (9) [1 / 4096]
// 0x4000  (7) [1 / 1024]
// 0x10000 (5) [1 / 64  ]
// 0x40000 (3) [1 / 16  ]
// all out of 0x400000
// divider & (1 << n) --> inc

// Either every 0x10 ticks, 0x40 ticks, 0x100 ticks, or 0x400 ticks

#pragma mark - Internal Clock

typedef struct __GBClock {
    bool (*install)(struct __GBClock *this, struct __GBGameboy *gameboy);

    GBTimerPort *divider;
    GBTimerPort *timer;
    GBTimerPort *timerModulus;
    GBTimerPort *timerControl;

    uint8_t overflowTicks;
    bool timerOverflow;

    uint64_t internalTick;
    uint16_t tick;

    struct __GBGameboy *gameboy;
} GBClock;

GBClock *GBClockCreate(void);
void GBClockTick(GBClock *this);
void GBClockDestroy(GBClock *this);

bool __GBClockInstall(GBClock *this, struct __GBGameboy *gameboy);

#endif /* !defined(__LIBGB_CLOCK__) */
