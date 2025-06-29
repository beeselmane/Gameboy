#include <SDL3/SDL.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include <libgb/gameboy.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define SDLGB_DEBUG 1

#define LOG(level,  msg, ...) SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ ## level, msg __VA_OPT__(,) __VA_ARGS__)

#define APP_NAME        "SDL Gameboy"
#define APP_VERSION     "0.9"
#define APP_ID          "com.beeselmane.sdlgb"
#define WINDOW_NAME     "Gameboy"
#define WINDOW_HEIGHT   (kGBScreenHeight * 3)
#define WINDOW_WIDTH    (kGBScreenWidth * 3)
#define SEC_THRESHOLD   999999999
#define REFRESH_NS      16742706 /* ~59.727500569606 Hz */
#define GB_CPS          4194304 /* Gameboy clock ticks per second */
#define BYTES_PER_ROW   (kGBScreenWidth * sizeof(uint32_t))

#define min(a, b) ((a) < (b) ? (a) : (b))

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

struct state {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    uint64_t sec;
    uint64_t ticks;
    uint64_t frame;

    GBGameboy *gameboy;
};

struct state g_state;

static void report_error(const char *title, const char *logmsg)
{
    const char *error = SDL_GetError();

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, error, NULL);
    LOG(CRITICAL, "%s Error: '%s'", logmsg, error);
}

static bool gameboy_init(struct state *state)
{
    state->gameboy = GBGameboyCreate();

    if (!state->gameboy)
    {
        return false;
    }

    GBBIOSROM *bios = GBBIOSROMCreate(gGBDMGEditedROM);
    if (!bios) { return false; }

    GBGameboyInstallBIOS(state->gameboy, bios);

    return true;
}

static bool gameboy_load_file(struct state *state, const char *path)
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

    return GBGameboyInsertCartridge(state->gameboy, cart);
}

static void gameboy_keydown(struct state *state, int key) {
    GBGamepadSetKeyState(state->gameboy->gamepad, key, true);
}

static void gameboy_keyup(struct state *state, int key) {
    GBGamepadSetKeyState(state->gameboy->gamepad, key, false);
}

