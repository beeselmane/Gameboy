#include <libgb/gameboy.h>
#include <stdlib.h>
#include <stdio.h>

GBDMARegister *GBDMARegisterCreate(void)
{
    GBDMARegister *port = malloc(sizeof(GBDMARegister));

    if (port)
    {
        port->address = kGBDMARegisterAddress;

        port->read = (void *)__GBIORegisterSimpleRead;
        port->write = __GBDMARegisterWrite;

        port->value = 0;
        port->inProgress = false;

        port->startAddress = 0x0000;
        port->offset = 0;
        port->ticks = 0;

        port->install = __GBDMARegisterInstall;
        port->tick = __GBDMARegisterTick;
    }

    return port;
}

void GBDMARegisterDestroy(GBDMARegister *this)
{
    free(this);
}

void __GBDMARegisterWrite(GBDMARegister *this, uint8_t byte)
{
    //fprintf(stdout, "Info: DMA Started from address 0x%02X00\n", byte);
    this->value = byte;

    this->inProgress = true;
    this->startAddress = (((uint16_t)byte) << 8);
    this->offset = 0;
    this->ticks = 0;
}

bool __GBDMARegisterInstall(GBDMARegister *this, struct __GBGameboy *gameboy)
{
    gameboy->cpu->mmu->dma = &this->inProgress;
    this->cpu = gameboy->cpu;

    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this);

    return true;
}

void __GBDMARegisterTick(GBDMARegister *this, uint64_t ticks)
{
    // DMA is paused during halt and stop modes.
    if (this->cpu->state.mode == kGBProcessorModeHalted || this->cpu->state.mode == kGBProcessorModeStopped)
        return;

    if (!this->inProgress)
        return;

    this->ticks++;

    if (!(this->ticks % 4))
    {
        if (this->ticks > kGBDMARegisterTotalClocks) {
            //fprintf(stdout, "Info: DMA Completed from address 0x%04X\n", this->startAddress);

            this->inProgress = false;
        }else if (this->ticks > kGBDMARegisterInitClocks) {
            uint16_t nextSource  = this->startAddress + this->offset + 1;
            uint16_t destination = kGBSpriteRAMStart + this->offset;

            //fprintf(stdout, "Info: [DMA] 0x%04X --> 0x%04X (%d)\n", nextSource - 1, destination, this->ticks);
            __GBMemoryManagerWrite(this->cpu->mmu, destination, this->byte);
            this->byte = __GBMemoryManagerRead(this->cpu->mmu, nextSource);

            this->offset++;
        } else {
            this->byte = __GBMemoryManagerRead(this->cpu->mmu, this->startAddress + this->offset);
        }
    }
}
