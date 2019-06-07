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
    UInt16 bankSize = kGBMemoryBankSize * 4;

    if (rom)
    {
        rom->romData = malloc(banks * bankSize);

        if (!rom->romData) {
            free(rom);
            return NULL;
        } else {
            memcpy(rom->romData, romData, banks * bankSize);
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

UInt8 GBCartROMReadDirect(GBCartROM *s, UInt16 address)
{
    if (address <= kGBCartROMBankLowEnd) {
        return s->romData[address];
    } else {
        UInt8 *bankStart = s->romData + (s->bank * (kGBMemoryBankSize * 4));
        UInt16 offset = 0x0FFF & address;


        switch (address & 0xF000)
        {
            case 0x4000: return bankStart[0x0000 | offset];
            case 0x5000: return bankStart[0x1000 | offset];
            case 0x6000: return bankStart[0x2000 | offset];
            case 0x7000: return bankStart[0x3000 | offset];
            default:     return 0xFF;
        }
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
        ram->ramData = malloc(banks * (kGBMemoryBankSize * 2));

        if (!ram->ramData) {
            free(ram);
            return NULL;
        } else {
            #ifdef __APPLE__
                // This warns if we ignore the result implicitly
                __unused int result = SecRandomCopyBytes(kSecRandomDefault, banks * (kGBMemoryBankSize * 2), ram->ramData);
            #else /* !defined(__APPLE__) */
                bzero(ram->ramData, banks * (kGBMemoryBankSize * 2));
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
    UInt8 *bankStart = this->ramData + (this->bank * (kGBMemoryBankSize * 2));

    bankStart[address & (~0xA0)] = byte;
}

UInt8 GBCartRAMReadDirect(GBCartRAM *this, UInt16 address)
{
    UInt8 *bankStart = this->ramData + (this->bank * (kGBMemoryBankSize * 2));

    return bankStart[address & (~0xA0)];
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

GBCartridge *GBCartridgeCreate(UInt8 *romData, UInt32 romSize)
{
    GBCartridge *cartridge = malloc(sizeof(GBCartridge));

    if (cartridge)
    {
        GBGameHeader *header = (GBGameHeader *)(romData + 0x100);
        UInt16 checksum = 0;

        if (header->licenseCode == 0x33) {
            memcpy(cartridge->title, header->title, 11);
            cartridge->title[11] = 0;

            fprintf(stdout, "Info: Loading cartridge for '%s'\n", cartridge->title);
            fprintf(stdout, "Info: Manufacturing code: %c%c%c%c\n",
                    header->maker[0], header->maker[1], header->maker[2], header->maker[3]);
        } else {
            memcpy(cartridge->title, &header->title, 16);
            cartridge->title[16] = 0;

            fprintf(stdout, "Info: Loading cartridge for '%s'\n", cartridge->title);
        }

        for (UInt32 i = 0; i < romSize; i++)
        {
            if (i == 0x014E || i == 0x014F)
                continue;

            checksum += romData[i];
        }

        checksum = ((checksum & 0xFF) << 8) | (checksum >> 8);

        if (checksum == header->checksum)
            fprintf(stdout, "Info: Checksum okay.\n");
        else
            fprintf(stdout, "Warning: Checksum invalid! (expected 0x%04X, calculated 0x%04X)\n", header->checksum, checksum);

        if (header->type != 0x00)
        {
            fprintf(stderr, "Error: Unsupported cart. type\n");
            free(cartridge);

            return NULL;
        }

        if (header->romSize > 0x07) {
            fprintf(stderr, "Error: Unknown ROM size.\n");
            free(cartridge);

            return NULL;
        } else {
            cartridge->rom = GBCartROMCreateWithNullMapper(romData, 1 + (1 << header->romSize));

            if (!cartridge->rom)
            {
                free(cartridge);

                return NULL;
            }
        }

        cartridge->hasRAM = false;
        cartridge->ram = NULL;

        cartridge->mbcType = 0x00;
        cartridge->installed = false;
    }

    return cartridge;
}

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
