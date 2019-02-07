#ifndef __gameboy_gamepad__
#define __gameboy_gamepad__ 1

#include "base.h"

#define kGBGamepadSelectMask    0x30
#define kGBGamepadPortAddress   0xFF00

struct __GBGameboy;

enum {
    kGBGamepadDown   = 0x3,
    kGBGamepadUp     = 0x2,
    kGBGamepadLeft   = 0x1,
    kGBGamepadRight  = 0x0,
    kGBGamepadStart  = 0x7,
    kGBGamepadSelect = 0x6,
    kGBGamepadA      = 0x5,
    kGBGamepadB      = 0x4
};

typedef struct __GBGamepad {
    UInt16 address;

    void (*write)(struct __GBGamepad *this, UInt8 byte);
    UInt8 (*read)(struct __GBGamepad *this);

    UInt8 value;
    bool pressed[2][4];

    UInt8 *interruptFlag;
    bool (*install)(struct __GBGamepad *this, struct __GBGameboy *gameboy);
} GBGamepad;

GBGamepad *GBGamepadCreate(void);
void GBGamepadSetKeyState(GBGamepad *this, UInt8 key, bool pressed);
bool GBGamepadIsKeyDown(GBGamepad *this, UInt8 key);
void GBGamepadDestroy(GBGamepad *this);

void __GBGamepadWrite(GBGamepad *this, UInt8 byte);

bool __GBGamepadInstall(GBGamepad *this, struct __GBGameboy *gameboy);

#endif /* !defined(__gameboy_gamepad__) */
