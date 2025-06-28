#include <libgb/gameboy.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
    #include <Security/Security.h>
#endif /* defined(__APPLE__) */

#pragma mark - ROM Structure

GBCartROM *GBCartROMCreateGeneric(uint8_t *romData, uint8_t banks, uint8_t (*readFunc)(struct __GBCartROM *this, uint16_t address), void (*writeFunc)(struct __GBCartROM *this, uint16_t address, uint8_t byte))
{
    GBCartROM *rom = malloc(sizeof(GBCartROM));
    uint16_t bankSize = kGBMemoryBankSize * 4;

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

        rom->write = writeFunc;
        rom->read = readFunc;

        rom->start = kGBCartROMBankLowStart;
        rom->end = kGBCartROMBankHighEnd;

        rom->maxBank = banks;
        rom->bank = 1;

        rom->installed = false;
    }

    return rom;
}

GBCartROM *GBCartROMCreateWithNullMapper(uint8_t *romData, uint8_t banks)
{
    return GBCartROMCreateGeneric(romData, banks, GBCartROMReadBanked, GBCartROMWriteNull);
}

GBCartROM *GBCartROMCreateWithMBC1(uint8_t *romData, uint8_t banks)
{
    return GBCartROMCreateGeneric(romData, banks, GBCartROMReadBanked, GBCartROMWriteMBC1);
}

GBCartROM *GBCartROMCreateWithMBC2(uint8_t *romData, uint8_t banks);

GBCartROM *GBCartROMCreateWithMBC3(uint8_t *romData, uint8_t banks);

GBCartROM *GBCartROMCreateWithMBC4(uint8_t *romData, uint8_t banks);

GBCartROM *GBCartROMCreateWithMBC5(uint8_t *romData, uint8_t banks);

void GBCartROMDestroy(GBCartROM *this)
{
    if (this->installed)
        fprintf(stderr, "Warning: Cartridge ROM destroyed before cartridge successfully ejected!\n");

    free(this->romData);
    free(this);
}

void GBCartROMWriteNull(GBCartROM *this, uint16_t address, uint8_t byte)
{
    fprintf(stderr, "Warning: Attempting to write directly to ROM! (addr=0x%04X, byte=0x%02X)\n", address, byte);
}

void GBCartROMWriteMBC1(GBCartROM *this, uint16_t address, uint8_t byte)
{
    //
}

