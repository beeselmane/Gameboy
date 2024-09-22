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

#pragma mark - Power State Functions

bool GBGameboyIsPoweredOn(GBGameboy *this)
{
    return (this->cpu->state.mode != kGBProcessorModeOff);
}

void GBGameboyPowerOff(GBGameboy *this)
{
    while (this->cpu->state.mode != kGBProcessorModeFetch)
        GBClockTick(this->clock);

    this->cpu->state.mode = kGBProcessorModeOff;
}

void GBGameboyPowerOn(GBGameboy *this)
{
    if (this->cpu->state.mode == kGBProcessorModeOff)
        this->cpu->state.mode = kGBProcessorModeFetch;
}

#pragma mark - BIOS Utility Functions

void GBGameboyInstallBIOS(GBGameboy *this, GBBIOSROM *bios)
{
    this->biosInstalled = bios->install(bios, this);

    if (this->biosInstalled)
        this->bios = bios;
}

void GBGameboyMaskBIOS(GBGameboy *this, GBBIOSROM *bios)
{
    // Write to and reset this port. Once masked through this method, BIOS cannot be re-enabled by a game.
    GBIORegister *port = this->mmio->portMap[kGBBIOSMaskPortAddress - kGBIOMapperFirstAddress];

    port->write(port, true);
    port->write(port, false);
}

#pragma mark - Cartridge Utility Functions

bool GBGameboyInsertCartridge(GBGameboy *this, GBCartridge *cart)
{
    return GBCartridgeMap(cart, this);
}

bool GBGameboyEjectCartridge(GBGameboy *this, GBCartridge *cart)
{
    return GBCartridgeUnmap(cart, this);
}
