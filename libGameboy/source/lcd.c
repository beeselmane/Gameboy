#include "lcd.h"
#include "gameboy.h"
#include "mmio.h"
#include "mmu.h"
#include "ic.h"

#ifdef __APPLE__
    #include <Security/Security.h>
#endif /* defined(__APPLE__) */

#pragma mark - Video RAM

GBVideoRAM *GBVideoRAMCreate(void)
{
    GBVideoRAM *ram = malloc(sizeof(GBVideoRAM));

    if (ram)
    {
        ram->install = __GBVideoRAMOnInstall;

        ram->write = __GBVideoRAMWrite;
        ram->read = __GBVideoRAMRead;

        ram->start = kGBVideoRAMStart;
        ram->end = kGBVideoRAMEnd;

        #ifdef __APPLE__
            // This warns if we ignore the result implicitly
            __unused int result = SecRandomCopyBytes(kSecRandomDefault, kGBVideoRAMSize, ram->memory);
        #else /* !defined(__APPLE__) */
            bzero(ram->memory, kGBVideoRAMSize);
        #endif /* defined(__APPLE__) */
    }

    return ram;
}

void GBVideoRAMDestroy(GBVideoRAM *this)
{
    free(this);
}

void __GBVideoRAMWrite(GBVideoRAM *this, UInt16 address, UInt8 byte)
{
    if (this->driver->displayOn)
    {
        if (this->driver->driverMode == kGBDriverStatePixelTransfer)
            return;
    }

    if (address < 0x9800) {
        // I dunno
    } else if (address < 0x9C00) {
        //UInt16 addr = address & (~(1 << 15));
        //fprintf(stderr, "note: wrote to background map 0. (0x%04X=0x%02X) [0x%04X]\n", address, byte, addr);
    } else {
        //fprintf(stderr, "note: wrote to background map 1. (0x%04X=0x%02X)\n", address, byte);
    }

    this->memory[address & (~0x8000)] = byte;
}

UInt8 __GBVideoRAMRead(GBVideoRAM *this, UInt16 address)
{
    if (this->driver->displayOn)
    {
        if (this->driver->driverMode == kGBDriverStatePixelTransfer)
            return 0xFF;
    }

    return this->memory[address & (~0x8000)];
}

bool __GBVideoRAMOnInstall(GBVideoRAM *this, GBGameboy *gameboy)
{
    return GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
}

#pragma - Sprite RAM

GBSpriteRAM *GBSpriteRAMCreate(void)
{
    GBSpriteRAM *ram = malloc(sizeof(GBSpriteRAM));

    if (ram)
    {
        ram->install = __GBSpriteRAMOnInstall;

        ram->write = __GBSpriteRAMWrite;
        ram->read = __GBSpriteRAMRead;

        ram->start = kGBSpriteRAMStart;
        ram->end = kGBSpriteRAMEnd;

        #ifdef __APPLE__
            // This warns if we ignore the result implicitly
            __unused int result = SecRandomCopyBytes(kSecRandomDefault, kGBSpriteRAMSize, ram->memory);
        #else /* !defined(__APPLE__) */
            bzero(ram->memory, kGBSpriteRAMSize);
        #endif /* defined(__APPLE__) */
    }

    return ram;
}

void GBSpriteRAMDestroy(GBSpriteRAM *this)
{
    free(this);
}

void __GBSpriteRAMWrite(GBSpriteRAM *this, UInt16 address, UInt8 byte)
{
    if (this->driver->displayOn)
    {
        if (this->driver->driverMode == kGBDriverStateSpriteSearch)
            return;

        if (this->driver->driverMode == kGBDriverStatePixelTransfer)
            return;
    }

    ((UInt8 *)this->memory)[address & (~kGBSpriteRAMStart)] = byte;
}