// The Mac OS X version of this app didn't have tick limited and woudl stall very badly.
// We usually only hit the limit if something bad happens and starts logging too much,
//   but I include this here as it's quite nice to not have the app lock up if it falls behind.
static bool gameboy_tick(struct state *state, uint64_t ticks, uint64_t before)
{
    if (!GBGameboyIsPoweredOn(state->gameboy)) {
        return true;
    }

    if (ticks > GB_CPS) {
        fprintf(stderr, "Error: Too far behind! (need %llu ticks)\n", ticks);
        return true;
    }

    for (uint64_t i = 0; i < ticks; i += 100)
    {
        for (uint64_t j = 0; j < min(ticks - i, 100); j++) {
            GBClockTick(state->gameboy->clock);
        }

        if (SDL_GetTicksNS() > before)
        {
            LOG(WARN, "Warrning: Too far behind! (%llu/%llu)", i, ticks);
            break;
        }
    }

    return true;
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

    SDL_WindowFlags windowFlags = 0;

    if (!SDL_CreateWindowAndRenderer(WINDOW_NAME, WINDOW_WIDTH, WINDOW_HEIGHT, windowFlags, &state->window, &state->renderer))
    {
        report_error("Initialization Failed", "Failed to create window and renderer.");
        return SDL_APP_FAILURE;
    }

    SDL_SetHintWithPriority(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1", SDL_HINT_OVERRIDE);

    if (!(state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, kGBScreenWidth, kGBScreenHeight)))
    {
        report_error("Initialization Failed", "Failed to setup rendering texture");
        return SDL_APP_FAILURE;
    }

    state->sec = SDL_GetTicksNS();
    state->ticks = SDL_GetTicksNS();
    state->frame = 0;

    return gameboy_init(state) ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

static bool draw_frame(struct state *state)
{
    uint32_t *screen_data = state->gameboy->driver->screenData;

    void *pixels;
    int pitch;

    if (!SDL_RenderClear(state->renderer))
    {
        report_error("Graphics Error", "Failed to clear renderer");
        return false;
    }

    if (!SDL_LockTexture(state->texture, NULL, &pixels, &pitch))
    {
        report_error("Graphics Error", "Failed to lock texture");
        return false;
    }

    if (pitch < BYTES_PER_ROW)
    {
        LOG(ERROR, "Texture pitch is invalid! (have %u)", pitch);
        return false;
    }

    for (int r = 0; r < kGBScreenHeight; r++)
    {
        uint32_t *row = (uint32_t *)(pixels + (pitch * r));
        memcpy(row, &screen_data[kGBScreenWidth * r], BYTES_PER_ROW);
    }

    SDL_UnlockTexture(state->texture);

    if (!SDL_RenderTexture(state->renderer, state->texture, NULL, NULL))
    {
        report_error("Graphics Error", "Failed to render texture");
        return false;
    }

    if (!SDL_RenderPresent(state->renderer))
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

    if (!gameboy_tick(state, (delta * GB_CPS) / SEC_THRESHOLD, now + REFRESH_NS)) {
        return SDL_APP_FAILURE;
    }

    if (!draw_frame(state)) {
        return SDL_APP_FAILURE;
    }

    if (now - state->sec > SEC_THRESHOLD) {
        state->sec = now;
        printf("FPS: %llu\n", state->frame);
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
                if (!gameboy_load_file(state, drop->data)) {
                    return SDL_APP_CONTINUE;
                }

                LOG(INFO, "Inserted '%s'\n", drop->data);
                GBGameboyPowerOn(state->gameboy);
            }
        } return SDL_APP_CONTINUE;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_MOUSE_MOTION:
            return SDL_APP_CONTINUE;

        case SDL_EVENT_KEY_DOWN: {
            switch (event->key.scancode)
            {
                case SDL_SCANCODE_RETURN: gameboy_keydown(state, kGBGamepadStart);  break;
                case SDL_SCANCODE_RSHIFT: gameboy_keydown(state, kGBGamepadSelect); break;
                case SDL_SCANCODE_P:      gameboy_keydown(state, kGBGamepadA);      break;
                case SDL_SCANCODE_L:      gameboy_keydown(state, kGBGamepadB);      break;
                case SDL_SCANCODE_W:      gameboy_keydown(state, kGBGamepadUp);     break;
                case SDL_SCANCODE_A:      gameboy_keydown(state, kGBGamepadLeft);   break;
                case SDL_SCANCODE_S:      gameboy_keydown(state, kGBGamepadDown);   break;
                case SDL_SCANCODE_D:      gameboy_keydown(state, kGBGamepadRight);  break;
                default: break;
            }
        } return SDL_APP_CONTINUE;

        case SDL_EVENT_KEY_UP: {
            switch (event->key.scancode)
            {
                case SDL_SCANCODE_RETURN: gameboy_keyup(state, kGBGamepadStart);  break;
                case SDL_SCANCODE_RSHIFT: gameboy_keyup(state, kGBGamepadSelect); break;
                case SDL_SCANCODE_P:      gameboy_keyup(state, kGBGamepadA);      break;
                case SDL_SCANCODE_L:      gameboy_keyup(state, kGBGamepadB);      break;
                case SDL_SCANCODE_W:      gameboy_keyup(state, kGBGamepadUp);     break;
                case SDL_SCANCODE_A:      gameboy_keyup(state, kGBGamepadLeft);   break;
                case SDL_SCANCODE_S:      gameboy_keyup(state, kGBGamepadDown);   break;
                case SDL_SCANCODE_D:      gameboy_keyup(state, kGBGamepadRight);  break;
                default: break;
            }
        } return SDL_APP_CONTINUE;

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
