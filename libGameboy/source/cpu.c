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

void GBProcessorTick(GBProcessor *this)
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

#if 0
    UInt8 instruction = GBMemoryManagerRead(this->mmu, this->state.pc++);
    GBProcessorOP *operation;

    if (instruction == 0xCB) {
        // prefix. Wait until next read cycle.
        operation = this->decode_prefix[instruction];
    } else {
        // normal. decode immidietely.
        operation = this->decode[instruction];
    }

    operation->go(operation, this);
#endif

    // check for interrupt here
}