UInt8 __GBSpriteRAMRead(GBSpriteRAM *this, UInt16 address)
{
    if (this->driver->displayOn)
    {
        if (this->driver->driverMode == kGBDriverStateSpriteSearch)
            return 0xFF;

        if (this->driver->driverMode == kGBDriverStatePixelTransfer)
            return 0xFF;
    }

    return ((UInt8 *)this->memory)[address & (~kGBSpriteRAMStart)];
}

bool __GBSpriteRAMOnInstall(GBSpriteRAM *this, GBGameboy *gameboy)
{
    return GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
}

#pragma mark - I/O Ports

GBLCDControlPort *GBLCDControlPortCreate(GBCoordPort *coordPort)
{
    GBLCDControlPort *port = malloc(sizeof(GBLCDControlPort));

    if (port)
    {
        port->address = kGBLCDControlPortAddress;

        port->read = (void *)__GBIORegisterSimpleRead;
        port->write = __GBLCDControlPortWrite;

        port->value = 0x00;
        port->coordPort = coordPort;
    }

    return port;
}

void __GBLCDControlPortWrite(GBLCDControlPort *this, UInt8 byte)
{
    this->value = byte;

    if (!(byte >> 7)) {
        if (this->driver->driverMode != kGBDriverStateVBlank)
        {
            fprintf(stderr, "WARNING: LCD was disabled during a mode other than V-Blank.\n");
            fprintf(stderr, "WARNING: This has been known to damage hardware!\n");
            fprintf(stderr, "Note: Luckily, this isn't real hardware, so oh well...\n");
        }

        this->driver->driverMode = kGBDriverStateVBlank;
        this->driver->driverModeTicks = 0;

        this->coordPort->value = 0;
        this->driver->displayOn = false;

        fprintf(stderr, "Note: Turned off display.\n");
    } else {
        this->driver->driverMode = kGBDriverStateSpriteSearch;
        this->driver->driverModeTicks = 0;
        this->driver->displayOn = true;

        fprintf(stderr, "Note: Turned on display.\n");
    }
}

GBLCDStatusPort *GBLCDStatusPortCreate(void)
{
    GBLCDStatusPort *port = malloc(sizeof(GBLCDStatusPort));

    if (port)
    {
        port->address = kGBLCDStatusPortAddress;

        port->write = (void *)__GBIORegisterSimpleWrite;
        port->read = (void *)__GBIORegisterSimpleRead;

        port->value = 0x00;
    }

    return port;
}

void __GBLCDStatusPortWrite(GBLCDStatusPort *this, UInt8 byte)
{
    // Don't allow writing to the lowest 2 bits of this register
    this->value &= 0x3;

    this->value |= (byte & 0xFC);
}

GBCoordPort *GBCoordPortCreate(void)
{
    GBCoordPort *port = malloc(sizeof(GBCoordPort));

    if (port)
    {
        port->address = kGBCoordPortAddress;

        port->read = (void *)__GBIORegisterSimpleRead;
        port->write = __GBCoordPortWrite;

        port->value = 0x00;
    }

    return port;
}

void __GBCoordPortWrite(GBCoordPort *this, UInt8 byte)
{
    this->value = 0;
}

GBVideoPortGeneric *GBVideoPortGenericCreate(UInt16 address)
{
    GBVideoPortGeneric *port = malloc(sizeof(GBVideoPortGeneric));

    if (port)
    {
        port->address = address;

        port->write = (void *)__GBIORegisterSimpleWrite;
        port->read = (void *)__GBIORegisterSimpleRead;

        port->value = 0x00;
    }

    return port;
}

#pragma mark - LCD Driver

