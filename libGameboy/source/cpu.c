#include "cpu.h"
#include "gameboy.h"

#pragma mark - Interrupt Controller

#pragma mark - High RAM

#pragma mark - Processor Core

#include "指令集.h"

// We can only read memory on memory line ticks
// --> This is 1/4 our tick rate

// fetch, increment, decode, perform

// fetch, increment, decode, data fetch (lags), increment, perform, data store (lags)

// fetch, increment, decode, prefix (wait), fetch, increment, decode, perform

void GBProcessorTick(GBProcessor *this)
{
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

    // check for interrupt here
}
