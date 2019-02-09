#include "ic.h"
#include "gameboy.h"

#pragma mark - Interrupt Request

GBInterruptFlagPort *GBInterruptFlagPortCreate(void)
{
    GBInterruptFlagPort *port = malloc(sizeof(GBInterruptFlagPort));

    if (port)
    {
        port->address = kGBInterruptFlagAddress;

        port->read = (void *)__GBIORegisterSimpleRead;
        port->write = __GBInterruptFlagPortWrite;

        port->value = 0;
    }

    return port;
}

void __GBInterruptFlagPortWrite(GBInterruptFlagPort *port, UInt8 byte)
{
    port->value &= 0xE0;
    port->value |= byte & ~0xE0;
}

#pragma mark - Intrrupt Controller

GBInterruptController *GBInterruptControllerCreate(struct __GBProcessor *cpu)
{
    GBInterruptController *ic = malloc(sizeof(GBInterruptController));

    if (ic)
    {
        ic->interruptFlagPort = GBInterruptFlagPortCreate();

        if (!ic->interruptFlagPort)
        {
            free(ic);

            return NULL;
        }

        ic->interruptControl = 0;
        ic->cpu = cpu;

        ic->interruptPending = false;
        ic->destination = 0x0000;

        ic->install = __GBInterruptControllerInstall;
        ic->tick = __GBInterruptControllerTick;

        ic->lookup[kGBInterruptVBlank]  = kGBInterruptVBlankAddress;
        ic->lookup[kGBInterruptLCDStat] = kGBInterruptLCDStatAddress;
        ic->lookup[kGBInterruptTimer]   = kGBInterruptTimerAddress;
        ic->lookup[kGBInterruptSerial]  = kGBInterruptSerialAddress;
        ic->lookup[kGBInterruptJoypad]  = kGBInterruptJoypadAddress;
    }

    return ic;
}

bool GBInterruptControllerCheck(GBInterruptController *this)
{
    return (this->cpu->state.ime && (this->interruptFlagPort->value & this->interruptControl & 0x1F));
}

void GBInterruptControllerReset(GBInterruptController *this)
{
    this->interruptFlagPort->value &= ~(1 << this->interrupt);
    this->interruptPending = false;
}

void GBInterruptControllerDestroy(GBInterruptController *this)
{
    free(this->interruptFlagPort);
    free(this);
}

bool __GBInterruptControllerInstall(GBInterruptController *this, struct __GBGameboy *gameboy)
{
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->interruptFlagPort);

    return true;
}

void __GBInterruptControllerTick(GBInterruptController *this, UInt64 tick)
{
    if (this->cpu->state.mode != kGBProcessorModeFetch)
        return;

    // Don't check for interrupts if interrupts are disabled or an interrupt is already pending
    if (!this->cpu->state.ime || this->interruptPending)
        return;

    UInt8 flagged = this->interruptFlagPort->value;
    UInt8 enabled = this->interruptControl;

    for (UInt8 i = 0; i < kGBInterruptCount; i++)
    {
        bool isFlagged = !!(flagged & (1 << i));
        bool isEnabled = !!(enabled & (1 << i));

        if (isEnabled && isFlagged)
        {
            // Trigger this interrupt. Any other pending interrupts will have to wait their turn...
            this->destination = this->lookup[i];
            this->interruptPending = true;
            this->interrupt = i;

            return;
        }
    }

    this->interruptPending = false;
}
