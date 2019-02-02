#ifndef __gameboy_clock__
#define __gameboy_clock__ 1

#include "base.h"

struct __GBGameboy;

typedef struct __GBClock {
    bool (*install)(struct __GBClock *this, struct __GBGameboy *gameboy);

    UInt64 ticks;

    GBGameboy *gameboy;
} GBClock;

GBClock *GBClockCreate(void);
void GBClockTick(GBClock *this);
void GBClockDestroy(GBClock *this);

bool __GBClockInstall(GBClock *this, struct __GBGameboy *gameboy);

#endif /* !defined(__gameboy_clock__) */
