#import <Cocoa/Cocoa.h>
#import "disasm.h"

struct __GBGameboy;

#ifndef __gameboy_gamepad__
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
#endif /* !defined(__gameboy_gamepad__) */

@interface GBGameboyInstance : NSObject
{
    @public
        struct __GBGameboy *gameboy;
}

@property (readonly) NSString *currentOpcode;
@property (readonly) BOOL cartInstalled;
@property (readonly) UInt32 *screen;
@property (readonly) NSString *game;
@property (readonly) SInt8 cpuMode;

@property UInt16 breakpointAddress;
@property BOOL breakpointActive;
@property BOOL isRunning;

- (instancetype) init;

- (void) installCartFromFile:(NSString *)path;
- (void) installCartFromData:(NSData *)data;

- (void) tick:(NSUInteger)times;
- (void) singleTick;
- (void) powerOn;

- (void) setKey:(UInt8)key pressed:(BOOL)pressed;
- (BOOL) isKeyPressed:(UInt8)key;

- (void) pressKey:(UInt8)key;
- (void) liftKey:(UInt8)key;

- (UInt8) read:(UInt16)address;

- (NSImage *) generateImage:(UInt8)map isHighMap:(bool)isHighMap;
- (NSImage *) tileset;

- (GBDisassemblyInfo **) disassemble:(UInt8 *)code ofSize:(UInt32)size count:(UInt32 *)count;
- (void) outputDisassembly:(GBDisassemblyInfo **)code atBase:(UInt32)base count:(UInt32)count;
- (void) freeDisassembly:(GBDisassemblyInfo **)code count:(UInt32)count;

- (void) installDebugSerial;
- (void) dumpBootROM;

@end
