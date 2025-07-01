#include "gameboy.h"
#include <stdlib.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#define APP_NAME            "SDL Gameboy"
#define APP_VERSION         "0.9"
#define APP_ID              "com.beeselmane.sdlgb"

#define MAIN_WINDOW_NAME    "0 — Gameboy"
#define MAIN_WINDOW_HEIGHT  kGBScreenHeight
#define MAIN_WINDOW_WIDTH   kGBScreenWidth
#define MAIN_WINDOW_RATIO   3.0F
#define MAIN_BYTES_PER_ROW  (kGBScreenWidth * sizeof(uint32_t))

#define DEBUG_WINDOW_NAME   "1 — Debugger"
#define DEBUG_WINDOW_HEIGHT 148 /* 12 lines of text */
#define DEBUG_WINDOW_WIDTH  240 /* 27 rows of text */ 
#define DEBUG_WINDOW_RATIO  2.0F

#define TILE_WINDOW_NAME    "2 — Tileset"
#define TILE_WINDOW_HEIGHT  kGBTilesetHeight
#define TILE_WINDOW_WIDTH   kGBTilesetWidth 
#define TILE_WINDOW_RATIO   2.0F
#define TILE_BYTES_PER_ROW  (kGBTilesetWidth * sizeof(uint32_t))

#define BG_WINDOW_NAME      "3 — Backgrounds"
#define BG_WINDOW_HEIGHT    ((kGBBackgroundHeight * 2) + 1)
#define BG_WINDOW_WIDTH     kGBBackgroundWidth 
#define BG_WINDOW_RATIO     2.0F
#define BG_BYTES_PER_ROW    (kGBBackgroundWidth * sizeof(uint32_t))

#define OAM_WINDOW_NAME     "4 — Sprites"
#define OAM_WINDOW_HEIGHT   kGBSpriteHeight
#define OAM_WINDOW_WIDTH    kGBSpriteWidth 
#define OAM_WINDOW_RATIO    3.0F
#define OAM_BYTES_PER_ROW   (kGBSpriteWidth * sizeof(uint32_t))

#define SEC_THRESHOLD       999999999
#define REFRESH_NS          16742706 /* ~59.727500569606 Hz */

struct window_state {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
};

struct state {
    // Main GB screen + debug text
    struct window_state screen;
    char debug_fps[8];
    char debug_cps[12];
    bool show_debug;

    // Various other windows
    struct window_state bg;
    struct window_state tiles;
    struct window_state oam;
    struct window_state debug;

    // General timing info
    uint64_t sec;
    uint64_t ticks;
    uint64_t frame;

    // Gameboy related info
    GBGameboy *gameboy;
    gb_tileset tileset;
    uint64_t clocks;
    double clk_mult;
    bool paused;

    // Is there an open file dialog now?
    bool showing_dialog;
};

struct state g_state;

static void report_error(const char *title, const char *logmsg)
{
    const char *error = SDL_GetError();
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, error, NULL);

    LOG(CRITICAL, "%s Error: '%s'", logmsg, error);
}

static bool create_window(struct window_state *state, const char *name, int width, int height, float ratio, SDL_WindowFlags flags)
{
    if (!SDL_CreateWindowAndRenderer(name, width * ratio, height * ratio, flags, &state->window, &state->renderer))
    {
        report_error("Initialization Failed", "Failed to create window and renderer.");
        return false;
    }

    if (!(state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, width, height)))
    {
        report_error("Initialization Failed", "Failed to setup texture.");
        return false;
    }

    if (!SDL_SetRenderScale(state->renderer, ratio, ratio))
    {
        report_error("Initialization Failed", "Failed to setup render scale.");
        return false;
    }

    LOG(INFO, "Created texture of size %u x %u", width, height);
    return true;
}

static bool toggle_window(struct state *state, struct window_state *wnd)
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(wnd->window);

    if (flags & SDL_WINDOW_HIDDEN) {
        return SDL_ShowWindow(wnd->window);
    } else if (flags & SDL_WINDOW_INPUT_FOCUS && wnd != &state->screen) {
        bool ok = SDL_HideWindow(wnd->window);
        if (!ok) { return ok; }

        return SDL_RaiseWindow(state->screen.window);
    } else {
        return SDL_RaiseWindow(wnd->window);
    }
}

