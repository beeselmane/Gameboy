#include "wram.h"
#include "gameboy.h"
#include "mmu.h"

#ifdef __APPLE__
    #include <Security/Security.h>
#endif /* defined(__APPLE__) */

GBWorkRAM *GBWorkRAMCreate(void)
{
    GBWorkRAM *ram = malloc(sizeof(GBWorkRAM));

    if (ram)
    {
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
    return GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
}