uint8_t GBCartROMReadBanked(GBCartROM *s, uint16_t address)
{
    if (address <= kGBCartROMBankLowEnd) {
        return s->romData[address];
    } else {
        uint8_t *bankStart = s->romData + (s->bank * (kGBMemoryBankSize * 4));

        return bankStart[address - 0x4000];
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

GBCartRAM *GBCartRAMCreateWithBanks(uint8_t banks)
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

        ram->enabled = true;
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

void GBCartRAMWriteDirect(GBCartRAM *this, uint16_t address, uint8_t byte)
{
    if (!this->enabled)
        return;

    uint8_t *bankStart = this->ramData + (this->bank * (kGBMemoryBankSize * 2));

    bankStart[address & (~0xA0)] = byte;
}

uint8_t GBCartRAMReadDirect(GBCartRAM *this, uint16_t address)
{
    if (!this->enabled)
        return 0xFF;

    uint8_t *bankStart = this->ramData + (this->bank * (kGBMemoryBankSize * 2));

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

#pragma mark - Cartridge Info Methods

GBCartInfo *GBCartInfoCreateWithHeader(GBCartHeader *header, bool validate)
{
    GBCartInfo *info = malloc(sizeof(GBCartInfo));

    if (info)
    {
        if (header->licenseCode == 0x33) {
            memcpy(info->title, header->title, 11);
            memcpy(info->maker, header->maker, 4);

            info->title[11] = 0;
        } else {
            memcpy(info->title, &header->title, 16);
            info->title[16] = 0;
        }

        if (validate)
        {
            uint32_t skipIndex = (uint32_t)(&header->headerChecksum - ((uint8_t *)header));
            uint8_t sum = 0;

            for (uint8_t i = 0; i < sizeof(GBCartHeader); i++)
            {
                if (i == skipIndex)
                    continue;

                sum += ((uint8_t *)header)[i];
            }

            if (sum != header->headerChecksum)
            {
                fprintf(stderr, "Error: Header checksum invalid for cartridge '%s'! (Expected 0x%02X, Calculated 0x%02X)\n", info->title, header->headerChecksum, sum);

                free(info);
                return NULL;
            }
        }

        info->mbcType = kGBCartMBCTypeNone;
        info->hasBattery = false;
        info->hasRumble = false;
        info->hasTimer = false;

        switch (header->type)
        {
            case 0x00:
                info->mbcType = kGBCartMBCTypeNone;
            break;

            case 0x03:
                info->hasBattery = true;
            case 0x02:
            case 0x01:
                info->mbcType = kGBCartMBCType1;
            break;

            case 0x06:
                info->hasBattery = true;
            case 0x05:
                info->mbcType = kGBCartMBCType2;
            break;

            case 0x09:
                info->hasBattery = true;
            case 0x08:
                info->mbcType = kGBCartMBCTypeNone;
            break;

            case 0x0D:
                info->hasBattery = true;
            case 0x0C:
            case 0x0B:
                info->mbcType = kGBCartMBCTypeMMM01;
            break;

            case 0x0F:
            case 0x10:
                info->hasTimer = true;
            case 0x13:
                info->hasBattery = true;
            case 0x12:
            case 0x11:
                info->mbcType = kGBCartMBCType3;
            break;

            case 0x17:
                info->hasBattery = true;
            case 0x16:
            case 0x15:
                info->mbcType = kGBCartMBCType4;
            break;

            case 0x1E:
                info->hasBattery = true;
            case 0x1D:
            case 0x1C:
                info->hasRumble = true;
                info->mbcType = kGBCartMBCType5;
            break;

            case 0x1B:
                info->hasBattery = true;
            case 0x1A:
            case 0x19:
                info->mbcType = kGBCartMBCType5;
            break;
        }

        if (header->romSize <= 0x07) {
            info->romSize = (32 * 0x1000) << header->romSize; // 32 KB << header->romSize
        } else {
            switch (header->romSize)
            {
                case 0x52: info->romSize = 72 * (2 * kGBMemoryBankSize); break; // 72 banks
                case 0x53: info->romSize = 80 * (2 * kGBMemoryBankSize); break; // 80 banks
                case 0x54: info->romSize = 96 * (2 * kGBMemoryBankSize); break; // 96 banks
                default:
                    fprintf(stderr, "Warning: Unknown ROM size '0x%02X' in cart '%s'.\n", header->romSize, info->title);
                    info->romSize = -1; // This means we were unable to determine ROM size.
                    // The correct ROM size can be found by the parameter passed into GBCartridgeCreate after we return from this function.
                break;
            }
        }

        if (info->mbcType == kGBCartMBCType2) {
            info->ramSize = 4 * 512;
        } else {
            switch (header->ramSize)
            {
                case 0: info->ramSize = 0; break;
                case 1:
                    info->ramSize = 2 * 0x1000; // 2 KB
                break;
                case 2:
                    info->ramSize = 8 * 0x1000; // 8 KB
                break;
                case 3:
                    info->ramSize = 4 * 8 * 0x1000; // 32 KB
                break;
                default:
                    fprintf(stderr, "Warning: Unknown RAM size '0x%02X' in cart '%s'.\n", header->ramSize, info->title);
                    info->ramSize = 0;
                break;
            }
        }

        info->romChecksum = (header->checksum >> 8) | (header->checksum << 8);
        info->isJapanese = !header->destination;
        info->romVersion = header->version;
    }

    return info;
}

void GBCartInfoDestroy(GBCartInfo *this)
{
    free(this);
}

#pragma mark - Cartridge Structure

GBCartridge *GBCartridgeCreate(uint8_t *romData, uint32_t romSize)
{
    GBCartridge *cartridge = malloc(sizeof(GBCartridge));

    if (cartridge)
    {
        GBCartHeader *header = (GBCartHeader *)(romData + kGBCartHeaderStart);

        cartridge->info = GBCartInfoCreateWithHeader(header, true);

        if (!cartridge->info)
        {
        fprintf(stderr, "Error: Attempted to load invalid cartridge (Or out of memory)!\n");
            free(cartridge);

            return NULL;
        }

        if (cartridge->info->romSize == -1)
        {
            // There was an invalid value in the header.
            // We just take the largest multiple of the block size passed into this function.
            cartridge->info->romSize = romSize & 0x3FFF;
        }

        if (cartridge->info->romSize < romSize)
        {
            fprintf(stderr, "Error: Cartridge ROM size smaller than data given!\n");

            GBCartInfoDestroy(cartridge->info);
            free(cartridge);

            return NULL;
        }

        uint8_t romBanks = cartridge->info->romSize / (4 * kGBMemoryBankSize);
        uint8_t ramBanks = cartridge->info->ramSize / (2 * kGBMemoryBankSize);

        if (cartridge->info->ramSize) {
            cartridge->ram = GBCartRAMCreateWithBanks(ramBanks);

            if (!cartridge->ram)
            {
                GBCartInfoDestroy(cartridge->info);
                free(cartridge);

                return NULL;
            }
        } else {
            cartridge->ram = NULL;
        }

        switch (cartridge->info->mbcType)
        {
            case kGBCartMBCTypeNone:
                cartridge->rom = GBCartROMCreateWithNullMapper(romData, romBanks);
            break;
            case kGBCartMBCType1:
                cartridge->rom = GBCartROMCreateWithMBC1(romData, romBanks);

                cartridge->ram->enabled = false;
            break;
            default: cartridge->rom = NULL; break;
        }

        if (!cartridge->rom)
        {
            GBCartInfoDestroy(cartridge->info);

            if (cartridge->info->ramSize)
                GBCartRAMDestroy(cartridge->ram);

            free(cartridge);

            return NULL;
        }

        cartridge->installed = false;
    }

    return cartridge;
}

GBCartHeader *GBCartridgeGetHeader(GBCartridge *cart)
{
    return (GBCartHeader *)(cart->rom->romData + kGBCartHeaderStart);
}

bool GBCartridgeChecksumIsValid(GBCartridge *this)
{
    uint16_t *checksum = &((GBCartHeader *)(this->rom->romData + kGBCartHeaderStart))->checksum;
    uint16_t offset = (uint16_t)(((void *)checksum) - (void *)this->rom->romData);
    uint16_t sum = 0;

    assert(offset == 0x014E);

    for (uint32_t i = 0; i < this->info->romSize; i++)
    {
        if (i == offset || i == (offset + 1))
            continue;

        sum += this->rom->romData[i];
    }

    fprintf(stdout, "Checksum calculated for cart '%s'. Expected 0x%04X, Calculated 0x%04X.\n", this->info->title, this->info->romChecksum, sum);

    return (sum == this->info->romChecksum);
}

void GBCartridgeDestroy(GBCartridge *this)
{
    if (this->installed)
        fprintf(stderr, "Warning: Cartridge destroyed before being successfully ejected!\n");

    GBCartROMDestroy(this->rom);

    if (this->info->ramSize)
        GBCartRAMDestroy(this->ram);

    GBCartInfoDestroy(this->info);
    free(this);
}

bool GBCartridgeUnmap(GBCartridge *this, GBGameboy *gameboy)
{
    bool success = this->rom->eject(this->rom, gameboy);

    if (!success)
        return false;

    if (this->info->ramSize)
        success = this->ram->eject(this->ram, gameboy);

    return success;
}

bool GBCartridgeMap(GBCartridge *this, GBGameboy *gameboy)
{
    bool success = this->rom->install(this->rom, gameboy);

    if (!success)
        return false;

    if (this->info->ramSize)
        success = this->ram->install(this->ram, gameboy);

    return success;
}

bool GBCartMemGenericOnEject(GBMemorySpace *this, GBGameboy *gameboy)
{
    for (uint8_t i = 0; i < 0x10; i++)
    {
        if (gameboy->cpu->mmu->spaces[i] == (GBMemorySpace *)this)
            gameboy->cpu->mmu->spaces[i] = gGBMemorySpaceNull;

        if (gameboy->cpu->mmu->highSpaces[i] == (GBMemorySpace *)this)
            gameboy->cpu->mmu->highSpaces[i] = gGBMemorySpaceNull;
    }

    return true;
}
