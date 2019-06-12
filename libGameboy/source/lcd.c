#include "lcd.h"
#include "gameboy.h"
#include "mmio.h"
#include "mmu.h"
#include "ic.h"

#ifdef __APPLE__
    #include <Security/Security.h>
#endif /* defined(__APPLE__) */

void __GBGraphicsDriverVBlankReset(GBGraphicsDriver *this);
void __GBGraphicsDriverSetMode(GBGraphicsDriver *this, UInt8 mode);
void __GBGraphicsDriverCheckCoincidence(GBGraphicsDriver *this);

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
    bool wasOff = !(this->value >> 7);
    this->value = byte;

    if (!(byte >> 7) && !wasOff) {
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

        memset(this->driver->screenData, this->driver->nullColor, kGBScreenWidth * kGBScreenHeight * sizeof(UInt32));

        __GBGraphicsDriverVBlankReset(this->driver);
    } else if ((byte >> 7) && wasOff) {
        this->driver->driverMode = kGBDriverStateSpriteSearch;
        this->driver->driverModeTicks = 0;
        this->driver->displayOn = true;

        fprintf(stderr, "Note: Turned on display.\n");

        // TODO: Set display to lookup index 0
        memset(this->driver->screenData, 0x00, kGBScreenWidth * kGBScreenHeight * sizeof(UInt32));

        __GBGraphicsDriverVBlankReset(this->driver);
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

void __GBVideoPortGenericDebugWrite(GBVideoPortGeneric *reg, UInt8 byte)
{
    printf("Port 0x%04X: 0x%04X --> 0x%04X\n", reg->address, reg->value, byte);

    __GBIORegisterSimpleWrite((GBIORegister *)reg, byte);
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

        bzero(driver->screenData, kGBScreenWidth * kGBScreenHeight * sizeof(UInt32));

        driver->displayOn = false;

        driver->driverMode = kGBDriverStateVBlank;
        driver->driverModeTicks = 0;

        driver->linePointer = driver->screenData;
        driver->linePosition = 0;

        driver->fifoPosition = 0;
        driver->fifoSize = 0;

        driver->drawingWindow = false;

        driver->fetcherBase = 0x1C00;
        driver->fetcherPosition = 0;
        driver->fetcherOffset = 0;
        driver->fetcherMode = kGBFetcherStateFetchTile;

        driver->lineSpriteCount = 0;
        driver->spriteIndex = 0;

        driver->lineMod8 = 0;
        driver->driverX = 0;

        driver->colorLookup[0] = 0xEEEEEEEE; // white
        driver->colorLookup[1] = 0xBBBBBBBB; // light gray
        driver->colorLookup[2] = 0x55555555; // dark gray
        driver->colorLookup[3] = 0x00000000; // black

        driver->nullColor = 0x00000000;

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

void __GBGraphicsDriverVBlankReset(GBGraphicsDriver *this)
{
    this->fetcherBase = (this->control->value & 0x08) ? 0x1C00 : 0x1800;
    this->fetcherPosition = (this->scrollY->value / kGBTileHeight) * kGBMapWidth;
    this->linePointer = this->screenData;

    this->lineMod8 = this->scrollY->value % 8;
    this->coordinate->value = 0;

    this->fetcherOffset = 0;
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
            // Read the y position on the first tick and the x position on the second tick
            // I'm pretty sure this is a more accurate way of doing this, but I'm not sure tbh
            if ((this->driverModeTicks & 1)) {
                // Just read the y position
                this->nextSpriteY = this->oam->memory[this->spriteIndex].y;
            } else {
                this->nextSpriteX = this->oam->memory[this->spriteIndex].x;

                if (this->lineSpriteCount == 10 || !(this->control->value & 1)) {
                    this->spriteIndex++;
                } else {
                    UInt8 spriteHeight = (this->control->value & 0x2) ? 8 : 16;
                    UInt16 coordinate = this->coordinate->value + 16;

                    UInt8 spriteBottomY = this->nextSpriteY + spriteHeight;
                    UInt8 spriteTopY = this->nextSpriteY;

                    UInt8 spriteX = this->nextSpriteX - 8;

                    if (spriteX && spriteX < 160)
                        if (spriteTopY <= coordinate && coordinate < spriteBottomY)
                            this->lineSprites[this->lineSpriteCount++] = this->spriteIndex;

                    this->spriteIndex++;
                }
            }

            if (this->spriteIndex == 40)
            {
                if (this->driverModeTicks != kGBDriverSpriteSearchClocks)
                    fprintf(stderr, "Error: Sprite search loop timing off!! (Completed in %d ticks [Should be %d!!])\n", this->driverModeTicks, kGBDriverSpriteSearchClocks);

                __GBGraphicsDriverSetMode(this, kGBDriverStatePixelTransfer);

                this->driverModeTicks = 0;
                this->spriteIndex = 0;

                return;
            }
        } break;
        case kGBDriverStatePixelTransfer: {
            // Fetch steps:
            // 1. Read tile (pointer already into bg map)
            // 2. Read data byte 0 (from tile offset in step 1)
            // 3. Read data byte 1
            //    --> Increase offset by 1
            //    --> Push 8 new pixels
            // 4. Idle (until FIFO has space)

            // Note: When we reach screen width, we reset the fetcher and the FIFO buffer and continue
            // Note: When we reach window x position, FIFO is cleared and the offset saved is overwritten by the correct offset for the window start.

            // Note: Format of FIFO/Fetcher is such that each stores the two bits of a given pixel with the palette used (here just paletteBG, etc.)

            // TODO: Transfer pixels if we can
            //if (this->screenIndex >= (kGBScreenHeight * kGBScreenWidth))
            //    this->screenIndex = 0;

            //this->screenData[this->screenIndex++] += 0x01010100;
            //this->fifoPosition++;

            // control & 0x10; highmap

            // FIFO
            if (true)
            {
                // FIFO only runs with over 8 pixels
                if (this->fifoSize > 8)
                {
                    UInt8 nextPixel = this->fifoBuffer[this->fifoPosition++];
                    UInt8 palette = nextPixel & 0x0F;
                    UInt8 color = nextPixel >> 4;

                    // We have to figure out the pixel's RGB by looking it up in the palette and the current color map
                    UInt8 trueColor;

                    switch (palette)
                    {
                        case 1:  trueColor = this->paletteSprite0->value >> (2 * color); break;
                        case 2:  trueColor = this->paletteSprite1->value >> (2 * color); break;
                        default: trueColor = this->paletteBG->value >> (2 * color);      break;
                    }

                    trueColor &= 0x3;

                    // Finally lookup based on selected RGB values
                    UInt32 nextPixelRGB = this->colorLookup[trueColor];

                    /*if (!(this->linePosition % 8) || !(this->coordinate->value % 8))
                        nextPixelRGB = 0xFF0000FF;*/

                    /*if (trueColor != 0)
                        printf("Calculated RGB 0x%06X for pixel %d%d with mapped value %d%d\n", nextPixelRGB, color >> 1, color & 1, trueColor >> 1, trueColor & 1);*/

                    // Output only if we've discarded enough pixels to get to the starting x position
                    if (!(this->driverX < this->scrollX->value))
                    {
                        if (this->spriteIndex < this->lineSpriteCount)
                        {
                            UInt16 index = this->lineSprites[this->spriteIndex];

                            GBSpriteDescriptor *nextSprite = &this->oam->memory[index];
                            UInt8 position = this->linePosition;

                            UInt8 spriteRightX = nextSprite->x;
                            UInt8 spriteLeftX = nextSprite->x - 8;

                            if (spriteLeftX <= position) {
                                //if (nextSprite->x == this->linePosition && this->coordinate->value == nextSprite->y)
                                //    printf("Next sprite at (%d, %d)\n", nextSprite->x, nextSprite->y);

                                this->linePointer[this->linePosition++] = 0xFF000000;
                            } else {
                                this->linePointer[this->linePosition++] = nextPixelRGB;
                            }

                            if (this->linePosition >= spriteRightX)
                                this->spriteIndex++;
                        } else {
                            this->linePointer[this->linePosition++] = nextPixelRGB;
                        }
                    }

                    this->fifoSize--;
                    this->driverX++;

                    // And then wrap position when it overflows
                    if (this->fifoPosition == 16)
                        this->fifoPosition = 0;

                    // We clear FIFO and change to the window when we reach its x position
                    /*if (!this->drawingWindow && this->control->value & 0x20) // Check if window enabled
                    {
                        if (this->coordinate->value >= this->windowY->value && this->driverX >= (this->windowX->value + 7))
                        {
                            // 'clear' the FIFO buffer
                            this->fifoPosition = 0;
                            this->fifoSize = 0;

                            // Set fetcher offset to the start of the window (on the given row)
                            UInt8 currentWindowRow = this->coordinate->value - this->windowY->value;

                            this->fetcherBase = (this->control->value & 0x40) ? 0x1C00 : 0x1800;
                            this->fetcherPosition = (currentWindowRow / kGBTileHeight) * kGBMapWidth;
                            this->fetcherOffset = 0;

                            this->drawingWindow = true;
                        }
                    }*/
                }
            }

            // Fetcher
            if (this->driverModeTicks & 1)
            {
                // We don't need complex memory checking rules here.
                // In a real gameboy, the video memory is hooked up directly to the screen driver
                // So there isn't really any possibility for this to fail unless the hardware itself fails.
                switch (this->fetcherMode)
                {
                    case kGBFetcherStateFetchTile: {
                        this->fetcherTile = this->vram->memory[this->fetcherBase + this->fetcherPosition + this->fetcherOffset++];

                        if (!(this->control->value & 0x10))
                            this->fetcherTile += 0x80;

                        /*if (this->fetcherTile)
                            printf("Fetcher: Tile 0x%02X read for position 0x%04X at offset 0x%04X [base: 0x%04X]\n", this->fetcherTile, this->fetcherPosition, this->fetcherOffset - 1, this->fetcherBase);*/

                        for (UInt8 i = 0; i < 8; i++)
                            this->fetchBuffer[i] = 0 /* palette index */;

                        this->fetcherMode = kGBFetcherStateFetchByte0;
                    } break;
                    case kGBFetcherStateFetchByte0: {
                        UInt16 tileset = (this->control->value & 0x10) ? 0x0000 : 0x0800;

                        UInt16 address = tileset + ((2 * kGBTileHeight) * this->fetcherTile) + (2 * this->lineMod8) + 0;

                        this->fetcherByte0 = this->vram->memory[address];

                        /*if (this->fetcherTile)
                            printf("Tile 0x%02X row 0x%02X byte 0 read as 0x%02X (tileset at 0x8%03X)\n", this->fetcherTile, this->lineMod8, this->fetcherByte0, tileset);*/

                        // Push back first byte of each pixel (into top nibble)
                        for (UInt8 i = 0; i < 8; i++)
                            this->fetchBuffer[i] |= ((this->fetcherByte0 >> (7 - i)) & 1) << 4;

                        this->fetcherMode = kGBFetcherStateFetchByte1;
                    } break;
                    case kGBFetcherStateFetchByte1: {
                        UInt16 tileset = (this->control->value & 0x10) ? 0x0000 : 0x0800;

                        UInt16 address = tileset + ((2 * kGBTileHeight) * this->fetcherTile) + (2 * this->lineMod8) + 1;

                        this->fetcherByte1 = this->vram->memory[address];

                        // Push back second byte of each pixel (into top nibble)
                        for (UInt8 i = 0; i < 8; i++)
                        {
                            this->fetchBuffer[i] |= ((this->fetcherByte1 >> (7 - i)) & 1) << 5;

                            /*if (this->fetcherTile)
                                printf("Tile 0x%02X row 0x%02X pixel %d decoded with value %d\n", this->fetcherTile, this->lineMod8, i, (this->fetchBuffer[i] >> 4));*/
                        }

                        // The fetcher wraps over x
                        if (this->fetcherOffset >= 32)
                            this->fetcherOffset = 0;

                        this->fetcherMode = kGBFetcherStateStall;
                    } break;
                    case kGBFetcherStateStall: {
                        if (this->fifoSize <= 8)
                        {
                            // Buffer is already prepared. Simply place it in the FIFO buffer.
                            if (this->fifoSize == 0) {
                                // Special case. Normally we don't want to overwrite the data in the buffer, but here the FIFO was just cleared or we're at the start of a line.
                                memcpy(this->fifoBuffer, this->fetchBuffer, 8);
                            } else {
                                // Copy to wherever we have 8 spaces.
                                UInt8 copyOffset = (this->fifoPosition < 8) ? 8 : 0;

                                memcpy(this->fifoBuffer + copyOffset, this->fetchBuffer, 8);
                            }

                            /*if (this->fetcherTile)
                            {
                                printf("Pushed back the following 8 pixels to FIFO: {");

                                for (SInt8 i = 7; i >= 0; i--)
                                {
                                    printf("0x%02X", this->fetchBuffer[i]);
                                    if (i) printf(", ");
                                }

                                printf("}\n");
                            }*/

                            this->fifoSize += 8;
                            this->fetcherMode = kGBFetcherStateFetchTile;
                        }
                    } break;
                }
            }

            if (this->linePosition == kGBScreenWidth)
            {
                __GBGraphicsDriverSetMode(this, kGBDriverStateHBlank);

                return;
            }
        } break;
        case kGBDriverStateHBlank: {
            // Don't do anything...

            if (this->driverModeTicks == kGBDriverHorizonalClocks)
            {
                this->coordinate->value++;

                __GBGraphicsDriverCheckCoincidence(this);

                if (this->coordinate->value >= kGBScreenHeight) {
                    __GBGraphicsDriverSetMode(this, kGBDriverStateVBlank);
                } else {
                    __GBGraphicsDriverSetMode(this, kGBDriverStateSpriteSearch);
                }

                this->driverModeTicks = 0;
                this->lineSpriteCount = 0;
                this->spriteIndex = 0;

                this->linePointer += kGBScreenWidth;
                this->linePosition = 0;

                this->fifoPosition = 0;
                this->fifoSize = 0;

                this->drawingWindow = false;

                this->driverX = 0;

                this->lineMod8++;
                this->lineMod8 %= 8;

                this->fetcherMode = kGBFetcherStateFetchTile;
                this->fetcherOffset = 0;

                // We need another row of tiles
                if (!this->lineMod8)
                {
                    this->fetcherPosition += kGBMapWidth;

                    if (this->fetcherPosition >= (kGBMapWidth * kGBMapHeight))
                        this->fetcherPosition = 0;
                }

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

                __GBGraphicsDriverVBlankReset(this);
            }

            __GBGraphicsDriverCheckCoincidence(this);
        } break;
        default: fprintf(stderr, "Emulator Error: Invalid LCD driver state '0x%02X'.\n", this->driverMode); break;
    }
}
