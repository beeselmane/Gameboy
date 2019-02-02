#include "wram.h"
#include "gameboy.h"
#include "mmu.h"

#ifdef __APPLE__
    #include <Security/Security.h>
#endif /* defined(__APPLE__) */

#pragma mark - High RAM

GBHighRAM *GBHighRAMCreate(void)
{
    GBHighRAM *ram = malloc(sizeof(GBHighRAM));

    if (ram)
    {
        // High RAM gets installed with work RAM.
        ram->install = (void *)__GBWorkRAMOnInstall;

        ram->write = __GBHighRAMWrite;
        ram->read = __GBHighRAMRead;

        ram->start = kGBHighRAMStart;
        ram->end = kGBHighRAMEnd;

        #ifdef __APPLE__
            // This warns if we ignore the result implicitly
            __unused int result = SecRandomCopyBytes(kSecRandomDefault, kGBHighRAMSize, ram->memory);
        #else /* !defined(__APPLE__) */
            bzero(ram->memory, kGBWorkRAMSize);
        #endif /* defined(__APPLE__) */
    }

    return ram;
}

void GBHighRAMDestroy(GBHighRAM *this)
{
    free(this);
}

void __GBHighRAMWrite(GBHighRAM *this, UInt16 address, UInt8 byte)
{
    this->memory[address & (~kGBHighRAMStart)] = byte;
}

UInt8 __GBHighRAMRead(GBHighRAM *this, UInt16 address)
{
    return this->memory[address & (~kGBHighRAMStart)];
}

#pragma mark - Work RAM

GBWorkRAM *GBWorkRAMCreate(void)
{
    GBWorkRAM *ram = malloc(sizeof(GBWorkRAM));

    if (ram)
    {
        ram->hram = GBHighRAMCreate();

        if (!ram->hram)
        {
            free(ram);

            return NULL;
        }

        ram->install = __GBWorkRAMOnInstall;

        ram->write = __GBWorkRAMWrite;
        ram->read = __GBWorkRAMRead;

        ram->start = kGBWorkRAMStart;
        ram->end = kGBWorkRAMEnd;

        #ifdef __APPLE__
            // This warns if we ignore the result implicitly
            __unused int result = SecRandomCopyBytes(kSecRandomDefault, kGBWorkRAMSize, ram->memory);
        #else /* !defined(__APPLE__) */
            bzero(ram->memory, kGBWorkRAMSize);
        #endif /* defined(__APPLE__) */
    }

    return ram;
}

void GBWorkRAMDestroy(GBWorkRAM *this)
{
    free(this);
}

void __GBWorkRAMWrite(GBWorkRAM *this, UInt16 address, UInt8 byte)
{
    this->memory[address & (~kGBMemoryBankMask)] = byte;
}

UInt8 __GBWorkRAMRead(GBWorkRAM *this, UInt16 address)
{
    return this->memory[address & (~kGBMemoryBankMask)];
}

bool __GBWorkRAMOnInstall(GBWorkRAM *this, GBGameboy *gameboy)
{
    bool success = GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
    success &= GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this->hram);

    return success;
}
