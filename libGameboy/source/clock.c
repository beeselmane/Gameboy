#include "clock.h"
#include "gameboy.h"
#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>

#pragma mark - Timer Registers

GBTimerPort *GBTimerPortCreate(UInt16 address)
{
    GBTimerPort *port = malloc(sizeof(GBTimerPort));

    if (port)
    {
        port->address = address;

        port->write = (void *)__GBIORegisterSimpleWrite;
        port->read = (void *)__GBIORegisterSimpleRead;

        port->value = 0;
    }

    return port;
}

void __GBDividerPortWrite(GBTimerPort *port, UInt8 byte)
{
    port->value = 0;
}

UInt8 __GBTimerControlPortRead(GBTimerPort *port)
{
    return port->value & 0xF8;
}

#pragma mark - Internal Clock

UInt8 gGBTimerBitLookup[4] = {9, 3, 5, 7};

GBClock *GBClockCreate(void)
{
    GBClock *clock = malloc(sizeof(GBClock));

    if (clock)
    {
        clock->gameboy = NULL;

        clock->divider = GBTimerPortCreate(kGBDividerAddress);
        clock->timer = GBTimerPortCreate(kGBTimerAddress);
        clock->timerModulus = GBTimerPortCreate(kGBTimerModuloAddress);
        clock->timerControl = GBTimerPortCreate(kGBTimerControlAddress);

        clock->timerControl->read = __GBTimerControlPortRead;
        clock->divider->write = __GBDividerPortWrite;

        clock->internalTick = 0;
        clock->tick = 0;

        clock->install = __GBClockInstall;
    }

    return clock;
}

void GBClockTick(GBClock *this)
{
    if (!this->gameboy)
        return;

    bool timerEnable = !!(this->timerControl->value & kGBTimerEnableFlag);

    this->internalTick++;
    this->tick++;

    if (this->timerOverflow)
    {
        this->overflowTicks++;

        if (this->overflowTicks == 4) {
            this->gameboy->cpu->ic->interruptFlagPort->value |= (1 << kGBInterruptTimer);
        } else if (this->overflowTicks == 5) {
            this->timer->value = this->timerModulus->value;
        } else if (this->overflowTicks == 6) {
            this->timer->value = this->timerModulus->value;

            this->timerOverflow = false;
            this->overflowTicks = 0;
        }
    }

    if (timerEnable)
    {
        UInt16 flag = (1 << gGBTimerBitLookup[this->timerControl->value & 0x3]);

        if (this->divider->value & flag)
        {
            this->timer->value++;

            if (!this->timer)
            {
                this->timerOverflow = true;
                this->overflowTicks = 0;
            }
        }
    }

    this->divider->value = this->tick >> 8;

    // Get timer speed and set new value.

    GBMemoryManager *mmu = this->gameboy->cpu->mmu;
    GBGraphicsDriver *driver = this->gameboy->driver;
    GBProcessor *cpu = this->gameboy->cpu;
    GBInterruptController *ic = cpu->ic;

    mmu->tick(mmu, this->internalTick);
    driver->tick(driver, this->internalTick);
    cpu->tick(cpu, this->internalTick);
    ic->tick(ic, this->internalTick);

    static CFAbsoluteTime time = 0;

    if (!time)
        time = CFAbsoluteTimeGetCurrent();

    if (!(this->internalTick % 0x800000))
    {
        CFAbsoluteTime updated = CFAbsoluteTimeGetCurrent();
        CFAbsoluteTime diff = updated - time;

        printf("Executed ticks in %fs (real GB should be 2.0s)\n", diff);

        time = updated;
    }
}

void GBClockDestroy(GBClock *this)
{
    free(this);
}

bool __GBClockInstall(GBClock *this, struct __GBGameboy *gameboy)
{
    this->gameboy = gameboy;

    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->divider);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->timer);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->timerModulus);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->timerControl);

    return true;
}
