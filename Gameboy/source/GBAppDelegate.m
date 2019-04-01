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
   0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
   0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

UInt8 orom[0x100] = {
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
UInt8 op = 0x00;

@interface GBAppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@property (atomic) BOOL breakTriggered;

- (UInt16) hexValue:(NSTextField *)textField;

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

    NSString *mode;

    switch (gameboy->cpu->state.mode)
    {
        case kGBProcessorModeHalted:  mode = @"Halted";         break;
        case kGBProcessorModeStopped: mode = @"Stopped";        break;
        case kGBProcessorModeOff:     mode = @"Off";            break;
        case kGBProcessorModeFetch:   mode = @"Fetch";          break;
        case kGBProcessorModePrefix:  mode = @"Prefixed Fetch"; break;
        case kGBProcessorModeStalled: mode = @"Stalled";        break;
        case kGBProcessorModeRun:     mode = @"Running";        break;
        case kGBProcessorModeWait1:   mode = @"Wait 1";         break;
        case kGBProcessorModeWait2:   mode = @"Wait 2";         break;
        case kGBProcessorModeWait3:   mode = @"Wait 3";         break;
        case kGBProcessorModeWait4:   mode = @"Wait 4";         break;
    }

    [_boxTicks setStringValue:[NSString stringWithFormat:@"%llu", gameboy->clock->internalTick]];
    [_boxMode setStringValue:mode];

    [_boxIME setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.ime]];
    [_boxAccessed setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.accessed]];

    [_boxMAR setStringValue:[NSString stringWithFormat:@"0x%04X", gameboy->cpu->state.mar]];
    [_boxMDR setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.mdr]];

    [_boxOP setStringValue:[NSString stringWithFormat:@"0x%02X", gameboy->cpu->state.op]];
    [_boxPrefix setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.prefix]];

    [_boxZF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.z]];
    [_boxNF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.n]];
    [_boxHF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.h]];
    [_boxCF setStringValue:[NSString stringWithFormat:@"%d", gameboy->cpu->state.f.c]];

    bool ie[5] = {
        gameboy->cpu->ic->interruptControl & 0x01,
        gameboy->cpu->ic->interruptControl & 0x02,
        gameboy->cpu->ic->interruptControl & 0x04,
        gameboy->cpu->ic->interruptControl & 0x08,
        gameboy->cpu->ic->interruptControl & 0x10,
    };

    bool ifl[5] = {
        gameboy->cpu->ic->interruptFlagPort->value & 0x01,
        gameboy->cpu->ic->interruptFlagPort->value & 0x02,
        gameboy->cpu->ic->interruptFlagPort->value & 0x04,
        gameboy->cpu->ic->interruptFlagPort->value & 0x08,
        gameboy->cpu->ic->interruptFlagPort->value & 0x10,
    };

    [_boxIF setStringValue:[NSString stringWithFormat:@"%d%d%d%d%d", ifl[4], ifl[3], ifl[2], ifl[1], ifl[0]]];
    [_boxIE setStringValue:[NSString stringWithFormat:@"%d%d%d%d%d", ie[4], ie[3], ie[2], ie[1], ie[0]]];

    if (gameboy->cpu->state.mode == kGBProcessorModeFetch)
    {
        UInt16 pc = gameboy->cpu->state.pc;
        UInt8 buffer[4];
        //UInt8 i = 0;
        //UInt8 op;

        //if (pc)
        //    pc--;

        /*while ((op = __GBMemoryManagerRead(gameboy->cpu->mmu, pc)) != gameboy->cpu->state.op)
        {
            if (pc)
                pc--;
            else
                break;
        }

        if (gameboy->cpu->state.prefix)
            buffer[i++] = 0xCB;

        buffer[i++] = __GBMemoryManagerRead(gameboy->cpu->mmu, pc + 0);
        buffer[i++] = __GBMemoryManagerRead(gameboy->cpu->mmu, pc + 1);
        buffer[i  ] = __GBMemoryManagerRead(gameboy->cpu->mmu, pc + 2);*/

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
}

GBIORegister *ff01, *ff02;

void __GBIOWrite(GBIORegister *this, UInt8 byte)
{
    if (byte == 0x81)
        fprintf(stdout, "%c", ff01->value);
}

- (void) tryThing
{
    ff01 = malloc(sizeof(GBIORegister));
    ff02 = malloc(sizeof(GBIORegister));

    ff01->address = 0xFF01;

    ff01->write = __GBIORegisterSimpleWrite;
    ff01->read = __GBIORegisterSimpleRead;

    ff02->address = 0xFF02;

    ff02->read = __GBIORegisterSimpleRead;
    ff02->write = __GBIOWrite;

    GBIOMapperInstallPort(gameboy->mmio, ff02);
    GBIOMapperInstallPort(gameboy->mmio, ff01);
}

