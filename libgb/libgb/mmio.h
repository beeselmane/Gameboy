#ifndef __LIBGB_MMIO__
#define __LIBGB_MMIO__ 1

#include <stdbool.h>
#include <stdint.h>

#define kGBIOMapperFirstAddress 0xFF00
#define kGBIOMapperFinalAddress 0xFF7F

struct __GBGameboy;

typedef struct __GBIORegister {
    uint16_t address;

    void (*write)(struct __GBIORegister *this, uint8_t byte);
    uint8_t (*read)(struct __GBIORegister *this);

    uint8_t value;
} GBIORegister;

void __GBIORegisterNullWrite(GBIORegister *reg, uint8_t byte);
uint8_t __GBIORegisterNullRead(GBIORegister *reg);

void __GBIORegisterSimpleWrite(GBIORegister *reg, uint8_t byte);
uint8_t __GBIORegisterSimpleRead(GBIORegister *reg);

typedef struct __GBIOMapper {
    bool (*install)(struct __GBIOMapper *this, struct __GBGameboy *gameboy);

    void (*write)(struct __GBIOMapper *this, uint16_t address, uint8_t byte);
    uint8_t (*read)(struct __GBIOMapper *this, uint16_t address);

    uint16_t startAddress;
    uint16_t endAddress;

    GBIORegister *portMap[(kGBIOMapperFinalAddress - kGBIOMapperFirstAddress) + 1];
} GBIOMapper;

GBIOMapper *GBIOMapperCreate(void);
void GBIOMapperInstallPort(GBIOMapper *this, GBIORegister *port);
void GBIOMapperDestroy(GBIOMapper *this);

// These call through into the ports
void __GBIOMapperWrite(GBIOMapper *this, uint16_t address, uint8_t byte);
uint8_t __GBIOMapperRead(GBIOMapper *this, uint16_t address);

bool __GBIOMapperInstall(GBIOMapper *this, struct __GBGameboy *gameboy);

#endif /* !defined(__LIBGB_MMIO__) */