GBGraphicsDriver *GBGraphicsDriverCreate(void)
{
    GBGraphicsDriver *driver = malloc(sizeof(GBGraphicsDriver));

    if (driver)
    {
        driver->oam = GBSpriteRAMCreate();

        if (!driver->oam)
        {
            free(driver);

            return NULL;
        }

        bool success = true;

        driver->paletteSprite1 = GBVideoPortGenericCreate(kGBPalettePortSprite1Address);
        driver->paletteSprite0 = GBVideoPortGenericCreate(kGBPalettePortSprite0Address);
        driver->paletteBG = GBVideoPortGenericCreate(kGBPalettePortBGAddress);

        success &= !!driver->paletteBG & !!driver->paletteSprite0 && !!driver->paletteSprite1;

        driver->windowX = GBVideoPortGenericCreate(kGBLineWindowPortXAddress);
        driver->windowY = GBVideoPortGenericCreate(kGBLineWindowPortYAddress);

        success &= !!driver->windowX & !!driver->windowY;

        driver->compare = GBVideoPortGenericCreate(kGBLineComparePortAddress);
        driver->coordinate = GBCoordPortCreate();

        success &= !!driver->compare & !!driver->coordinate;

        driver->scrollX = GBVideoPortGenericCreate(kGBScrollPortXAddress);
        driver->scrollY = GBVideoPortGenericCreate(kGBScrollPortYAddress);

        success &= !!driver->scrollX & !!driver->scrollY;

        driver->status = GBLCDStatusPortCreate();
        driver->control = GBLCDControlPortCreate(driver->coordinate);

        success &= !!driver->control & !!driver->status;

        if (!success)
        {
            GBSpriteRAMDestroy(driver->oam);

            if (driver->control)
                free(driver->control);

            if (driver->status)
                free(driver->status);

            if (driver->scrollY)
                free(driver->scrollY);

            if (driver->scrollX)
                free(driver->scrollX);

            if (driver->coordinate)
                free(driver->coordinate);

            if (driver->compare)
                free(driver->compare);

            if (driver->windowY)
                free(driver->windowY);

            if (driver->windowX)
                free(driver->windowX);

            if (driver->paletteBG)
                free(driver->paletteBG);

            if (driver->paletteSprite0)
                free(driver->paletteSprite0);

            if (driver->paletteSprite1)
                free(driver->paletteSprite1);

            free(driver);
            return NULL;
        }

        driver->displayOn = false;

        driver->driverMode = kGBDriverStateVBlank;
        driver->driverModeTicks = 0;

        driver->lineSpriteCount = 0;
        driver->fifoPosition = 0;
        driver->spriteIndex = 0;
        driver->driverX = 0;

        driver->tick = __GBGraphicsDriverTick;
        driver->install = __GBGraphicsDriverInstall;
    }

    return driver;
}

void GBGraphicsDriverDestroy(GBGraphicsDriver *this)
{
    GBSpriteRAMDestroy(this->oam);

    free(this);
}

bool __GBGraphicsDriverInstall(GBGraphicsDriver *this, struct __GBGameboy *gameboy)
{
    this->interruptRequest = &gameboy->cpu->ic->interruptFlagPort->value;
    this->oam->install(this->oam, gameboy);
    this->vram = gameboy->vram;

    this->control->driver = this;
    this->vram->driver = this;
    this->oam->driver = this;

    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->control);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->status);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->scrollX);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->scrollY);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->coordinate);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->compare);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->windowX);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->windowY);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->paletteBG);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->paletteSprite0);
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this->paletteSprite1);

    return true;
}

void __GBGraphicsDriverSetMode(GBGraphicsDriver *this, UInt8 mode)
{
    this->status->value &= 0xFC;
    this->status->value |= mode;

    this->driverMode = mode;

    switch (mode)
    {
        case kGBDriverStateVBlank: {
            if (this->status->value & kGBVideoInterruptVBlank)
                (*this->interruptRequest) |= (1 << kGBInterruptLCDStat);

            (*this->interruptRequest) |= (1 << kGBInterruptVBlank);
        } break;
        case kGBDriverStateHBlank: {
            if (this->status->value & kGBVideoInterruptHBlank)
                (*this->interruptRequest) |= (1 << kGBInterruptLCDStat);
        } break;
        case kGBDriverStateSpriteSearch: {
            if (this->status->value & kGBVideoInterruptSpriteSearch)
                (*this->interruptRequest) |= (1 << kGBInterruptLCDStat);
        }
    }
}

