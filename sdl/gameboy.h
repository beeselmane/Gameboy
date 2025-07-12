#pragma once

#include <libgb/gameboy.h>
#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef SDLGB_DEBUG
    #define SDLGB_DEBUG 1
#endif /* !defined(SDLGB_DEBUG) */

#define GB_CPS 4194304 /* Gameboy clock ticks per second */

#define LOG(level,  msg, ...) SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ ## level, msg __VA_OPT__(,) __VA_ARGS__)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Breakpoint info
struct brk_info {
    // Breakpoint at address
    uint16_t addr;

    // Breakpoint on opcode
    uint16_t op;

    // Is breakpoint active?
    bool addr_active;
    bool op_active;

    // Was a breakpoint triggered?
    bool trigger_addr;
    bool trigger_op;
};

// Init new gameboy
extern GBGameboy *gameboy_init(void);

// Trigger system reset
extern void gameboy_reset(GBGameboy *gameboy);

// Cartridge handling

extern bool gameboy_has_cart(GBGameboy *gameboy);

extern bool gameboy_eject(GBGameboy *gameboy);

extern bool gameboy_load(GBGameboy *gameboy, const void *data, size_t size);

extern bool gameboy_load_file(GBGameboy *gameboy, const char *path);

// Power management

extern bool gameboy_is_on(GBGameboy *gameboy);

extern void gameboy_power_on(GBGameboy *gameboy);

extern void gameboy_power_off(GBGameboy *gameboy);

// Keypad handling

extern void gameboy_keydown(GBGameboy *gameboy, int key);

extern void gameboy_keyup(GBGameboy *gameboy, int key);

// Console clock ticking

extern int64_t gameboy_tick(GBGameboy *gameboy, uint32_t ticks, uint64_t deadline, struct brk_info *breakpoint);

extern int gameboy_step_once(GBGameboy *gameboy);

extern bool gameboy_tick_once(GBGameboy *gameboy);

// CPU debugging

extern uint16_t gameboy_op(GBGameboy *gameboy);

extern void gameboy_current_insn(GBGameboy *gameboy, char buf[32]);

extern int gameboy_cpu_mode(GBGameboy *gameboy);

// Accessing video memory

#define gameboy_screendata(gameboy) ((gameboy)->driver->screenData)

#define kGBTileCount 384

#define kGBBackgroundHiOffset    0x1C00
#define kGBBackgroundLoOffset    0x1800

#define kGBBackgroundHeight (kGBBackgroundTileCount * kGBTileHeight)
#define kGBBackgroundWidth (kGBBackgroundTileCount * kGBTileWidth)

#define kGBBackgroundTileCount 32

#define kGBTilesetHeight 192
#define kGBTilesetWidth  128

// There are 40 sprites made of 1 or 2 tiles.
#define kGBSpriteHeight  (5 * (kGBTileHeight * 2))
#define kGBSpriteWidth   (8 * (kGBTileWidth * 1))
#define kGBSpriteCount   40

typedef uint32_t gb_tileset[kGBTileCount][kGBTileHeight * kGBTileWidth];

extern void gameboy_decode_tileset_data(GBGameboy *gameboy, gb_tileset tileset);

extern void gameboy_decode_background_data(GBGameboy *gameboy, bool high_map, gb_tileset tileset, uint32_t *dest, int stride);

extern void gameboy_decode_sprite_data(GBGameboy *gameboy, uint32_t *dest, int stride);

extern void gameboy_copy_tileset(GBGameboy *gameboy, gb_tileset tileset, uint32_t *dest, int stride);
