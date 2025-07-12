#include "gameboy.h"

#include <libgb/disasm.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

__attribute__((section("__TEXT,__rom"))) uint8_t gGBDMGEditedROM[0x100] = {
    0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
    0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
    0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
    0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
    0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
    0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
    0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
    0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
    0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
    0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
    0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
    0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
    0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
    0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
    0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x00, 0x00, 0x23, 0x7D, 0xFE, 0x34, 0x20,
    0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x00, 0x00, 0x3E, 0x01, 0xE0, 0x50
};

#define _read(gb, addr)     __GBMemoryManagerRead((gb)->cpu->mmu, (addr))

#define __cpu_mode(gb)      ((gb)->cpu->state.mode)
#define __tick_once(gb)     (GBClockTick((gb)->clock))
#define __clock_ticks(gb)   ((gb)->clock->internalTick)

#define _pc(gb)             ((gb)->cpu->state.pc)

#define _op(gb) ({                                  \
    uint16_t cur = _read(gameboy, _pc(gameboy));    \
                                                    \
    /* Prefix instructions */                       \
    if (cur == 0xCB)                                \
    {                                               \
        cur <<= 8;                                  \
        cur |= _read(gameboy, _pc(gameboy) + 1);    \
    }                                               \
                                                    \
    cur;                                            \
})

GBGameboy *gameboy_init(void)
{
    GBGameboy *gameboy = GBGameboyCreate();

    if (!gameboy)
    {
        return NULL;
    }

    GBBIOSROM *bios = GBBIOSROMCreate(gGBDMGEditedROM);

    if (!bios)
    {
        // free gameboy
        return NULL;
    }

    // can this fail?
    GBGameboyInstallBIOS(gameboy, bios);

    return gameboy;
}

void gameboy_reset(GBGameboy *gameboy)
{
}

bool gameboy_eject(GBGameboy *gameboy)
{
    return false;
}

bool gameboy_load_file(GBGameboy *gameboy, const char *path)
{
    struct stat stats;

    if (stat(path, &stats))
    {
        perror("stat");
        return false;
    }

    void *buf = malloc(stats.st_size);

    if (!buf)
    {
        perror("malloc");
        return false;
    }

    int fd = open(path, O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        free(buf);

        return false;
    }

    if (read(fd, buf, stats.st_size) < 0)
    {
        perror("read");
        close(fd);
        free(buf);

        return false;
    }

    if (close(fd))
    {
        perror("close");
        free(buf);

        return false;
    }

    GBCartridge *cart = GBCartridgeCreate(buf, stats.st_size);
    if (!cart) { return false; }

    return GBGameboyInsertCartridge(gameboy, cart);
}

void gameboy_keydown(GBGameboy *gameboy, int key) {
    GBGamepadSetKeyState(gameboy->gamepad, key, true);
}

void gameboy_keyup(GBGameboy *gameboy, int key) {
    GBGamepadSetKeyState(gameboy->gamepad, key, false);
}

// The Mac OS X version of this app didn't have tick limited and woudl stall very badly.
// We usually only hit the limit if something bad happens and starts logging too much,
//   but I include this here as it's quite nice to not have the app lock up if it falls behind.
int64_t gameboy_tick(GBGameboy *gameboy, uint32_t ticks, uint64_t deadline, struct brk_info *breakpoint)
{
    if (!GBGameboyIsPoweredOn(gameboy)) {
        return 0;
    }

    // TODO: This ought to happen in the render loop.
    if (ticks > GB_CPS) {
        fprintf(stderr, "Error: Too far behind! (need %u ticks)\n", ticks);
        return 0;
    }

    if (breakpoint->trigger_addr || breakpoint->trigger_op) {
        return 0;
    }

    // Account ticks here.
    uint64_t res = 0;

    // We may go slightly over, but never more than 12 ticks.
    while (res < ticks)
    {
        uint32_t i = 0;

        while (i < MIN(ticks - i, 100))
        {
            if (breakpoint->addr_active)
            {
                if (gameboy->cpu->state.pc == breakpoint->addr)
                {
                    breakpoint->trigger_addr = true;
                    return (res + i);
                }
            }

            if (breakpoint->op_active)
            {
                if (breakpoint->op == _op(gameboy))
                {
                    breakpoint->trigger_op = true;
                    return (res + i);
                }
            }

            i += gameboy_step_once(gameboy);
        }

        // Account how many ticks we've just done.
        res += i;

        if (SDL_GetTicksNS() > deadline)
        {
            LOG(WARN, "Too far behind! (%llu/%u)", res, ticks);
            break;
        }
    }

    return res;
}

