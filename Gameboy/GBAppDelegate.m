#import "GBAppDelegate.h"

#import "gameboy.h"
#import "disasm.h"

UInt8 rom[0x100] = {
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

GBGameboy *gameboy;

@interface GBAppDelegate ()

@property (weak) IBOutlet NSWindow *window;

@end

@implementation GBAppDelegate

- (void) outputAndFreeDisassembly:(GBDisassemblyInfo **)disasm atOffset:(UInt32)offset withCount:(UInt32)count
{
    if (!disasm || !disasm[0])
    {
        NSLog(@"Nothing!");
        return;
    }

    for (UInt32 i = 0; i < count; i++)
    {
        printf("0x%04X: %s\n", offset + disasm[i]->offset, disasm[i]->string);

        free(disasm[i]->string);
        free(disasm[i]);
    }

    free(disasm);
}

- (void) setBoxes
{
    [_boxA setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.a]];
    [_boxB setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.b]];
    [_boxC setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.c]];
    [_boxD setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.d]];
    [_boxE setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.e]];
    [_boxH setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.h]];
    [_boxL setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.l]];

    [_boxSP setStringValue:[NSString stringWithFormat:@"0x%04X", gameboy->cpu->state.sp]];
    [_boxPC setStringValue:[NSString stringWithFormat:@"0x%04X", gameboy->cpu->state.pc]];

    [_boxMode setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.mode]];
    [_boxTicks setStringValue:[NSString stringWithFormat:@"%llu", gameboy->clock->ticks]];

    [_boxIME setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.ime]];
    [_boxAccessed setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.accessed]];

    [_boxMAR setStringValue:[NSString stringWithFormat:@"0x%04X", gameboy->cpu->state.mar]];
    [_boxMDR setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.mdr]];

    [_boxOP setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.op]];

    [_boxZF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.z]];
    [_boxNF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.n]];
    [_boxHF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.h]];
    [_boxCF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.c]];

    UInt8 buffer[3];
    UInt16 pc = gameboy->cpu->state.pc;
    UInt8 op;

    while ((op = __GBMemoryManagerRead(gameboy->cpu->mmu, pc)) != gameboy->cpu->state.op)
    {
        if (pc)
            pc--;
        else
            break;
    }

    if (pc && __GBMemoryManagerRead(gameboy->cpu->mmu, pc - 1) == 0xCB)
        pc--;

    buffer[0] = __GBMemoryManagerRead(gameboy->cpu->mmu, pc + 0);
    buffer[1] = __GBMemoryManagerRead(gameboy->cpu->mmu, pc + 1);
    buffer[2] = __GBMemoryManagerRead(gameboy->cpu->mmu, pc + 2);

    GBDisassemblyInfo *info = GBDisassembleSingle(buffer, 3, NULL);

    if (info) {
        [_boxASM setStringValue:[NSString stringWithUTF8String:info->string]];

        free(info->string);
        free(info);
    } else {
        [_boxASM setStringValue:@"??????"];
    }
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
    UInt32 count;

    GBDisassemblyInfo **disasm = GBDisassemblerProcess(rom, 0xA8, &count);

    [self outputAndFreeDisassembly:disasm atOffset:0x0000 withCount:count];

    disasm = GBDisassemblerProcess(rom + 0xE0, 0x20, &count);

    [self outputAndFreeDisassembly:disasm atOffset:0x00E0 withCount:count];

    gameboy = GBGameboyCreate();

    if (!gameboy)
    {
        printf("Nothing. :(\n");

        return;
    }

    GBBIOSROM *bios = GBBIOSROMCreate(rom);

    if (!bios)
    {
        printf("No rom. :(\n");

        return;
    }

    GBGameboyInstallBIOS(gameboy, bios);

    GBGameboyPowerOn(gameboy);

    [self setBoxes];
}

- (IBAction) tick:(id)sender
{
    NSInteger ticks = [[_ticksBox stringValue] integerValue];

    for (NSInteger i = 0; i < ticks; i++)
        GBClockTick(gameboy->clock);

    [self setBoxes];
}

- (IBAction) step:(id)sender
{
    NSInteger steps = [[_ticksBox stringValue] integerValue];
    UInt8 op = gameboy->cpu->state.op;

    for (NSInteger i = 0; i < steps; )
    {
        GBClockTick(gameboy->clock);

        if (gameboy->cpu->state.op != op)
        {
            op = gameboy->cpu->state.op;
            i++;
        }
    }

    [self setBoxes];
}

- (IBAction) run:(id)sender
{
    NSInteger breakpoint = [[_breakpointBox stringValue] integerValue];

    for ( ; ; )
    {
        GBClockTick(gameboy->clock);

        if (gameboy->cpu->state.pc == breakpoint && gameboy->cpu->state.mode == kGBProcessorModeFetch)
            break;
    }

    [self setBoxes];
}

@end

// Gameboy:
// --> Controller Options
// --> Color Options
// --> DMG/CGB (let's just do DMG first...)
// --> Remove cart
// --> Insert cart (load from file)
// --> Power toggle
// Emulation:
// --> Save state to file (note: stamp game name into save state)
// --> Load state from file
// --> Set emulation speed (tick rate)
// Debug:
// --> View status of basically everything

// Each main window is attached to a single emulator instance. You can run multiple at a time, I suppose...
//
