#include <libgb/gameboy.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#pragma mark - Null port

GBIORegister *gGBIOMapperNullPorts[(kGBIOMapperFinalAddress - kGBIOMapperFirstAddress) + 1];

void __GBIORegisterNullWrite(GBIORegister *reg, uint8_t byte)
{
    //fprintf(stderr, "Warning: Attempt to write byte '0x%02X' to nonexistant I/O Port '0x%04X'\n", byte, reg->address);
}

uint8_t __GBIORegisterNullRead(GBIORegister *reg)
{
    fprintf(stderr, "Warning: Attempt to read byte from nonexistant I/O Port '0x%04X'\n", reg->address);

    return 0xFF;
}

#pragma mark - Simple Read/Write

void __GBIORegisterSimpleWrite(GBIORegister *reg, uint8_t byte)
{
    reg->value = byte;
}

uint8_t __GBIORegisterSimpleRead(GBIORegister *reg)
{
    return reg->value;
}

#pragma mark - I/O Mapper

GBIOMapper *GBIOMapperCreate(void)
{
    GBIOMapper *mapper = malloc(sizeof(GBIOMapper));

    if (mapper)
    {
        mapper->install = __GBIOMapperInstall;

        mapper->write = __GBIOMapperWrite;
        mapper->read = __GBIOMapperRead;

        mapper->startAddress = kGBIOMapperFirstAddress;
        mapper->endAddress = kGBIOMapperFinalAddress;

        memcpy(mapper->portMap, gGBIOMapperNullPorts, ((kGBIOMapperFinalAddress - kGBIOMapperFirstAddress) + 1) * sizeof(GBIORegister *));
    }

    return mapper;
}

void GBIOMapperInstallPort(GBIOMapper *mapper, GBIORegister *port)
{
    if (port->address < kGBIOMapperFirstAddress || kGBIOMapperFinalAddress < port->address)
    {
        fprintf(stderr, "Warning: Attempted to install I/O port at memory address out of range! (addr=0x%04X)\n", port->address);
        return;
    }

    mapper->portMap[port->address - kGBIOMapperFirstAddress] = port;
}

void GBIOMapperDestroy(GBIOMapper *this)
{
    free(this);
}

void __GBIOMapperWrite(GBIOMapper *this, uint16_t address, uint8_t byte)
{
    if (address < kGBIOMapperFirstAddress || kGBIOMapperFinalAddress < address)
    {
        fprintf(stderr, "Warning: Attempted write to I/O port at memory address out of range! (addr=0x%04X, byte=0x%02X)\n", address, byte);
        return;
    }

    GBIORegister *port = this->portMap[address - kGBIOMapperFirstAddress];
    port->write(port, byte);
}

uint8_t __GBIOMapperRead(GBIOMapper *this, uint16_t address)
{
    if (address < kGBIOMapperFirstAddress || kGBIOMapperFinalAddress < address)
    {
        fprintf(stderr, "Warning: Attempted read to I/O port at memory address out of range! (addr=0x%04X)\n", address);
        return 0xFF;
    }

    GBIORegister *port = this->portMap[address - kGBIOMapperFirstAddress];
    return port->read(port);
}

bool __GBIOMapperInstall(GBIOMapper *this, GBGameboy *gameboy)
{
    return GBMemoryManagerInstallSpace(gameboy->cpu->mmu, (GBMemorySpace *)this);
}

#pragma mark - Initializer

__attribute__((constructor)) static void __GBIOMapperInitNullPorts(void)
{
    uint16_t count = (kGBIOMapperFinalAddress - kGBIOMapperFirstAddress) + 1;

    for (uint16_t i = 0; i < count; i++)
    {
        gGBIOMapperNullPorts[i] = malloc(sizeof(GBIORegister));

        if (gGBIOMapperNullPorts[i]) {
            gGBIOMapperNullPorts[i]->address = i + kGBIOMapperFirstAddress;

            gGBIOMapperNullPorts[i]->write = __GBIORegisterNullWrite;
            gGBIOMapperNullPorts[i]->read = __GBIORegisterNullRead;

            gGBIOMapperNullPorts[i]->value = 0;
        } else {
            fprintf(stderr, "Error: Out of memory during constructor!\n");
            exit(EXIT_FAILURE);
        }
    }
}

__attribute__((destructor)) static void __GBIOMapperDestroyNullPorts(void)
{
    uint16_t count = (kGBIOMapperFinalAddress - kGBIOMapperFirstAddress) + 1;

    for (uint16_t i = 0; i < count; i++)
        free(gGBIOMapperNullPorts[i]);
}
