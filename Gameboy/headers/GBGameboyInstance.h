#import <Cocoa/Cocoa.h>

struct __GBGameboy;

@interface GBGameboyInstance : NSObject
{
    struct __GBGameboy *gameboy;
}

@property (readonly) BOOL cartInstalled;
@property (readonly) UInt32 *screen;

- (instancetype) init;

- (void) installCartFromFile:(NSString *)path;
- (void) installCartFromData:(NSData *)data;

- (void) tick:(NSUInteger)times;

- (NSImage *) generateBitmap:(UInt8)map isHighMap:(bool)isHighMap;
- (NSImage *) tileset;

@end
