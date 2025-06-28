#include <libgb/gameboy.h>
#include <strings.h>
#include <stdlib.h>
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
        cpu->ic = GBInterruptControllerCreate(cpu);

        if (!cpu->ic)
        {
            free(cpu);

            return NULL;
        }

        cpu->mmu = GBMemoryManagerCreate();

        if (!cpu->mmu)
        {
            GBInterruptControllerDestroy(cpu->ic);
            free(cpu);

            return NULL;
        }

        cpu->mmu->interruptControl = &cpu->ic->interruptControl;
        cpu->state.mode = kGBProcessorModeOff;

        cpu->state.enableIME = false;
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
        cpu->state.data = 0;
        cpu->state.op = 0;

        cpu->state.bug = false;

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

void __GBProcessorTick(GBProcessor *this, uint64_t tick)
{
    switch (this->state.mode)
    {
        case kGBProcessorModeHalted: {
            if (GBInterruptControllerCheck(this->ic))
            {
                if (this->state.enableIME) {
                    this->state.mode = kGBProcessorModeInterrupted;
                    this->state.data = 0;

                    return;
                } else {
                    this->state.mode = kGBProcessorModeFetch;
                }
            }
        } break;
        case kGBProcessorModeStopped:
        case kGBProcessorModeOff:
            return;
        case kGBProcessorModeFetch: {
            if (this->state.enableIME)
            {
                this->state.enableIME = false;
                this->state.ime = true;
            }

            if (this->ic->interruptPending)
            {
                this->state.mode = kGBProcessorModeInterrupted;

                //printf("CPU Interrupted! Jump to 0x%04X\n", this->ic->destination);

                // We use this to track stalls
                this->state.data = 0;

                return;
            }

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
        case kGBProcessorModeInterrupted: {
            if (this->ic->interruptPending && !GBInterruptControllerCheck(this->ic))
            {
                printf("Interrupt Cancelled.\n");

                this->state.mode = kGBProcessorModeFetch;
                this->ic->interruptPending = false;

                return;
            }

            GBInterruptControllerReset(this->ic);

            switch (this->state.data)
            {
                case 0: {
                    // Store high byte of PC
                    __GBProcessorWrite(this, this->state.sp - 1, this->state.pc >> 8);

                    this->state.data++;
                } break;
                case 1: {
                    // Store low byte of PC
                    if (this->state.accessed)
                    {
                        __GBProcessorWrite(this, this->state.sp - 2, this->state.pc & 0xFF);

                        this->state.data++;
                    }
                } break;
                case 2: {
                    // Stall once (Also set PC to interrupt vector)
                    if (this->state.accessed)
                    {
                        __GBProcessorRead(this, 0x0000);

                        this->state.pc = this->ic->destination;
                        this->state.sp -= 2;

                        this->state.data++;
                    }
                } break;
                case 3: {
                    // Stall again (Also reset interrupt controller)
                    if (this->state.accessed)
                    {
                        __GBProcessorRead(this, 0x0000);

                        this->state.ime = false;
                        this->state.data++;
                    }
                } break;
                case 4: {
                    if (this->state.accessed)
                        this->state.mode = kGBProcessorModeFetch;
                } break;
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
}
