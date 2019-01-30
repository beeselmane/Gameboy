#include "bios.h"
#include "gameboy.h"

#pragma mark - I/O Port

GBBIOSMaskPort *GBBIOSMaskPortCreate(GBBIOSROM *bios)
{
    GBBIOSMaskPort *port = malloc(sizeof(GBBIOSMaskPort));

    if (port)
    {
        port->address = kGBBIOSMaskPortAddress;

        port->write = __GBBIOSMaskPortWrite;
        port->read = __GBBIOSMaskPortRead;

        port->value = 0;
        port->bios = bios;
    }

    return port;
}

void __GBBIOSMaskPortWrite(GBBIOSMaskPort *port, UInt8 byte)
{
    if (!port->bios->masked && byte)
        port->bios->masked = true;

    port->value = byte;
}

UInt8 __GBBIOSMaskPortRead(GBBIOSMaskPort *port)
{
    return port->value;
}

#pragma mark - BIOS

GBBIOSROM *GBBIOSROMCreate(UInt8 data[kGBBIOSROMSize])
{
    GBBIOSROM *bios = malloc(sizeof(GBBIOSROM));

    if (bios)
    {
        memcpy(bios->data, data, kGBBIOSROMSize);
        bios->install = __GBBIOSROMOnInstall;

        bios->port = GBBIOSMaskPortCreate(bios);
        bios->masked = false;

        if (!bios->port)
        {
            free(bios);
            bios = NULL;
        }
    }

    return bios;
}

void GBBIOSROMDestroy(GBBIOSROM *this)
{
    free(this->port);
    free(this);
}

bool __GBBIOSROMOnInstall(GBBIOSROM *this, GBGameboy *gameboy)
{
    gameboy->cpu->mmu->romSpace = (GBMemorySpace *)this;
    gameboy->cpu->mmu->romMasked = &this->masked;

    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->port);
    return true;
}
