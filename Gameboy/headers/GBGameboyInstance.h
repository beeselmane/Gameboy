#import <Cocoa/Cocoa.h>

struct __GBGameboy;

// Interface class to libGameboy
@interface GBGameboyInstance : NSDocument
{
    struct __GBGameboy *gameboy;
}

@property (readonly) BOOL cartInstalled;
@property (readonly) UInt32 *screen;

- (void) tick:(NSUInteger)times;

@end