void __GBGraphicsDriverCheckCoincidence(GBGraphicsDriver *this)
{
    if (this->coordinate->value == this->compare->value) {
        this->status->value |= kGBVideoStatMatchFlag;

        if (this->status->value & kGBVideoInterruptOnLine)
            (*this->interruptRequest) |= (1 << kGBInterruptLCDStat);
    } else {
        this->status->value &= ~kGBVideoStatMatchFlag;
    }
}

void __GBGraphicsDriverTick(GBGraphicsDriver *this, UInt64 ticks)
{
    if (!this->displayOn)
        return;

    this->driverModeTicks++;

    switch (this->driverMode)
    {
        case kGBDriverStateSpriteSearch: {
            // Do a sprite every two ticks
            // This isn't *really* important, but I like it...
            // Note: This is a result of the fact that video RAM is clocked at 2 MHz
            if (this->driverModeTicks % 2)
            {
                if (this->lineSpriteCount == 10 || !(this->control->value & 1))
                    break;

                GBSpriteDescriptor *sprite = &this->oam->memory[this->spriteIndex];
                UInt8 spriteHeight = (this->control->value & 0x2) ? 16 : 8;
                UInt8 spriteWidth = 8;

                UInt8 spriteY = sprite->y + spriteHeight;
                UInt8 spriteX = sprite->x + spriteWidth;

                if (sprite->x <= this->driverX && this->driverX <= spriteX)
                    if (sprite->y <= this->coordinate->value && this->coordinate->value <= spriteY)
                        this->lineSprites[this->lineSpriteCount++] = this->spriteIndex;

                this->spriteIndex++;
            }

            if (this->driverModeTicks == kGBDriverSpriteSearchClocks)
            {
                __GBGraphicsDriverSetMode(this, kGBDriverStatePixelTransfer);
                this->driverModeTicks = 0;

                return;
            }
        } break;
        case kGBDriverStatePixelTransfer: {

            // TODO: Transfer pixels if we can
            this->fifoPosition++;

            if (this->fifoPosition == kGBScreenWidth)
            {
                __GBGraphicsDriverSetMode(this, kGBDriverStateHBlank);

                this->fifoPosition = 0;
                this->fifoSize = 0;

                return;
            }
        } break;
        case kGBDriverStateHBlank: {
            // Don't do anything...

            if (this->driverModeTicks == kGBDriverHorizonalClocks)
            {
                this->coordinate->value++;
                this->fifoPosition = 0;

                __GBGraphicsDriverCheckCoincidence(this);

                if (this->coordinate->value >= kGBScreenHeight) {
                    __GBGraphicsDriverSetMode(this, kGBDriverStateVBlank);
                } else {
                    __GBGraphicsDriverSetMode(this, kGBDriverStateSpriteSearch);
                }

                this->fetcherMode = kGBFetcherStateFetchTile;
                // TODO: Fetcher initial state

                this->driverModeTicks = 0;
                this->lineSpriteCount = 0;
                this->spriteIndex = 0;

                return;
            }
        } break;
        case kGBDriverStateVBlank: {
            // Just update the coordinate every "horizonal" clock
            if (this->driverModeTicks == kGBDriverVerticalClockUpdate)
            {
                this->driverModeTicks = 0;

                this->coordinate->value++;
            }

            if (this->coordinate->value > kGBCoordinateMaxY)
            {
                __GBGraphicsDriverSetMode(this, kGBDriverStateSpriteSearch);
                this->coordinate->value = 0;
            }

            __GBGraphicsDriverCheckCoincidence(this);
        } break;
        default: fprintf(stderr, "Emulator Error: Invalid LCD driver state '0x%02X'.\n", this->driverMode); break;
    }
}
