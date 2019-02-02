#include "clock.h"
#include "gameboy.h"
#include <stdio.h>

GBClock *GBClockCreate(void)
{
    GBClock *clock = malloc(sizeof(GBClock));

    if (clock)
    {
        clock->gameboy = NULL;

        clock->ticks = 0;

        clock->install = __GBClockInstall;
    }

    return clock;
}

void GBClockTick(GBClock *this)
{
    if (!this->gameboy)
        return;

    this->ticks++;

    // Every 4 ticks tick memory once
    if (!(this->ticks & 3))
        this->gameboy->cpu->mmu->tick(this->gameboy->cpu->mmu);

    this->gameboy->cpu->tick(this->gameboy->cpu);
}

void GBClockDestroy(GBClock *this)
{
    free(this);
}

bool __GBClockInstall(GBClock *this, struct __GBGameboy *gameboy)
{
    this->gameboy = gameboy;

    return true;
}
