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

bool printed[0x100] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void __GBClockTimerDebug(GBClock *this, GBProcessor *cpu, GBGraphicsDriver *driver)
{
    if (cpu->state.pc < 0x100 && !printed[cpu->state.pc])
    {
        //printf("PC: 0x%04X, timer: 0x%04X\n", cpu->state.pc, this->tick);

        printed[cpu->state.pc] = true;
    }

    static UInt64 lastFetch = 0;

    if (!lastFetch && cpu->state.mode == kGBProcessorModeFetch)
        lastFetch = this->internalTick;

    if (cpu->state.mode == kGBProcessorModeFetch)
    {
        UInt64 diff = this->internalTick - lastFetch;
        lastFetch = this->internalTick;

        const char *opString = (cpu->state.prefix ? cpu->decode_prefix[cpu->state.op]->name : cpu->decode[cpu->state.op]->name);

        //printf("Info: Instruction '%s' ran in %llu ticks.\n", opString, diff);
    }

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

    static UInt64 lastScreenBegin = 0;

    if (driver->displayOn)
    {
        static UInt8 lastMode = kGBDriverStateSpriteSearch;

        if (!lastScreenBegin && driver->driverMode == kGBDriverStateSpriteSearch)
            lastScreenBegin = this->internalTick;

        if (lastMode == kGBDriverStateVBlank && driver->driverMode == kGBDriverStateSpriteSearch)
        {
            UInt64 diff = this->internalTick - lastScreenBegin;
            lastScreenBegin = this->internalTick;

            //printf("Info: Last screen drawn in %llu ticks.\n", diff);
        }

        lastMode = driver->driverMode;
    }
}

// 0xC224
// 0xC7D3 --> std_print
// 0xC246 (C24F)
// 0xC24C
// 0xC399
// 0xC3B1

void GBClockTick(GBClock *this)
{
    if (!this->gameboy || this->gameboy->cpu->state.mode == kGBProcessorModeOff)
        return;

    bool timerEnable = !!(this->timerControl->value & kGBTimerEnableFlag);
    GBMemoryManager *mmu = this->gameboy->cpu->mmu;
    GBGraphicsDriver *driver = this->gameboy->driver;
    GBDMARegister *dma = this->gameboy->dma;
    GBProcessor *cpu = this->gameboy->cpu;
    GBInterruptController *ic = cpu->ic;

    //__GBClockTimerDebug(this, cpu, driver);

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

    mmu->tick(mmu, this->internalTick);
    driver->tick(driver, this->internalTick);
    cpu->tick(cpu, this->internalTick);
    ic->tick(ic, this->internalTick);
    dma->tick(dma, this->internalTick);
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
