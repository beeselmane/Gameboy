#include "gameboy.h"
#include <stdio.h>

GBGameboy *GBGameboyCreate(void)
{
    GBGameboy *gameboy = malloc(sizeof(GBGameboy));

    if (gameboy)
    {
        bzero(gameboy, sizeof(GBGameboy));
        bool success = true;

        gameboy->cartInstalled = false;
        gameboy->biosInstalled = false;

        gameboy->cpu = GBProcessorCreate();
        success &= !!gameboy->cpu;

        gameboy->driver = GBGraphicsDriverCreate();
        success &= !!gameboy->driver;

        gameboy->mmio = GBIOMapperCreate();
        success &= !!gameboy->mmio;

        gameboy->wram = GBWorkRAMCreate();
        success &= !!gameboy->wram;

        gameboy->vram = GBVideoRAMCreate();
        success &= !!gameboy->vram;

        gameboy->gamepad = GBGamepadCreate();
        success &= !!gameboy->gamepad;

        gameboy->dma = GBDMARegisterCreate();
        success &= !!gameboy->dma;

        gameboy->clock = GBClockCreate();
        success &= !!gameboy->clock;

        if (!success)
        {
            if (gameboy->dma)
                GBDMARegisterDestroy(gameboy->dma);

            if (gameboy->gamepad)
                free(gameboy->gamepad);

            if (gameboy->wram)
                GBWorkRAMDestroy(gameboy->wram);

            if (gameboy->vram)
                GBVideoRAMDestroy(gameboy->vram);

            if (gameboy->mmio)
                GBIOMapperDestroy(gameboy->mmio);

            if (gameboy->driver)
                GBGraphicsDriverDestroy(gameboy->driver);

            if (gameboy->cpu)
                GBProcessorDestroy(gameboy->cpu);

            return NULL;
        }

        bool installed = true;

        installed &= gameboy->mmio->install(gameboy->mmio, gameboy);
        installed &= gameboy->wram->install(gameboy->wram, gameboy);
        installed &= gameboy->vram->install(gameboy->vram, gameboy);
        installed &= gameboy->driver->install(gameboy->driver, gameboy);
        installed &= gameboy->gamepad->install(gameboy->gamepad, gameboy);
        installed &= gameboy->dma->install(gameboy->dma, gameboy);
        installed &= gameboy->cpu->ic->install(gameboy->cpu->ic, gameboy);
        installed &= gameboy->clock->install(gameboy->clock, gameboy);

        if (!installed)
        {
            GBClockDestroy(gameboy->clock);
            GBDMARegisterDestroy(gameboy->dma);
            GBGamepadDestroy(gameboy->gamepad);
            GBWorkRAMDestroy(gameboy->wram);
            GBVideoRAMDestroy(gameboy->vram);
            GBIOMapperDestroy(gameboy->mmio);
            GBGraphicsDriverDestroy(gameboy->driver);
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
