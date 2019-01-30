#ifndef __gameboy_mmio__
#define __gameboy_mmio__ 1

#include "base.h"

#define kGBIOMapperFirstAddress 0xFF00
#define kGBIOMapperFinalAddress 0xFF7F

typedef struct __GBIORegister {
    UInt16 address;

    void (*write)(struct __GBIORegister *this, UInt8 byte);
    UInt8 (*read)(struct __GBIORegister *this);

    UInt8 value;
} GBIORegister;

void __GBIORegisterNullWrite(GBIORegister *reg, UInt8 value);
UInt8 __GBIORegisterNullRead(GBIORegister *reg);

typedef struct __GBIOMapper {
    bool (*install)(struct __GBIOMapper *this, GBGameboy *gameboy);

    void (*write)(struct __GBIOMapper *this, UInt16 address, UInt8 byte);
    UInt8 (*read)(struct __GBIOMapper *this, UInt16 address);

    UInt16 startAddress;
    UInt16 endAddress;

    GBIORegister *portMap[(kGBIOMapperFinalAddress - kGBIOMapperFirstAddress) + 1];
} GBIOMapper;

GBIOMapper *GBIOMapperCreate(void);
void GBIOMapperInstallPort(GBIOMapper *this, GBIORegister *port);
void GBIOMapperDestroy(GBIOMapper *this);

// These call through into the ports
void __GBIOMapperWrite(GBIOMapper *this, UInt16 address, UInt8 byte);
UInt8 __GBIOMapperRead(GBIOMapper *this, UInt16 address);

bool __GBIOMapperInstall(GBIOMapper *this, GBGameboy *gameboy);

#endif /* __gameboy_mmio__ */
