#include "gameboy.h"
#include <stdio.h>

GBGameboy *GBGameboyCreate(void)
{
    GBGameboy *gameboy = malloc(sizeof(GBGameboy));

    if (gameboy)
    {
        gameboy->cartInstalled = false;
        gameboy->biosInstalled = false;

        gameboy->cpu = GBProcessorCreate();

        if (!gameboy->cpu)
        {
            free(gameboy);

            return NULL;
        }

        gameboy->mmio = GBIOMapperCreate();

        if (!gameboy->mmio)
        {
            GBProcessorDestroy(gameboy->cpu);

            return NULL;
        }

        gameboy->wram = GBWorkRAMCreate();

        if (!gameboy->wram)
        {
            GBIOMapperDestroy(gameboy->mmio);
            GBProcessorDestroy(gameboy->cpu);

            return NULL;
        }

        gameboy->clock = GBClockCreate();

        if (!gameboy->clock)
        {
            GBWorkRAMDestroy(gameboy->wram);
            GBIOMapperDestroy(gameboy->mmio);
            GBProcessorDestroy(gameboy->cpu);

            return NULL;
        }

        bool installed = true;

        installed &= gameboy->mmio->install(gameboy->mmio, gameboy);
        installed &= gameboy->wram->install(gameboy->wram, gameboy);
        installed &= gameboy->clock->install(gameboy->clock, gameboy);

        if (!installed)
        {
            GBClockDestroy(gameboy->clock);
            GBWorkRAMDestroy(gameboy->wram);
            GBIOMapperDestroy(gameboy->mmio);
            GBProcessorDestroy(gameboy->cpu);

            return NULL;
        }
    }

    return gameboy;
}

void GBGameboyInstallBIOS(GBGameboy *this, GBBIOSROM *bios)
{
    this->biosInstalled = bios->install(bios, this);

    if (this->biosInstalled)
        this->bios = bios;
}

void GBGameboyPowerOn(GBGameboy *this)
{
    this->cpu->state.mode = kGBProcessorModeFetch;
}

bool GBGameboyInsertCartridge(GBCartridge *cart);

bool GBGameboyEjectCartridge(GBCartridge *cart);
