#include "gameboy.h"
#include <stdio.h>

#if 0

#pragma mark - I/O Space

// These 2 functions are the default for unregistered i/o ports.
void GBIORegisterNullWrite(GBIORegister *port, UInt8 byte);
UInt8 GBIORegisterNullRead(GBIORegister *port);

void GBIORegisterNullWrite(GBIORegister *port, UInt8 byte)
{}

UInt8 GBIORegisterNullRead(GBIORegister *port)
{
    return 0xFF;
}

bool GBIOSpaceInit(GBIOMapper *space)
{
    GBIORegister *nullRegister = malloc(sizeof(GBIORegister));

    if (!nullRegister)
    {
        fprintf(stderr, "Error: Out of memory.\n");
        return false;
    }

    nullRegister->write = GBIORegisterNullWrite;
    nullRegister->read = GBIORegisterNullRead;

    for (UInt8 i = 0; i < 0x80; i++)
        space->portMap[i] = nullRegister;

    return true;
}

#pragma mark - Memory map

void GBMemoryManagerWrite(GBMemoryManager *map, UInt16 address, UInt8 byte);
UInt8 GBMemoryManagerRead(GBMemoryManager *map, UInt16 address);

bool GBMemoryMapInit(GBMemoryManager *map, GBCartridge *cartridge);

// 0x0000 --> 0x0100 (bios) [masks cart]
// 0x0000 --> 0x7FFF (rom)
// 0x8000 --> 0x9FFF (vram)
// 0xA000 --> 0xBFFF (xram) [paged]
// 0xC000 --> 0xDFFF (wram)
// 0xE000 --> 0xFDFF (mirror)
// 0xFE00 --> 0xFE9F (oam)
// 0xFEA0 --> 0xFEFF (xxx)
// 0xFF00 --> 0xFF7F (mmio)
// 0xFF80 --> 0xFFFE (hram)
// 0xFFFF --- interrupt control

void GBMemoryManagerWrite(GBMemoryManager *map, UInt16 address, UInt8 byte)
{
    if (!(~address)) {
        // interrupt
    } else if (address >= 0xFF80) {
        // high memory
    } else if (address >= 0xFF00) {
        // memory mapped i/o
        map->ioSpace->write(map->ioSpace, address, byte);
    } else if (address >= 0xFEA0) {
        // not present.
        fprintf(stderr, "Warning: Write to nonexistant memory address 0x%04X (byte=0x%02X)\n", address, byte);
        return;
    } else if (address >= 0xFE00) {
        // oam space
    } else if (address >= 0xE000) {
        // mirror of work ram (write work ram instead)
        map->workRAM->write(map->workRAM, address - 0x2000, byte);
    } else if (address >= 0xC000) {
        // work ram
        map->workRAM->write(map->workRAM, address, byte);
    } else if (address >= 0xA000) {
        // cartridge ram
    } else if (address >= 0x8000) {
        // video ram
    } else {
        // cartridge rom or bios. This can only be written with a MBC
    }
}

// Components:
// Internal, mainboard:
// --> CPU
// --> MMU
// --> Work RAM
// --> PPU
// --> APU
// --> Input Controller
// Internal, attached:
// --> PPU --> LCD
// --> APU --> Speaker
// --> APU --> Headphone Jack
// --> Input Controller --> Gamepad
// External, cartridge:
// --> Game ROM
// --> MBC (maybe)
// --> External RAM (maybe)
// --> 

UInt8 GBMemoryManagerRead(GBMemoryManager *map, UInt16 address)
{
    if (!(~address)) {
        // interrupt
    } else if (address >= 0xFF80) {
        // high memory
    } else if (address >= 0xFF00) {
        // memory mapped i/o
        return map->ioSpace->read(map->ioSpace, address);
    } else if (address >= 0xFEA0) {
        // not present.
        fprintf(stderr, "Warning: Read to nonexistant memory address 0x%04X\n", address);
        return 0xFF;
    } else if (address >= 0xFE00) {
        // oam space
    } else if (address >= 0xE000) {
        // mirror of work ram (write work ram instead)
        map->workRAM->read(map->workRAM, address - 0x2000);
    } else if (address >= 0xC000) {
        // work ram
        map->workRAM->read(map->workRAM, address);
    } else if (address >= 0xA000) {
        // cartridge ram
    } else if (address >= 0x8000) {
        // video ram
    } else {
        // cartridge rom OR bios if below 0x100 and bios still enabled

        if (address >= 0x100 || map->biosMasked) {
            // read from cartridge rom
        } else {
            // read from bios rom
        }
    }

    return 0xFF;
}

bool GBMemoryManagerInit(GBMemoryManager *map, GBCartridge *cartridge)
{
    map->cartridge = cartridge;
    map->ioSpace = malloc(sizeof(GBIOMapper));

    if (map->ioSpace) {
        if (!GBIOSpaceInit(map->ioSpace))
            return false;
    } else {
        fprintf(stderr, "Error: Out of memory.\n");
        return false;
    }

    return true;
}

#endif
