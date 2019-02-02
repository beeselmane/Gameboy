#include "bios.h"
#include "gameboy.h"
#include <stdio.h>

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

        bios->write = __GBBIOSROMWrite;
        bios->read = __GBBIOSROMRead;

        bios->start = 0x0000;
        bios->end = kGBBIOSROMSize;

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

void __GBBIOSROMWrite(GBBIOSROM *this, UInt16 address, UInt8 byte)
{
    fprintf(stderr, "Warning: Write to BIOS ROM! (addr=0x%04X, byte=0x%02X)\n", address, byte);
}

UInt8 __GBBIOSROMRead(GBBIOSROM *this, UInt16 address)
{
    return this->data[address];
}