static bool handle_file(struct state *state, const char *path)
{
    if (!gameboy_load_file(state->gameboy, path)) {
        return false;
    }

    LOG(INFO, "Inserted '%s'\n", path);
    GBGameboyPowerOn(state->gameboy);

    return true;
}

static void open_file_callback(void *userdata, const char *const *filelist, int filter)
{
    struct state *state = (struct state *)userdata;
    state->showing_dialog = false;

    if (!filelist) {
        LOG(ERROR, "Error selecting file. Error: '%s'", SDL_GetError());
    } else if (!filelist[0]) {
        LOG(INFO, "No file selected");
    } else if (filelist[1]) {
        LOG(WARN, "More than 1 file selected");
    } else {
        handle_file(state, filelist[0]);
    }
}

// We can return SDL_APP_SUCCESS, SDL_APP_CONTINUE, or SDL_APP_FAILURE here.
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    struct state *state = &g_state;
    (*appstate) = state;

    if (SDLGB_DEBUG) {
        setenv("SDL_LOGGING", "*=trace", false);
    }

    LOG(INFO, "sdlgb starting up...");

    if (!SDL_SetAppMetadata(APP_NAME, APP_VERSION, APP_ID))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Setup Error", "Failed to set app metadata. Some things may seem odd.", NULL);
        LOG(ERROR, "Failed to set app metadata!");
    }

    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        report_error("Initialization Failed", "Failed to initialize SDL.");
        return SDL_APP_FAILURE;
    }

    SDL_SetHintWithPriority(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1", SDL_HINT_OVERRIDE);

    #define mkwindow(prop, w, flags)                                                \
        do {                                                                        \
            const char *name = w ## _WINDOW_NAME;                                   \
            unsigned int width = w ## _WINDOW_WIDTH;                                \
            unsigned int height = w ## _WINDOW_HEIGHT;                              \
            double ratio = w ## _WINDOW_RATIO;                                      \
                                                                                    \
            if (!create_window(&state->prop, name, width, height, ratio, flags)) {  \
                return SDL_APP_FAILURE;                                             \
            }                                                                       \
        } while (0)

    mkwindow(screen, MAIN, 0);
    mkwindow(bg, BG, SDL_WINDOW_HIDDEN);
    mkwindow(tiles, TILE, SDL_WINDOW_HIDDEN);
    mkwindow(oam, OAM, SDL_WINDOW_HIDDEN);
    mkwindow(debug, DEBUG, SDL_WINDOW_HIDDEN);
    #undef mkwindow

    state->sec = SDL_GetTicksNS();
    state->ticks = SDL_GetTicksNS();
    state->frame = 0;
    state->clocks = 0;

    state->showing_dialog = false;

    state->debug_fps[0] = '\0';
    state->debug_cps[0] = '\0';
    state->show_debug = false;

    state->gameboy = gameboy_init();
    state->clk_mult = 1.0F;
    state->paused = false;

    if (!state->gameboy)
    {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

#define RENDER_TEXTURED(state, window, bpr, kind, render, ext)                              \
    do {                                                                                    \
        if (!SDL_RenderClear((window)->renderer))                                           \
        {                                                                                   \
            report_error("Graphics Error", "Failed to clear renderer");                     \
            return false;                                                                   \
        }                                                                                   \
                                                                                            \
        if (!SDL_LockTexture((window)->texture, NULL, &pixels, &pitch))                     \
        {                                                                                   \
            report_error("Graphics Error", "Failed to lock texture");                       \
            return false;                                                                   \
        }                                                                                   \
                                                                                            \
        if (pitch < bpr)                                                                    \
        {                                                                                   \
            LOG(ERROR, kind " texture pitch is invalid! (have %u, need %lu)", pitch, bpr);  \
            return false;                                                                   \
        }                                                                                   \
                                                                                            \
        render                                                                              \
                                                                                            \
        SDL_UnlockTexture((window)->texture);                                               \
                                                                                            \
        if (!SDL_RenderTexture((window)->renderer, (window)->texture, NULL, NULL))          \
        {                                                                                   \
            report_error("Graphics Error", "Failed to render texture");                     \
            return false;                                                                   \
        }                                                                                   \
                                                                                            \
        ext                                                                                 \
                                                                                            \
        if (!SDL_RenderPresent((window)->renderer))                                         \
        {                                                                                   \
            report_error("Graphics Error", "Failed to present renderer");                   \
            return false;                                                                   \
        }                                                                                   \
    } while (0)

static bool render_screen(struct state *state)
{
    uint32_t *screen_data = gameboy_screendata(state->gameboy);

    void *pixels;
    int pitch;

    RENDER_TEXTURED(state, &state->screen, MAIN_BYTES_PER_ROW, "Screen", {
        for (int r = 0; r < kGBScreenHeight; r++)
        {
            uint32_t *row = (uint32_t *)(pixels + (pitch * r));
            memcpy(row, &screen_data[kGBScreenWidth * r], MAIN_BYTES_PER_ROW);
        }
    }, {
        if (state->show_debug)
        {
            SDL_SetRenderDrawColor(state->screen.renderer, 255, 0, 0, 128);
            SDL_RenderDebugText(state->screen.renderer, 8, 8, state->debug_fps);
            SDL_RenderDebugText(state->screen.renderer, 8, 20, state->debug_cps);
            SDL_SetRenderDrawColor(state->screen.renderer, 0, 0, 0, 255);
        }
    });

    return true;
}

static bool render_background(struct state *state)
{
    void *pixels;
    int pitch;

    RENDER_TEXTURED(state, &state->bg, BG_BYTES_PER_ROW, "Background", {
        int half = (kGBBackgroundHeight * pitch);

        for (int i = 0; i < pitch / sizeof(uint32_t); i++) {
            ((uint32_t *)(pixels + half))[i] = 0xFF00FFFF;
        }

        half += pitch;

        gameboy_decode_background_data(state->gameboy, false, state->tileset, &pixels[   0], pitch);
        gameboy_decode_background_data(state->gameboy, true,  state->tileset, &pixels[half], pitch);
    }, {});

    return true;
}

static bool render_tileset(struct state *state)
{
    void *pixels;
    int pitch;

    RENDER_TEXTURED(state, &state->tiles, TILE_BYTES_PER_ROW, "Tileset", {
        gameboy_copy_tileset(state->gameboy, state->tileset, pixels, pitch);
    }, {});

    return true;
}

static bool render_sprites(struct state *state)
{
    void *pixels;
    int pitch;

    RENDER_TEXTURED(state, &state->oam, OAM_BYTES_PER_ROW, "Sprites", {
        gameboy_decode_sprite_data(state->gameboy, pixels, pitch);
    }, {});

    return true;
}

const char *gameboy_mode(GBGameboy *gameboy)
{
    switch (gameboy->cpu->state.mode)
    {
        case kGBProcessorModeHalted:  return "Halted";
        case kGBProcessorModeStopped: return "Stopped";
        case kGBProcessorModeOff:     return "Off";
        case kGBProcessorModeFetch:   return "Fetch";
        case kGBProcessorModePrefix:  return "PreFetch";
        case kGBProcessorModeStalled: return "Stalled";
        case kGBProcessorModeRun:     return "Running";
        case kGBProcessorModeWait1:   return "Wait 1";
        case kGBProcessorModeWait2:   return "Wait 2";
        case kGBProcessorModeWait3:   return "Wait 3";
        case kGBProcessorModeWait4:   return "Wait 4";
    }

    return "[???]";
}

// The debug screen is all text, which is all 8x8
static bool render_debug(struct state *state)
{
    struct window_state *window = &state->debug;

    if (!SDL_RenderClear(window->renderer))
    {
        report_error("Graphics Error", "Failed to clear renderer");
        return false;
    }

    // Row and column to pixel indexing. The font is 8x8 and I used 4 pixels between lines.
    #define row(n) ((n * 12) + 4)
    #define col(n) ((n * 8) + 8)

    struct __GBProcessorState *cpustate = &state->gameboy->cpu->state;
    GBInterruptController *ic = state->gameboy->cpu->ic;

    #define renderf(r, c, fmt, ...)                                     \
        do {                                                            \
            char buf[27];                                               \
                                                                        \
            snprintf(buf, 27, fmt __VA_OPT__(,) __VA_ARGS__);           \
            SDL_RenderDebugText(window->renderer, col(c), row(r), buf); \
        } while (0)

    SDL_SetRenderDrawColor(window->renderer, 255, 255, 255, 255);
    renderf(0, 0, "MODE: %s", gameboy_mode(state->gameboy));

    renderf(1, 0, "A: 0x%02X", cpustate->a);
    renderf(1, 10, "PC: 0x%04X", cpustate->pc);
    renderf(1, 23, "IE: %d", ic->interruptControl);

    renderf(2, 0, "B: 0x%02X", cpustate->b);
    renderf(2, 10, "SP: 0x%04X", cpustate->sp);
    renderf(2, 23, "IF: %d", ic->interruptFlagPort->value);

    renderf(3, 0, "C: 0x%02X", cpustate->c);

    renderf(4, 0, "D: 0x%02X", cpustate->d);
    renderf(4, 9, "MAR: 0x%04X", cpustate->mar);
    renderf(4, 22, "IME: %d", cpustate->ime);

    renderf(5, 0, "E: 0x%02X", cpustate->e);
    renderf(5, 9, "MDR: 0x%04X", cpustate->mdr);
    renderf(5, 22, "ACC: %d", cpustate->accessed);

    renderf(6, 0, "H: 0x%02X", cpustate->h);

    renderf(7, 0, "L: 0x%02X", cpustate->l);
    renderf(7, 10, "OP:   0x%02X", cpustate->op);
    renderf(7, 22, "PRE: %d", cpustate->prefix);

    renderf(8, 4,  "Z: %d", cpustate->f.z);
    renderf(8, 9,  "N: %d", cpustate->f.n);
    renderf(8, 14, "H: %d", cpustate->f.h);
    renderf(8, 19, "C: %d", cpustate->f.c);

    char ibuf[32];
    gameboy_current_insn(state->gameboy, ibuf);
    renderf(9, 0, "ASM: %s", ibuf);
    renderf(10, 0, "MULT: %.20f", state->clk_mult);
    renderf(11, 0, "TICK: %020llu", state->gameboy->clock->internalTick);

    SDL_SetRenderDrawColor(window->renderer, 0, 0, 0, 255);

    #undef renderf
    #undef col
    #undef row

    if (!SDL_RenderPresent(window->renderer))
    {
        report_error("Graphics Error", "Failed to present renderer");
        return false;
    }

    return true;
}

// We can return SDL_APP_SUCCESS, SDL_APP_FAILURE, or SDL_APP_CONTINUE
SDL_AppResult SDL_AppIterate(void *appstate)
{
    struct state *state = (struct state *)appstate;

    uint64_t now = SDL_GetTicksNS();
    uint64_t delta = now - state->ticks;

    if (!state->paused)
    {
        if (!gameboy_tick(state->gameboy, (delta * state->clk_mult * GB_CPS) / SEC_THRESHOLD, now + REFRESH_NS)) {
            return SDL_APP_FAILURE;
        }
    }

    if (!render_screen(state)) {
        return SDL_APP_FAILURE;
    }

    if (true)
    {
        gameboy_decode_tileset_data(state->gameboy, state->tileset);

        if (!render_background(state)) {
            return SDL_APP_FAILURE;
        }

        if (!render_tileset(state)) {
            return SDL_APP_FAILURE;
        }

        if (!render_sprites(state)) {
            return SDL_APP_FAILURE;
        }
    }

    if (!render_debug(state)) {
        return SDL_APP_FAILURE;
    }

    if (now - state->sec > SEC_THRESHOLD) {
        state->sec = now;

        uint64_t cps = state->gameboy->clock->internalTick - state->clocks;
        snprintf(state->debug_fps, 8, "FPS: %llu", state->frame);
        snprintf(state->debug_cps, 12, "CPS: %llu", cps);

        state->clocks = state->gameboy->clock->internalTick;
        state->frame = 0;
    } else {
        state->frame++;
    }

    state->ticks = now;

    // Elapsed time since the start of this frame.
    uint64_t elapsed = SDL_GetTicksNS() - now;

    if (elapsed < REFRESH_NS) {
        SDL_DelayNS(REFRESH_NS - elapsed);
    }

    return SDL_APP_CONTINUE;
}

// We can return SDL_APP_SUCCESS, SDL_APP_FAILURE, or SDL_APP_CONTINUE
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    struct state *state = (struct state *)appstate;

    switch (event->type)
    {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;

        case SDL_EVENT_DROP_FILE: {
            SDL_DropEvent *drop = &event->drop;

            if (drop->data) {
                handle_file(state, drop->data);
            }
        } return SDL_APP_CONTINUE;

        case SDL_EVENT_KEY_DOWN: {
            switch (event->key.scancode)
            {
                case SDL_SCANCODE_RETURN:   gameboy_keydown(state->gameboy, kGBGamepadStart);   break;
                case SDL_SCANCODE_RSHIFT:   gameboy_keydown(state->gameboy, kGBGamepadSelect);  break;
                case SDL_SCANCODE_P:        gameboy_keydown(state->gameboy, kGBGamepadA);       break;
                case SDL_SCANCODE_L:        gameboy_keydown(state->gameboy, kGBGamepadB);       break;
                case SDL_SCANCODE_W:        gameboy_keydown(state->gameboy, kGBGamepadUp);      break;
                case SDL_SCANCODE_A:        gameboy_keydown(state->gameboy, kGBGamepadLeft);    break;
                case SDL_SCANCODE_S:        gameboy_keydown(state->gameboy, kGBGamepadDown);    break;
                case SDL_SCANCODE_D:        gameboy_keydown(state->gameboy, kGBGamepadRight);   break;
                case SDL_SCANCODE_EQUALS:   state->clk_mult *= 2.0;                             break;
                case SDL_SCANCODE_MINUS:    state->clk_mult *= 0.5;                             break;
                default: break;
            }
        } return SDL_APP_CONTINUE;

        case SDL_EVENT_KEY_UP: {
            switch (event->key.scancode)
            {
                case SDL_SCANCODE_RETURN: gameboy_keyup(state->gameboy, kGBGamepadStart);  break;
                case SDL_SCANCODE_RSHIFT: gameboy_keyup(state->gameboy, kGBGamepadSelect); break;
                case SDL_SCANCODE_P:      gameboy_keyup(state->gameboy, kGBGamepadA);      break;
                case SDL_SCANCODE_L:      gameboy_keyup(state->gameboy, kGBGamepadB);      break;
                case SDL_SCANCODE_W:      gameboy_keyup(state->gameboy, kGBGamepadUp);     break;
                case SDL_SCANCODE_A:      gameboy_keyup(state->gameboy, kGBGamepadLeft);   break;
                case SDL_SCANCODE_S:      gameboy_keyup(state->gameboy, kGBGamepadDown);   break;
                case SDL_SCANCODE_D:      gameboy_keyup(state->gameboy, kGBGamepadRight);  break;
                case SDL_SCANCODE_F: {
                    state->show_debug = !state->show_debug;
                    LOG(DEBUG, "Debug %s", state->show_debug ? "enabled" : "disabled");
                } break;
                case SDL_SCANCODE_O: {
                    SDL_ShowOpenFileDialog(open_file_callback, state, NULL, NULL, 0, NULL, false);
                    state->showing_dialog = true;
                } break;
                case SDL_SCANCODE_0:        toggle_window(state, &state->screen);               break;
                case SDL_SCANCODE_1:        toggle_window(state, &state->debug);                break;
                case SDL_SCANCODE_2:        toggle_window(state, &state->tiles);                break;
                case SDL_SCANCODE_3:        toggle_window(state, &state->bg);                   break;
                case SDL_SCANCODE_4:        toggle_window(state, &state->oam);                  break;
                case SDL_SCANCODE_Z:        state->paused = !state->paused;                     break;
                case SDL_SCANCODE_E:        gameboy_eject(state->gameboy);                      break;
                case SDL_SCANCODE_R:        gameboy_reset(state->gameboy);                      break;
                case SDL_SCANCODE_T:        gameboy_tick_once(state->gameboy);                  break;
                default: break;
            }
        } return SDL_APP_CONTINUE;

        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_MOUSE_MOTION:
            return SDL_APP_CONTINUE;

        default:
            LOG(DEBUG, "Received event of unknown type '0x%X'", event->type);
            return SDL_APP_CONTINUE;
    }
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (result == SDL_APP_FAILURE) {
        LOG(ERROR, "App closing due to failure. Error: '%s'", SDL_GetError());
    }

    SDL_Quit();
}