// Execute a single full instruction.
// Return number of clockt icks, or negative on failure.
int gameboy_step_once(GBGameboy *gameboy)
{
    uint64_t starting_tick = __clock_ticks(gameboy);

    if (__cpu_mode(gameboy) == kGBProcessorModeFetch) {
        __tick_once(gameboy);
    }

    while (__cpu_mode(gameboy) != kGBProcessorModeFetch) {
        __tick_once(gameboy);
    }

    return (__clock_ticks(gameboy) - starting_tick);
}

bool gameboy_tick_once(GBGameboy *gameboy)
{
    if (!GBGameboyIsPoweredOn(gameboy)) {
        return true;
    }

    __tick_once(gameboy);
    return true;
}

uint16_t gameboy_op(GBGameboy *gameboy)
{ return _op(gameboy); }

void gameboy_current_insn(GBGameboy *gameboy, char buf[32])
{
    uint16_t pc = gameboy->cpu->state.pc;

    uint8_t insn[3] = {
        _read(gameboy, pc + 0),
        _read(gameboy, pc + 1),
        _read(gameboy, pc + 2)
    };

    GBDisassemblyInfo *info = GBDisassembleSingle(insn, 3, NULL);

    if (!info)
    {
        buf[0] = '?';
        buf[1] = '\0';

        return;
    }

    strlcpy(buf, info->string, 32);
    free(info->string);
    free(info);
}

int gameboy_cpu_mode(GBGameboy *gameboy);

// Video memory section

static uint32_t _color_lookup[4] = { 0xEEEEEEFF, 0xBBBBBBFF, 0x555555FF, 0x000000FF };

static void _decode_tile(uint8_t *source, uint32_t dest[kGBTileWidth * kGBTileHeight], uint8_t palette[4])
{
    for (int y = 0; y < (2 * kGBTileHeight); y += 2)
    {
        for (int x = 0; x < kGBTileWidth; x++)
        {
            uint8_t value = ((source[y + 1] >> x) & 2);
            value |= (source[y] >> x) & 1;

            dest[(y * (kGBTileWidth / 2)) + (7 - x)] = _color_lookup[value];
        }
    }
}

void gameboy_decode_tileset_data(GBGameboy *gameboy, gb_tileset tileset)
{
    uint8_t raw_palette = _read(gameboy, kGBPalettePortBGAddress);
    uint8_t *tileset_source = &gameboy->vram->memory[0];

    uint8_t palette[4] = {
        ((raw_palette >> 0) & 3),
        ((raw_palette >> 2) & 3),
        ((raw_palette >> 4) & 3),
        ((raw_palette >> 6) & 3)
    };

    for (int i = 0; i < kGBTileCount; i++) {
        _decode_tile(&tileset_source[i * (2 * kGBTileHeight)], tileset[i], palette);
    }
}

void gameboy_decode_background_data(GBGameboy *gameboy, bool high_map, gb_tileset tileset, uint32_t *dest, int stride)
{
    uint8_t *bg_source = &gameboy->vram->memory[high_map ? kGBBackgroundHiOffset : kGBBackgroundLoOffset];

    for (int y = 0; y < kGBBackgroundTileCount; y++)
    {
        for (int x = 0; x < kGBBackgroundTileCount; x++)
        {
            uint16_t tile = bg_source[(y * kGBBackgroundTileCount) + x];

            if (high_map && !(tile & 0x80)) {
                tile |= 0x100;
            }

            for (uint8_t y2 = 0; y2 < kGBTileHeight; y2++)
            {
                int row = (y * kGBTileHeight) + y2;

                uint8_t *target = &((uint8_t *)dest)[(row * stride) + (x * kGBTileWidth * sizeof(uint32_t))];

                memcpy(target, &tileset[tile][y2 * kGBTileWidth], kGBTileWidth * sizeof(uint32_t));
            }
        }
    }
}

