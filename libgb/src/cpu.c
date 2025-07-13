#include <libgb/gameboy.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#pragma mark - Interrupt Controller

#pragma mark - High RAM

#pragma mark - Processor Core

#include "指令集.h"

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

void GBProcessorDestroy(GBProcessor *cpu)
{
    GBMemoryManagerDestroy(cpu->mmu);

    free(cpu);
}

void GBDispatchOP(GBProcessor *cpu)
{
    GBProcessorOP *op;

    if (cpu->state.prefix) {
        op = cpu->decode_prefix[cpu->state.op];
    } else {
        op = cpu->decode[cpu->state.op];
    }

    //printf("Dispatch '%s' (%02X) [%d]\n", op->name, op->value, cpu->state.mode);
    op->go(op, cpu);
}

void __GBProcessorTick(GBProcessor *cpu, uint64_t tick)
{
    switch (cpu->state.mode)
    {
        case kGBProcessorModeHalted: {
            if (GBInterruptControllerCheck(cpu->ic))
            {
                if (cpu->state.enableIME) {
                    cpu->state.mode = kGBProcessorModeInterrupted;
                    cpu->state.data = 0;

                    return;
                } else {
                    cpu->state.mode = kGBProcessorModeFetch;
                }
            }
        } break;
        case kGBProcessorModeStopped:
        case kGBProcessorModeOff:
            return;
        case kGBProcessorModeFetch: {
            if (cpu->state.enableIME)
            {
                cpu->state.enableIME = false;
                cpu->state.ime = true;
            }

            if (cpu->ic->interruptPending)
            {
                cpu->state.mode = kGBProcessorModeInterrupted;

                //printf("CPU Interrupted! Jump to 0x%04X\n", cpu->ic->destination);

                // We use cpu to track stalls
                cpu->state.data = 0;

                return;
            }

            __mmu_read(cpu, cpu->state.pc++);
            cpu->state.mode = kGBProcessorModeRun;
        } break;
        case kGBProcessorModePrefix: {
            if (cpu->state.accessed)
            {
                cpu->state.op = cpu->state.mdr;

                GBDispatchOP(cpu);
            }
        } break;
        case kGBProcessorModeInterrupted: {
            if (cpu->ic->interruptPending && !GBInterruptControllerCheck(cpu->ic))
            {
                printf("Interrupt Cancelled.\n");

                cpu->state.mode = kGBProcessorModeFetch;
                cpu->ic->interruptPending = false;

                return;
            }

            GBInterruptControllerReset(cpu->ic);

            switch (cpu->state.data)
            {
                case 0: {
                    // Store high byte of PC
                    __mmu_write(cpu, cpu->state.sp - 1, cpu->state.pc >> 8);

                    cpu->state.data++;
                } break;
                case 1: {
                    // Store low byte of PC
                    if (cpu->state.accessed)
                    {
                        __mmu_write(cpu, cpu->state.sp - 2, cpu->state.pc & 0xFF);

                        cpu->state.data++;
                    }
                } break;
                case 2: {
                    // Stall once (Also set PC to interrupt vector)
                    if (cpu->state.accessed)
                    {
                        __mmu_read(cpu, 0x0000);

                        cpu->state.pc = cpu->ic->destination;
                        cpu->state.sp -= 2;

                        cpu->state.data++;
                    }
                } break;
                case 3: {
                    // Stall again (Also reset interrupt controller)
                    if (cpu->state.accessed)
                    {
                        __mmu_read(cpu, 0x0000);

                        cpu->state.ime = false;
                        cpu->state.data++;
                    }
                } break;
                case 4: {
                    if (cpu->state.accessed)
                        cpu->state.mode = kGBProcessorModeFetch;
                } break;
            }
        } break;
        case kGBProcessorModeStalled: {
            // Some opcodes stall until a memory tick. They should know how to handle cpu themselves.
            GBDispatchOP(cpu);
        } break;
        case kGBProcessorModeRun: {
            if (cpu->state.accessed)
            {
                if (cpu->state.mdr == 0xCB) {
                    cpu->state.prefix = true;
                    cpu->state.op = 0xCB;

                    __mmu_read(cpu, cpu->state.pc++);

                    cpu->state.mode = kGBProcessorModePrefix;
                } else {
                    cpu->state.op = cpu->state.mdr;
                    cpu->state.prefix = false;

                    GBDispatchOP(cpu);
                }
            }
        } break;
        case kGBProcessorModeWait1:
        case kGBProcessorModeWait2:
        case kGBProcessorModeWait3:
        case kGBProcessorModeWait4: {
            GBDispatchOP(cpu);
        } break;
    }
}