- (instancetype) init
{
    self = [super init];

    if (self)
        [self setMainController:[[GBGameboyController alloc] init]];

    return self;
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
    [self setBreakTriggered:NO];
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

    // 09-op r,r
    NSData *game = [NSData dataWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"tetris" ofType:@"gb"]];
    if (!game) exit(EXIT_FAILURE);

    GBCartridge *cart = GBCartridgeCreate((UInt8 *)[game bytes], (UInt32)[game length]);
    bool inserted = GBCartridgeInsert(cart, gameboy);

    if (inserted)
        printf("Loaded ROM!\n");

    [self setBoxes];
    [self tryThing];
}

- (UInt16) hexValue:(NSTextField *)textField
{
    const char *stringValue = [[textField stringValue] UTF8String];

    UInt16 hex = strtol(stringValue, NULL, 16);

    return hex;
}

// pop af fails checksums maybe? This would cause a lot of these failures on instructions which I know work...

// 0xC32A (end of pop af test)
// 0xC31D (push bc in pop af test)
// Fails on second test with 0x1301 because 0x01 & 0xF0 != 0x01

// 01 - Passed, yay, credit to this thread: http://forums.nesdev.com/viewtopic.php?f=20&t=15944
// 02 - Failed (timer doesn't work)
// 03 - Failed (E8 F8)
// 04 - Passed
// 05 - Passed
// 06 - Passed
// 07 - Passed
// 08 - Passed
// 09 - Failed (B8 B9 BA BB BC BD 90 91 92 93 94 95 98 99 9A 9B 9C 9D 9F 05 0D 15 1D 25 2D 3D) [Everything that use __ALUSub8; Everything else passed]
// 10 - Passed
// 11 - Failed (BE 96 9E 35) [Again, just things dependant on __ALUSub8]

// 0xFEA0 - 0xFEFF returns 0

// EI delays a single cycle after completion (the instruction after EI completes before the interrupt)
// HALT can fail?
// 10 nn - STOP with screen on
// Undefined only hang CPU.
// IME simply disables calling of interrupts.
// Upper bits of IF are 1
// Writing to IF overwrites natural sets in the same cycle
// Interrupts checked before fetch
// IF bit is cleared only on jump
// It takes 20 clocks to interrupt. 24 if halted.
// Interrupts: (two waits, push, push, set PC)
// Exiting halt mode always takes 4 cycles, even without interrupt
// Halt bugs if an interrupt is pending and ime is disabled when executed.
// --> In this case, we don't increment PC after the next instruction, and don't clear any IF bits

// Div is upper 8 of internal clock. This is a direct mapping.
// Bit 14 of the internal clock
// Lots more timer stuff to fix. See timing PDF...

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

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^() {
        for (NSInteger i = 0; i < steps; )
        {
            GBClockTick(gameboy->clock);

            if ([self breakTriggered])
            {
                [self setBreakTriggered:NO];

                break;
            }

            if (gameboy->cpu->state.mode == kGBProcessorModeFetch)
                i++;
        }

        dispatch_async(dispatch_get_main_queue(), ^() {
            [self setBoxes];
            [self image:sender];
        });
    });
}

// 0xC bytes from 0x2AF7 --> 0xFFB6
// 0x02A3 call fails.

- (IBAction) run:(id)sender
{
    NSInteger breakpoint = [self hexValue:_breakpointBox]; //[[_breakpointBox stringValue] integerValue];
    [_breakpointBox setStringValue:[NSString stringWithFormat:@"0x%04X", (UInt16)breakpoint]];

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^() {
        for ( ; ; )
        {
            GBClockTick(gameboy->clock);

            if ([self breakTriggered])
            {
                [self setBreakTriggered:NO];

                break;
            }

            if (!(gameboy->clock->internalTick % 0x1200000))
                dispatch_async(dispatch_get_main_queue(), ^() {
                    [self image:sender];
                });

            if (!(gameboy->clock->internalTick % 0x48000))
                dispatch_async(dispatch_get_main_queue(), ^() {
                    [self setBoxes];
                });

            if (gameboy->cpu->state.pc == breakpoint && gameboy->cpu->state.mode == kGBProcessorModeFetch)
                break;
        }

        // 0x2400000

        dispatch_async(dispatch_get_main_queue(), ^() {
            [self setBoxes];
            [self image:sender];
        });
    });
}

- (NSBitmapImageRep *) makeBitmapImageOfWidth:(UInt32)width height:(UInt32) height
{
    NSBitmapImageRep *bitmapImage = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:nil
                                                                            pixelsWide:width
                                                                            pixelsHigh:height
                                                                         bitsPerSample:8
                                                                       samplesPerPixel:3
                                                                              hasAlpha:NO
                                                                              isPlanar:NO
                                                                        colorSpaceName:NSCalibratedRGBColorSpace
                                                                           bytesPerRow:(width * 4)
                                                                          bitsPerPixel:32];

    //memcpy([bitmapImage bitmapData], rgbData, width * height * sizeof(UInt32));

    //return [[NSImage alloc] initWithCGImage:[bitmapImage CGImage] size:NSMakeSize(width, height)];
    return bitmapImage;
}

