#import "gameboy.h"

// This is included after gameboy.h to avoiding the gamepad enum
#import "GBGameboyInstance.h"
#import "disasm.h"

#define kGBTileCount            384

#define kGBTilesetOffset        0
#define kGBBackground0Offset    0x1C00
#define kGBBackground1Offset    0x1800

#define kGBBackgroundTileCount  32

__attribute__((section("__TEXT,__rom"))) UInt8 gGBDMGOriginalROM[0x100] = {
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

__attribute__((section("__TEXT,__rom"))) UInt8 gGBDMGEditedROM[0x100] = {
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

@interface GBGameboyInstance ()

- (NSBitmapImageRep *) makeBitmapImageOfWidth:(UInt32)width height:(UInt32) height;

- (NSBitmapImageRep *) decodeTileset:(UInt8 *)source into:(UInt32 [kGBTileCount][kGBTileWidth * kGBTileHeight])result needsImage:(bool)needsImage;
- (void) decodeTile:(UInt8 *)tileSource into:(UInt32 [kGBTileWidth * kGBTileHeight])result;

@end

@implementation GBGameboyInstance

@synthesize breakpointAddress = _breakpointAddress;
@synthesize breakpointActive = _breakpointActive;

@synthesize isRunning = _isRunning;

@dynamic currentOpcode;
@dynamic cpuMode;
@dynamic screen;

- (instancetype) init
{
    self = [super init];

    if (self)
    {
        self->gameboy = GBGameboyCreate();
        NSAssert(self->gameboy, @"Error: Could not instantiate gameboy instance!");

        GBBIOSROM *bios = GBBIOSROMCreate(gGBDMGEditedROM);
        NSAssert(self->gameboy, @"Error: Could not instantiate gameboy ROM!");

        GBGameboyInstallBIOS(self->gameboy, bios);

        _cartInstalled = false;
    }

    return self;
}

#pragma mark - ROM Loading

- (void) installCartFromFile:(NSString *)path
{
    return [self installCartFromData:[[NSFileManager defaultManager] contentsAtPath:path]];
}

- (void) installCartFromData:(NSData *)data
{
    GBCartridge *cart = GBCartridgeCreate((UInt8 *)[data bytes], (UInt32)[data length]);

    if (!cart)
        return;

    bool inserted = GBCartridgeInsert(cart, self->gameboy);

    if (!inserted)
        return;

    GBGameboyPowerOn(self->gameboy);
    _cartInstalled = YES;

    [self setIsRunning:YES];
}

#pragma mark - Instance Variables

- (NSString *) currentOpcode
{
    UInt16 pc = self->gameboy->cpu->state.pc;

    UInt8 buffer[3] = {
        [self read:pc + 0],
        [self read:pc + 1],
        [self read:pc + 2]
    };

    GBDisassemblyInfo *info = GBDisassembleSingle(buffer, 3, NULL);
    NSString *result = @"<?????>";

    if (info)
    {
        result = [NSString stringWithUTF8String:info->string];

        free(info->string);
        free(info);
    }

    return result;
}

- (SInt8) cpuMode
{
    return self->gameboy->cpu->state.mode;
}

- (UInt32 *) screen
{
    return self->gameboy->driver->screenData;
}

#pragma mark - Basic Functions

- (void) tick:(NSUInteger)times
{
    if (![self isRunning])
        return;

    for (NSUInteger i = 0; i < times; i++)
        [self singleTick];
}

- (void) singleTick
{
    GBClockTick(self->gameboy->clock);
}

- (void) powerOn
{
    GBGameboyPowerOn(self->gameboy);
}

- (void) pressKey:(UInt8)key
{
    GBGamepadSetKeyState(self->gameboy->gamepad, key, true);
}

- (void) liftKey:(UInt8)key
{
    GBGamepadSetKeyState(self->gameboy->gamepad, key, false);
}

- (UInt8) read:(UInt16)address
{
    return __GBMemoryManagerRead(self->gameboy->cpu->mmu, address);
}

#pragma mark - Background/Palette Image Routines

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
    return bitmapImage;
}

- (void) decodeTile:(UInt8 *)tileSource into:(UInt32 [kGBTileWidth * kGBTileHeight])result
{
    UInt32 lookup[4] = {0xEEEEEE, 0x000000, 0x555555, 0xBBBBBB};

    for (UInt8 y = 0; y < (2 * kGBTileHeight); y += 2)
    {
        for (UInt8 x = 0; x < kGBTileWidth; x++)
        {
            UInt8 value = ((tileSource[y + 1] >> x) << 1) & 2;
            value |= (tileSource[y] >> x) & 1;

            result[(y * 4) + (7 - x)] = lookup[value];
        }
    }
}

- (NSBitmapImageRep *) decodeTileset:(UInt8 *)source into:(UInt32 [kGBTileCount][kGBTileWidth * kGBTileHeight])result needsImage:(bool)needsImage
{
     NSBitmapImageRep *image = [self makeBitmapImageOfWidth:128 height:192];

     for (UInt16 i = 0; i < kGBTileCount; i++)
     {  
         [self decodeTile:&source[i * (2 * kGBTileHeight)] into:result[i]];

         if (needsImage)
         {
             UInt16 y = i / (2 * kGBTileHeight);
             UInt16 x = i % (2 * kGBTileWidth);

             for (UInt8 y0 = 0; y0 < kGBTileHeight; y0++)
             {
                 for (UInt8 x0 = 0; x0 < kGBTileWidth; x0++)
                 {
                     NSUInteger color = result[i][(y0 * kGBTileHeight) + x0];

                     NSUInteger rgb[3] = {
                         (color >> 16) & 0xFF,
                         (color >>  8) & 0xFF,
                         (color >>  0) & 0xFF
                     };

                     [image setPixel:rgb atX:((x * kGBTileWidth) + x0) y:((y * kGBTileHeight) + y0)];
                 }
             }
         }
     }

    return image;
}

- (NSImage *) tileset
{
    UInt32 tiles[kGBTileCount][kGBTileWidth * kGBTileHeight];
    UInt8 *tileset = &self->gameboy->vram->memory[0];

    NSBitmapImageRep *bitmap = [self decodeTileset:tileset into:tiles needsImage:YES];
    return [[NSImage alloc] initWithCGImage:[bitmap CGImage] size:NSMakeSize(128, 192)];
}

- (NSImage *) generateImage:(UInt8)map isHighMap:(bool)isHighMap
{
    bool isFirstMap = !map;

    UInt8 *sourceData = &self->gameboy->vram->memory[isFirstMap ? kGBBackground0Offset : kGBBackground1Offset];
    UInt8 *tileset = &self->gameboy->vram->memory[0];

    UInt32 tiles[kGBTileCount][kGBTileWidth * kGBTileHeight];
    [self decodeTileset:tileset into:tiles needsImage:NO];

    NSBitmapImageRep *image = [self makeBitmapImageOfWidth:(kGBBackgroundTileCount * kGBTileWidth) height:(kGBBackgroundTileCount * kGBTileHeight)];

    for (UInt8 y = 0; y < kGBBackgroundTileCount; y++)
    {
        for (UInt8 x = 0; x < kGBBackgroundTileCount; x++)
        {
            UInt8 tile;

            if (isHighMap) {
                SInt8 tid = (y * kGBBackgroundTileCount) + x;

                // -128 ---  0  --- +127
                //   0  --- 128 ---  255
                tile = sourceData[tid + 128];
            } else {
                tile = sourceData[(y * kGBBackgroundTileCount) + x];
            }


            for (UInt8 y0 = 0; y0 < kGBTileHeight; y0++)
            {
                for (UInt8 x0 = 0; x0 < kGBTileWidth; x0++)
                {
                    NSUInteger color = tiles[tile][(y0 * kGBTileHeight) + x0];

                    NSUInteger rgb[3] = {
                        (color >> 16) & 0xFF,
                        (color >>  8) & 0xFF,
                        (color >>  0) & 0xFF
                    };

                    [image setPixel:rgb atX:((x * kGBTileWidth) + x0) y:((y * kGBTileHeight) + y0)];
                }

            }
        }
    }

    return [[NSImage alloc] initWithCGImage:[image CGImage] size:NSMakeSize((kGBBackgroundTileCount * kGBTileWidth), (kGBBackgroundTileCount * kGBTileHeight))];
}

#pragma mark - Disassembler Utilities

- (GBDisassemblyInfo **) disassemble:(UInt8 *)code ofSize:(UInt32)size count:(UInt32 *)count
{
    return GBDisassemblerProcess(code, size, count);
}

- (void) outputDisassembly:(GBDisassemblyInfo **)code atBase:(UInt32)base count:(UInt32)count
{
    if (!code || !code[0])
        return;

    for (UInt32 i = 0; i < count; i++)
        fprintf(stdout, "0x%04X: %s\n", base + code[i]->offset, code[i]->string);
}

- (void) freeDisassembly:(GBDisassemblyInfo **)code count:(UInt32)count
{
    for (UInt32 i = 0; i < count; i++)
    {
        free(code[i]->string);
        free(code[i]);
    }

    free(code);
}

#pragma mark - Debug Routines

static GBIORegister *dbg_ff01, *dbg_ff02;

static void __GBIOWrite(GBIORegister *this, UInt8 byte)
{
    if (byte == 0x81)
        fprintf(stdout, "%c", dbg_ff01->value);
}

- (void) installDebugSerial
{
    dbg_ff01 = malloc(sizeof(GBIORegister));
    dbg_ff02 = malloc(sizeof(GBIORegister));

    dbg_ff01->address = 0xFF01;

    dbg_ff01->write = __GBIORegisterSimpleWrite;
    dbg_ff01->read = __GBIORegisterSimpleRead;

    dbg_ff02->address = 0xFF02;

    dbg_ff02->read = __GBIORegisterSimpleRead;
    dbg_ff02->write = __GBIOWrite;

    GBIOMapperInstallPort(self->gameboy->mmio, dbg_ff02);
    GBIOMapperInstallPort(self->gameboy->mmio, dbg_ff01);
}

- (void) dumpBootROM
{
    UInt32 count;

    GBDisassemblyInfo **disasm = [self disassemble:gGBDMGOriginalROM ofSize:0xA8 count:&count];
    [self outputDisassembly:disasm atBase:0x0000 count:count];
    [self freeDisassembly:disasm count:count];

    disasm = [self disassemble:(gGBDMGOriginalROM + 0xE0) ofSize:0x20 count:&count];
    [self outputDisassembly:disasm atBase:0x00E0 count:count];
    [self freeDisassembly:disasm count:count];
}

@end
