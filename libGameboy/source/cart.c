#include "cart.h"
#include "gameboy.h"
#include "mmu.h"
#include <stdio.h>

#ifdef __APPLE__
    #include <Security/Security.h>
#endif /* defined(__APPLE__) */

#pragma mark - ROM Structure

GBCartROM *GBCartROMCreateWithNullMapper(UInt8 *romData, UInt8 banks)
{
    GBCartROM *rom = malloc(sizeof(GBCartROM));

    if (rom)
    {
        rom->romData = malloc(banks * kGBMemoryBankSize);

        if (!rom->romData) {
            free(rom);
            return NULL;
        } else {
            memcpy(rom->romData, romData, banks * kGBMemoryBankSize);
        }

        rom->install = GBCartROMOnInstall;
        rom->eject = GBCartROMOnEject;

        rom->write = GBCartROMWriteNull;
        rom->read = GBCartROMReadDirect;

        rom->start = kGBCartROMBankLowStart;
        rom->end = kGBCartROMBankHighEnd;

        rom->maxBank = banks;
        rom->bank = 1;

        rom->installed = false;
    }

    return rom;
}

void GBCartROMDestroy(GBCartROM *this)
{
    if (this->installed)
        fprintf(stderr, "Warning: Cartridge ROM destroyed before cartridge successfully ejected!\n");

    free(this->romData);
    free(this);
}

void GBCartROMWriteNull(GBCartROM *this, UInt16 address, UInt8 byte)
{
    fprintf(stderr, "Warning: Attempting to write directly to ROM! (addr=0x%04X, byte=0x%02X)\n", address, byte);
}

UInt8 GBCartROMReadDirect(GBCartROM *this, UInt16 address)
{
    if (address <= kGBCartROMBankLowEnd) {
        return this->romData[address & (~kGBMemoryBankMask)];
    } else {
        UInt8 *bankStart = this->romData + (this->bank * kGBMemoryBankSize);

        return bankStart[address & (~kGBMemoryBankMask)];
    }
}

bool GBCartROMOnInstall(GBCartROM *this, GBGameboy *gameboy)
{
    if (this->installed)
        return true;

    bool success = GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);

    if (success) this->installed = true;
    return success;
}

bool GBCartROMOnEject(GBCartROM *this, GBGameboy *gameboy)
{
    if (!this->installed)
        return true;

    bool success = GBCartMemGenericOnEject((GBMemorySpace *)this, gameboy);

    if (success) this->install = false;
    return success;
}

#pragma mark - RAM Structure

GBCartRAM *GBCartRAMCreateWithNullMapper(UInt8 banks)
{
    GBCartRAM *ram = malloc(sizeof(GBCartRAM));

    if (ram)
    {
        ram->ramData = malloc(banks * kGBMemoryBankSize);

        if (!ram->ramData) {
            free(ram);
            return NULL;
        } else {
            #ifdef __APPLE__
                // This warns if we ignore the result implicitly
                __unused int result = SecRandomCopyBytes(kSecRandomDefault, banks * kGBMemoryBankSize, ram->ramData);
            #else /* !defined(__APPLE__) */
                bzero(ram->ramData, banks * kGBMemoryBankSize);
            #endif /* defined(__APPLE__) */
        }

        ram->install = GBCartRAMOnInstall;
        ram->eject = GBCartRAMOnEject;

        ram->write = GBCartRAMWriteDirect;
        ram->read = GBCartRAMReadDirect;

        ram->start = kGBCartRAMBankStart;
        ram->end = kGBCartRAMBankEnd;

        ram->maxBank = banks;
        ram->bank = 0;

        ram->installed = false;
    }

    return ram;
}

void GBCartRAMDestroy(GBCartRAM *this)
{
    if (this->installed)
        fprintf(stderr, "Warning: Cartridge RAM destroyed before cartridge successfully ejected!\n");

    free(this->ramData);
    free(this);
}

void GBCartRAMWriteDirect(GBCartRAM *this, UInt16 address, UInt8 byte)
{
    UInt8 *bankStart = this->ramData + (this->bank * kGBMemoryBankSize);

    bankStart[address & (~kGBMemoryBankMask)] = byte;
}

UInt8 GBCartRAMReadDirect(GBCartRAM *this, UInt16 address)
{
    UInt8 *bankStart = this->ramData + (this->bank * kGBMemoryBankSize);

    return bankStart[address & (~kGBMemoryBankMask)];
}

bool GBCartRAMOnInstall(GBCartRAM *this, GBGameboy *gameboy)
{
    if (this->installed)
        return true;

    bool success = GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);

    if (success) this->installed = true;
    return success;
}

bool GBCartRAMOnEject(GBCartRAM *this, GBGameboy *gameboy)
{
    if (!this->installed)
        return true;

    bool success = GBCartMemGenericOnEject((GBMemorySpace *)this, gameboy);

    if (success) this->install = false;
    return success;
}

#pragma mark - Cartridge Structure

GBCartridge *GBCartridgeCreate(UInt8 *romData, UInt32 romSize);

void GBCartridgeDestroy(GBCartridge *this)
{
    if (this->installed)
        fprintf(stderr, "Warning: Cartridge destroyed before being successfully ejected!\n");

    GBCartROMDestroy(this->rom);

    if (this->hasRAM)
        GBCartRAMDestroy(this->ram);

    free(this);
}

bool GBCartridgeInsert(GBCartridge *this, GBGameboy *gameboy)
{
    bool success = this->rom->install(this->rom, gameboy);

    if (!success)
        return false;

    if (this->hasRAM)
        success = this->ram->install(this->ram, gameboy);

    return success;
}

bool GBCartridgeEject(GBCartridge *this, GBGameboy *gameboy)
{
    bool success = this->rom->eject(this->rom, gameboy);

    if (!success)
        return false;

    if (this->hasRAM)
        success = this->ram->eject(this->ram, gameboy);

    return success;
}

bool GBCartMemGenericOnEject(GBMemorySpace *this, GBGameboy *gameboy)
{
    for (UInt8 i = 0; i < 0x10; i++)
    {
        if (gameboy->cpu->mmu->spaces[i] == (GBMemorySpace *)this)
            gameboy->cpu->mmu->spaces[i] = gGBMemorySpaceNull;

        if (gameboy->cpu->mmu->highSpaces[i] == (GBMemorySpace *)this)
            gameboy->cpu->mmu->highSpaces[i] = gGBMemorySpaceNull;
    }

    return true;
}