- (void) decodeTile:(UInt8 *)tileSource into:(UInt32 [8 * 8])result
{
    UInt32 lookup[4] = {0xEEEEEE, 0x000000, 0x555555, 0xBBBBBB};

    for (UInt8 y = 0; y < 0x10; y += 2)
    {
        for (UInt8 x = 0; x < 0x8; x++)
        {
            UInt8 value = ((tileSource[y + 1] >> x) << 1) & 2;
            value |= (tileSource[y] >> x) & 1;

            result[(y * 4) + (7 - x)] = lookup[value];
        }
    }
}

- (IBAction) image:(id)sender
{
    UInt8 control = gameboy->mmio->portMap[0x40]->value;
    bool highmap = control & 0x10;

    UInt8 *map1src = &gameboy->vram->memory[0x1800];
    UInt8 *map0src = &gameboy->vram->memory[0x1C00];
    UInt8 *tileset = &gameboy->vram->memory[0];

    //UInt32 map1data[32 * 32 * 8 * 8];
    //UInt32 map0data[32 * 32 * 8 * 8];
    NSBitmapImageRep *tileImage = [self makeBitmapImageOfWidth:128 height:192];
    UInt32 tiles[384][8 * 8];

    for (UInt16 i = 0; i < 384; i++)
    {
        [self decodeTile:&tileset[i * 0x10] into:tiles[i]];

        UInt16 y = i / 16;
        UInt16 x = i % 16;

        for (UInt8 y0 = 0; y0 < 8; y0++)
        {
            for (UInt8 x0 = 0; x0 < 8; x0++)
            {
                NSUInteger color = tiles[i][(y0 * 8) + x0];

                NSUInteger rgb[3] = {
                    (color >> 16) & 0xFF,
                    (color >>  8) & 0xFF,
                    (color >>  0) & 0xFF
                };

                [tileImage setPixel:rgb atX:((x * 8) + x0) y:((y * 8) + y0)];
            }
        }
    }

    [_paletteImage setImage:[[NSImage alloc] initWithCGImage:[tileImage CGImage] size:NSMakeSize(128, 192)]];

    NSBitmapImageRep *map1img = [self makeBitmapImageOfWidth:256 height:256];

    for (UInt8 y = 0; y < 32; y++)
    {
        for (UInt8 x = 0; x < 32; x++)
        {
            UInt8 tile;

            if (highmap) {
                SInt8 tid = (y * 32) + x;

                // -128 ---  0  --- +127
                //   0  --- 128 ---  255
                tile = map1src[tid + 128];
            } else {
                tile = map1src[(y * 32) + x];
            }


            for (UInt8 y0 = 0; y0 < 8; y0++)
            {
                for (UInt8 x0 = 0; x0 < 8; x0++)
                {
                    NSUInteger color = tiles[tile][(y0 * 8) + x0];

                    NSUInteger rgb[3] = {
                        (color >> 16) & 0xFF,
                        (color >>  8) & 0xFF,
                        (color >>  0) & 0xFF
                    };

                    [map1img setPixel:rgb atX:((x * 8) + x0) y:((y * 8) + y0)];
                }

            }
        }
    }

    [_bgImage1 setImage:[[NSImage alloc] initWithCGImage:[map1img CGImage] size:NSMakeSize(256, 256)]];

    NSBitmapImageRep *map0img = [self makeBitmapImageOfWidth:256 height:256];

    for (UInt8 y = 0; y < 32; y++)
    {
        for (UInt8 x = 0; x < 32; x++)
        {
            UInt8 tile;

            if (highmap) {
                SInt8 tid = (y * 32) + x;

                // -128 ---  0  --- +127
                //   0  --- 128 ---  255
                tile = map1src[tid + 128];
            } else {
                tile = map0src[(y * 32) + x];
            }


            for (UInt8 y0 = 0; y0 < 8; y0++)
            {
                for (UInt8 x0 = 0; x0 < 8; x0++)
                {
                    NSUInteger color = tiles[tile][(y0 * 8) + x0];

                    NSUInteger rgb[3] = {
                        (color >> 16) & 0xFF,
                        (color >>  8) & 0xFF,
                        (color >>  0) & 0xFF
                    };

                    [map0img setPixel:rgb atX:((x * 8) + x0) y:((y * 8) + y0)];
                }

            }
        }
    }

    [_bgImage0 setImage:[[NSImage alloc] initWithCGImage:[map0img CGImage] size:NSMakeSize(256, 256)]];
}

- (IBAction) breakProgram:(id)sender
{
    _breakTriggered = YES;
}

- (IBAction) loadMemory:(id)sender
{
    UInt16 address = [self hexValue:_boxMemoryAt];

    UInt8 data = __GBMemoryManagerRead(gameboy->cpu->mmu, address);

    [_boxMemoryAt setStringValue:[NSString stringWithFormat:@"0x%04X", address]];
    [_boxMemoryData setStringValue:[NSString stringWithFormat:@"0x%02X", data]];
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
