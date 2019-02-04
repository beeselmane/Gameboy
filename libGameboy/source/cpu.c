#include "cpu.h"
#include "gameboy.h"
#include <stdio.h>

#pragma mark - Interrupt Controller

#pragma mark - High RAM

#pragma mark - Processor Core

#include "指令集.h"

// We can only read memory on memory line ticks
// --> This is 1/4 our tick rate

// fetch, increment, decode, perform

// fetch, increment, decode, data fetch (lags), increment, perform, data store (lags)

// fetch, increment, decode, prefix (wait), fetch, increment, decode, perform

GBProcessor *GBProcessorCreate(void)
{
    GBProcessor *cpu = malloc(sizeof(GBProcessor));

    if (cpu)
    {
        cpu->mmu = GBMemoryManagerCreate();

        if (!cpu->mmu)
        {
            free(cpu);

            return NULL;
        }

        cpu->state.mode = kGBProcessorModeOff;

        cpu->state.ime = false;
        cpu->state.a = 0;

        cpu->state.bc = 0;
        cpu->state.de = 0;
        cpu->state.hl = 0;

        cpu->state.sp = 0;
        cpu->state.pc = 0;

        cpu->state.accessed = false;
        cpu->state.mar = 0;
        cpu->state.mdr = 0;

        cpu->state.prefix = false;
        cpu->state.op = 0;

        cpu->state.data = 0;

        memcpy(cpu->decode_prefix, gGBInstructionSetCB, 0x100 * sizeof(GBProcessorOP *));
        memcpy(cpu->decode, gGBInstructionSet, 0x100 * sizeof(GBProcessorOP *));

        cpu->tick = __GBProcessorTick;
    }

    return cpu;
}

void GBProcessorDestroy(GBProcessor *this)
{
    GBMemoryManagerDestroy(this->mmu);

    free(this);
}

void GBDispatchOP(GBProcessor *this)
{
    GBProcessorOP *op;

    if (this->state.prefix) {
        op = this->decode_prefix[this->state.op];
    } else {
        op = this->decode[this->state.op];
    }

    op->go(op, this);
}

void __GBProcessorTick(GBProcessor *this, UInt64 tick)
{
    switch (this->state.mode)
    {
        case kGBProcessorModeHalted:
            break;
        case kGBProcessorModeStopped:
        case kGBProcessorModeOff:
            return;
        case kGBProcessorModeFetch: {
            __GBProcessorRead(this, this->state.pc++);

            this->state.mode = kGBProcessorModeRun;
        } break;
        case kGBProcessorModePrefix: {
            if (this->state.accessed)
            {
                this->state.op = this->state.mdr;

                GBDispatchOP(this);
            }
        } break;
        case kGBProcessorModeStalled: {
            // Some opcodes stall until a memory tick. They should know how to handle this themselves.
            GBDispatchOP(this);
        } break;
        case kGBProcessorModeRun: {
            if (this->state.accessed)
            {
                if (this->state.mdr == 0xCB) {
                    this->state.prefix = true;
                    this->state.op = 0xCB;

                    __GBProcessorRead(this, this->state.pc++);

                    this->state.mode = kGBProcessorModePrefix;
                } else {
                    this->state.op = this->state.mdr;
                    this->state.prefix = false;

                    GBDispatchOP(this);
                }
            }
        } break;
        case kGBProcessorModeWait1:
        case kGBProcessorModeWait2:
        case kGBProcessorModeWait3:
        case kGBProcessorModeWait4: {
            GBDispatchOP(this);
        } break;
    }

    // check for interrupt here
}
