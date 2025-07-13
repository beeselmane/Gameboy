#include <libgb/gameboy.h>
#include <stdlib.h>
#include <stdio.h>

#pragma mark - Empty Space

GBMemorySpace *gGBMemorySpaceNull;

void __GBMemorySpaceNullWrite(GBMemorySpace *space, uint16_t address, uint8_t byte)
{
    fprintf(stderr, "Warning: Attempted write to unmapped address '0x%04X' (byte=0x%02X)\n", address, byte);
}

uint8_t __GBMemorySpaceNullRead(GBMemorySpace *space, uint16_t address)
{
    fprintf(stderr, "Warning: Attempted read to unmapped address '0x%04X'\n", address);
    return 0xFF;
}

#pragma mark - Memory Manager

// ROM is masked unless it is installed
bool gGBMemoryManagerROMDefault = true;

GBMemoryManager *GBMemoryManagerCreate(void)
{
    GBMemoryManager *mmu = malloc(sizeof(GBMemoryManager));

    if (mmu)
    {
        for (uint8_t i = 0; i < 0x10; i++)
            mmu->highSpaces[i] = gGBMemorySpaceNull;

        for (uint8_t i = 0; i < 0x10; i++)
            mmu->spaces[i] = gGBMemorySpaceNull;

        mmu->romMasked = &gGBMemoryManagerROMDefault;
        mmu->romSpace = gGBMemorySpaceNull;

        mmu->install = NULL;

        mmu->isWrite = false;
        mmu->accessed = NULL;
        mmu->mdr = NULL;
        mmu->mar = NULL;

        mmu->tick = __GBMemoryManagerTick;
    }

    return mmu;
}

bool GBMemoryManagerInstallSpace(GBMemoryManager *this, GBMemorySpace *space)
{
    fprintf(stdout, "Note: Installed 0x%04X --> 0x%04X\n", space->start, space->end);

    if (space->end < 0xFE00) {
        if (space->start & 0x0FFF || ~space->end & 0x0FFF)
            return false;

        uint16_t realEnd = (space->end & 0xF000) >> 12;
        uint16_t realStart = space->start >> 12;

        for (uint8_t i = realStart; i <= realEnd; i++)
            this->spaces[i] = space;

        return true;
    } else {
        // (address & 0x01E0) >> 5

        if (space->start & 0x1F)
            return false;

        if (~space->end & 0x1F)
            if (space->end != 0xFFFE)
                return false;

        uint16_t start = (space->start & 0x01E0) >> 5;
        uint16_t end = (space->end & 0x01E0) >> 5;

        for (uint8_t i = start; i <= end; i++)
            this->highSpaces[i] = space;

        return true;
    }
}

void GBMemoryManagerDestroy(GBMemoryManager *this)
{
    free(this);
}

void __GBMemoryManagerWrite(GBMemoryManager *this, uint16_t address, uint8_t byte)
{
    if (address >= 0xFE00) {
        // high memory
        if (address == 0xFFFF)
        {
            /*printf("Changed interrupt status:\n");
            printf("VBlank:   %s\n", (byte & 0x01) ? "yes" : "no");
            printf("LCD Stat: %s\n", (byte & 0x02) ? "yes" : "no");
            printf("Timer:    %s\n", (byte & 0x04) ? "yes" : "no");
            printf("Serial:   %s\n", (byte & 0x08) ? "yes" : "no");
            printf("Joypad:   %s\n", (byte & 0x10) ? "yes" : "no");*/

            *this->interruptControl = byte;
        }

        // Mask top 7 bits. Shift off bottom 5
        GBMemorySpace *space = this->highSpaces[(address & 0x01E0) >> 5];
        space->write(space, address, byte);
    } else {
        GBMemorySpace *space = this->spaces[address >> 12];
        space->write(space, address, byte);
    }
}

uint8_t __GBMemoryManagerRead(GBMemoryManager *this, uint16_t address)
{
    if (address < 0x100 && !(*this->romMasked)) {
        // read rom
        return this->romSpace->read(this->romSpace, address);
    } else if (address > 0xFE00) {
        // high memory
        if (address == 0xFFFF)
            return *this->interruptControl;

        // Mask top 7 bits. Shift off bottom 5
        GBMemorySpace *space = this->highSpaces[(address & 0x01E0) >> 5];
        return space->read(space, address);
    } else {
        GBMemorySpace *space = this->spaces[address >> 12];
        return space->read(space, address);
    }
}

void GBMemoryManagerWriteRequest(GBMemoryManager *this, uint16_t *mar, uint8_t *mdr, bool *accessed)
{
    //printf("Request write 0x%02X to 0x%04X (%p, %p, %p)\n", *mdr, *mar, mar, mdr, accessed);
    this->accessed = accessed;
    this->mar = mar;
    this->mdr = mdr;

    this->isWrite = true;
}

void GBMemoryManagerReadRequest(GBMemoryManager *this, uint16_t *mar, uint8_t *mdr, bool *accessed)
{
    //printf("Request read from 0x%04X (%p, %p, %p)\n", *mar, mar, mdr, accessed);
    this->accessed = accessed;
    this->mar = mar;
    this->mdr = mdr;

    this->isWrite = false;
}

void __GBMemoryManagerTick(GBMemoryManager *this, uint64_t tick)
{
    // We tick at 1 MHz
    if (tick % 4)
        return;

    if (this->mar && this->mdr)
    {
        if (!(*this->dma)) {
            if (this->isWrite) {
                //printf("Write 0x%02X to 0x%04X (%p, %p, %p)\n", *this->mdr, *this->mar, this->mar, this->mdr, this->accessed);
                __GBMemoryManagerWrite(this, *this->mar, *this->mdr);
            } else {
                (*this->mdr) = __GBMemoryManagerRead(this, *this->mar);
                //printf("Read 0x%02X from 0x%04X (%p, %p, %p)\n", *this->mdr, *this->mar, this->mar, this->mdr, this->accessed);
            }
        } else {
            if ((*this->mar) < kGBHighRAMStart)
            {
                if (!this->isWrite)
                    (*this->mdr) = 0xFF;

                (*this->accessed) = true;
                return;
            }

            if ((*this->mar) > kGBHighRAMEnd)
            {
                if (!this->isWrite)
                    (*this->mdr) = 0xFF;

                (*this->accessed) = true;
                return;
            }

            if (this->isWrite) {
                __GBMemoryManagerWrite(this, *this->mar, *this->mdr);
            } else {
                (*this->mdr) = __GBMemoryManagerRead(this, *this->mar);
            }
        }

        // Prevent repeated writing while other hardware may modify the *mar byte.
        this->mar = NULL;
        this->mdr = NULL;

        (*this->accessed) = true;
    }
}

#pragma mark - Initializer

__attribute__((constructor)) static void __GBMemoryManagerInitNullSpace(void)
{
    gGBMemorySpaceNull = malloc(sizeof(GBMemorySpace));

    gGBMemorySpaceNull->install = NULL;

    gGBMemorySpaceNull->write = __GBMemorySpaceNullWrite;
    gGBMemorySpaceNull->read = __GBMemorySpaceNullRead;

    gGBMemorySpaceNull->start = 0x0000;
    gGBMemorySpaceNull->end   = 0xFFFF;
}

__attribute__((destructor)) static void __GBMemoryManagerDestroyNullSpace(void)
{
    free(gGBMemorySpaceNull);
}
