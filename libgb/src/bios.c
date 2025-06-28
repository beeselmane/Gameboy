#include <libgb/gameboy.h>
#include <strings.h>
#include <stdlib.h>
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

void __GBBIOSMaskPortWrite(GBBIOSMaskPort *port, uint8_t byte)
{
    if (!port->bios->masked && byte)
        port->bios->masked = true;

    port->value = byte;
}

uint8_t __GBBIOSMaskPortRead(GBBIOSMaskPort *port)
{
    return port->value;
}

#pragma mark - BIOS

GBBIOSROM *GBBIOSROMCreate(uint8_t data[kGBBIOSROMSize])
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

void __GBBIOSROMWrite(GBBIOSROM *this, uint16_t address, uint8_t byte)
{
    fprintf(stderr, "Warning: Write to BIOS ROM! (addr=0x%04X, byte=0x%02X)\n", address, byte);
}

uint8_t __GBBIOSROMRead(GBBIOSROM *this, uint16_t address)
{
    return this->data[address];
}
