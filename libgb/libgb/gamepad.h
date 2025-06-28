#ifndef __LIBGB_GAMEPAD__
#define __LIBGB_GAMEPAD__ 1

#include <stdbool.h>
#include <stdint.h>

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
    uint16_t address;

    void (*write)(struct __GBGamepad *this, uint8_t byte);
    uint8_t (*read)(struct __GBGamepad *this);

    uint8_t value;
    bool pressed[2][4];

    uint8_t *interruptFlag;
    bool (*install)(struct __GBGamepad *this, struct __GBGameboy *gameboy);
} GBGamepad;

GBGamepad *GBGamepadCreate(void);
void GBGamepadSetKeyState(GBGamepad *this, uint8_t key, bool pressed);
bool GBGamepadIsKeyDown(GBGamepad *this, uint8_t key);
void GBGamepadDestroy(GBGamepad *this);

void __GBGamepadWrite(GBGamepad *this, uint8_t byte);

bool __GBGamepadInstall(GBGamepad *this, struct __GBGameboy *gameboy);

#endif /* !defined(__LIBGB_GAMEPAD__) */
