#include <libgb/gameboy.h>
#include <strings.h>
#include <stdlib.h>

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

void __GBHighRAMWrite(GBHighRAM *this, uint16_t address, uint8_t byte)
{
    /*uint16_t addr = address & (~kGBHighRAMStart);

    // 0xFA --> 0xFFA6
    // 0x25 --> 0xFFE1

    if (address == 0xFFA6)
        fprintf(stdout, "HRAM: Writing 0x%02X to 0x%04X (0x%04X)\n", byte, address, addr);

    if (address == 0xFFE1)
        fprintf(stdout, "HRAM: Changing 0x%04X from 0x%02X to 0x%02X\n", address, this->memory[addr], byte);*/

    //fprintf(stdout, "HRAM: Writing 0x%02X to 0x%04X (0x%04X)\n", byte, address, addr);

    this->memory[address & (~kGBHighRAMStart)] = byte;
}

uint8_t __GBHighRAMRead(GBHighRAM *this, uint16_t address)
{
    //uint16_t addr = address & (~kGBHighRAMStart);

    //fprintf(stdout, "HRAM: Read byte 0x%02X rfom 0x%04X (0x%04X)\n", this->memory[addr], address, addr);

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

void __GBWorkRAMWrite(GBWorkRAM *this, uint16_t address, uint8_t byte)
{
    this->memory[address & (~0xC000)] = byte;
}

uint8_t __GBWorkRAMRead(GBWorkRAM *this, uint16_t address)
{
    return this->memory[address & (~0xC000)];
}

bool __GBWorkRAMOnInstall(GBWorkRAM *this, GBGameboy *gameboy)
{
    bool success = GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
    success &= GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this->hram);

    return success;
}