void _copy_tile(uint32_t tile[kGBTileHeight * kGBTileWidth], int x, int y, void *in, int stride, bool flip_x, bool flip_y)
{
    for (int tile_y = 0; tile_y < kGBTileHeight; tile_y++)
    {
        int src_y = flip_y ? kGBTileHeight - tile_y - 1 : tile_y;
        uint32_t *src = &tile[src_y * kGBTileWidth];

        int dst_idx = ((y + tile_y) * stride) + (x * sizeof(uint32_t));
        uint32_t *target = (uint32_t *)(in + dst_idx);

        if (flip_x) {
            for (int tile_x = 0; tile_x < kGBTileWidth; tile_x++) {
                target[tile_x] = src[kGBTileWidth - tile_x - 1];
            }
        } else {
            memcpy(target, src, kGBTileWidth * sizeof(uint32_t));
        }
    }
}

void gameboy_decode_sprite_data(GBGameboy *gameboy, uint32_t *dest, int stride)
{
    uint8_t *tileset_source = &gameboy->vram->memory[0];

    uint8_t palette_lo_raw = _read(gameboy, kGBPalettePortSprite0Address);
    uint8_t palette_hi_raw = _read(gameboy, kGBPalettePortSprite1Address);

    uint8_t palette_lo[4] = {
        0,
        ((palette_lo_raw >> 2) & 3),
        ((palette_lo_raw >> 4) & 3),
        ((palette_lo_raw >> 6) & 3)
    };

    uint8_t palette_hi[4] = {
        0,
        ((palette_hi_raw >> 2) & 3),
        ((palette_hi_raw >> 4) & 3),
        ((palette_hi_raw >> 6) & 3)
    };

    // It requires less decoding to decode the 40-80 tiles for the sprites
    //   than the entire 300+ tile tileset in each palette.
    uint32_t tile[kGBTileHeight * kGBTileWidth];

    uint8_t lcdc = _read(gameboy, kGBLCDControlPortAddress);
    bool twoTile = ((lcdc >> 2) & 1);

    for (int i = 0; i < kGBSpriteCount; i++)
    {
        GBSpriteDescriptor *descriptor = &gameboy->driver->oam->memory[i];
        uint8_t *palette = ((descriptor->attributes >> 4) & 1) ? palette_lo : palette_hi;

        _decode_tile(&tileset_source[descriptor->pattern * (2 * kGBTileHeight)], tile, palette);

        int y = (i / 8) * (kGBTileHeight * 2);
        int x = (i % 8) * kGBTileWidth;

        bool flip_x = (descriptor->attributes >> 5) & 1;
        bool flip_y = (descriptor->attributes >> 6) & 1;

        _copy_tile(tile, x, y, dest, stride, flip_x, flip_y);
        y += kGBTileHeight;

        if (twoTile) {
            _decode_tile(&tileset_source[(descriptor->pattern + 1) * (2 * kGBTileHeight)], tile, palette);
            _copy_tile(tile, x, y, dest, stride, flip_x, flip_y);
        } else {
            for (int j = 0; j < kGBTileHeight; j++)
            {
                for (int k = 0; k < kGBTileWidth; k++) {
                    tile[(j * kGBTileWidth) + k] = 0xFF00FFFF;
                }
            }

            _copy_tile(tile, x, y, dest, stride, false, false);
        }
    }
}

void gameboy_copy_tileset(GBGameboy *gameboy, gb_tileset tileset, uint32_t *dest, int stride)
{
    int tiles_per_row = kGBTilesetWidth / kGBTileWidth;

    for (int i = 0; i < kGBTileCount; i++)
    {
        int y = (i / tiles_per_row) * kGBTileHeight;
        int x = (i % tiles_per_row) * kGBTileWidth;

        _copy_tile(tileset[i], x, y, dest, stride, false, false);

        //for (int y2 = 0; y2 < kGBTileHeight; y2++)
        //{
        //    uint32_t *src = &tileset[i][y2 * kGBTileWidth];
        //    int row = y + y2;

        //    uint8_t *target = &((uint8_t *)dest)[(row * stride) + (x * sizeof(uint32_t))];
        //    memcpy(target, src, kGBTileWidth * sizeof(uint32_t));
        //}
    }
}
