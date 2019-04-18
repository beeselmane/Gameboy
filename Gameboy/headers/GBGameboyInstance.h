#import <Cocoa/Cocoa.h>
#import "disasm.h"

struct __GBGameboy;

@interface GBGameboyInstance : NSObject
{
    @public
        struct __GBGameboy *gameboy;
}

@property (readonly) NSString *currentOpcode;
@property (readonly) BOOL cartInstalled;
@property (readonly) UInt32 *screen;
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

- (UInt8) read:(UInt16)address;

- (NSImage *) generateBitmap:(UInt8)map isHighMap:(bool)isHighMap;
- (NSImage *) tileset;

- (GBDisassemblyInfo **) disassemble:(UInt8 *)code ofSize:(UInt32)size count:(UInt32 *)count;
- (void) outputDisassembly:(GBDisassemblyInfo **)code atBase:(UInt32)base count:(UInt32)count;
- (void) freeDisassembly:(GBDisassemblyInfo **)code count:(UInt32)count;

- (void) installDebugSerial;
- (void) dumpBootROM;

@end
