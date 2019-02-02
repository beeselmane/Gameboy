#include "lcd.h"
#include "gameboy.h"
#include "mmu.h"

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
    this->memory[address & (~kGBMemoryBankMask)] = byte;
}

UInt8 __GBVideoRAMRead(GBVideoRAM *this, UInt16 address)
{
    return this->memory[address & (~kGBMemoryBankMask)];
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
    ((UInt8 *)this->memory)[address & (~kGBSpriteRAMStart)] = byte;
}

UInt8 __GBSpriteRAMRead(GBSpriteRAM *this, UInt16 address)
{
    return ((UInt8 *)this->memory)[address & (~kGBSpriteRAMStart)];
}

bool __GBSpriteRAMOnInstall(GBSpriteRAM *this, GBGameboy *gameboy)
{
    return GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
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
    this->oam->install(this->oam, gameboy);

    this->vram = gameboy->vram;

    return true;
}
