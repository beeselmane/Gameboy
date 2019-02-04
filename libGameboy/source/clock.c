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

    GBMemoryManager *mmu = this->gameboy->cpu->mmu;
    GBGraphicsDriver *driver = this->gameboy->driver;
    GBProcessor *cpu = this->gameboy->cpu;

    mmu->tick(mmu, this->ticks);
    driver->tick(driver, this->ticks);
    cpu->tick(cpu, this->ticks);
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
