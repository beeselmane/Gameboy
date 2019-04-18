#import "GBPaletteWindow.h"
#import "GBAppDelegate.h"
#import "GBGameboyInstance.h"

#define kGBStateFrameUpdateTrigger 30

@implementation GBPaletteWindow

@synthesize imageView = _imageView;

- (void) updateFrame
{
    if (self->framesSinceUpdate++ >= kGBStateFrameUpdateTrigger)
    {
        self->framesSinceUpdate = 0;

        dispatch_async(dispatch_get_main_queue(), ^() {
            [self updateImage];
        });
    }
}

- (void) updateImage
{
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];
    [[self imageView] setImage:[gameboy tileset]];
}

- (void) windowDidLoad
{
    [super windowDidLoad];

    [self updateImage];
}

@end
