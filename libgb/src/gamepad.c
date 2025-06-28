#include <libgb/gameboy.h>
#include <strings.h>
#include <stdlib.h>

GBGamepad *GBGamepadCreate(void)
{
    GBGamepad *gamepad = malloc(sizeof(GBGamepad));

    if (gamepad)
    {
        bzero(gamepad->pressed, 8);

        gamepad->address = kGBGamepadPortAddress;

        gamepad->read = (void *)__GBIORegisterSimpleRead;
        gamepad->write = __GBGamepadWrite;

        gamepad->value = 0xCF;
        gamepad->install = __GBGamepadInstall;
    }

    return gamepad;
}

void GBGamepadSetKeyState(GBGamepad *this, uint8_t key, bool pressed)
{
    this->pressed[key >> 2][key & 0x3] = pressed;

    if (pressed)
        (*this->interruptFlag) &= (1 << kGBInterruptJoypad);
}

bool GBGamepadIsKeyDown(GBGamepad *this, uint8_t key)
{
    return this->pressed[key >> 2][key & 0x3];
}

void GBGamepadDestroy(GBGamepad *this)
{
    free(this);
}

void __GBGamepadWrite(GBGamepad *this, uint8_t byte)
{
    this->value = byte & ~0xCF;
    this->value |= 0xCF;

    if (this->value & 0x20) {
        for (uint8_t i = 0; i < 4; i++)
        {
            if (this->pressed[0][i])
                this->value &= ~(1 << i);
        }
    } if (this->value & 0x10) {
        for (uint8_t i = 0; i < 4; i++)
        {
            if (this->pressed[1][i])
                this->value &= ~(1 << i);
        }
    }
}

bool __GBGamepadInstall(GBGamepad *this, struct __GBGameboy *gameboy)
{
    GBIOMapperInstallPort(gameboy->mmio, (GBIORegister *)this);

    this->interruptFlag = &gameboy->cpu->ic->interruptFlagPort->value;

    return true;
}
