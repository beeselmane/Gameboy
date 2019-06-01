#import "GBPaletteWindow.h"
#import "GBAppDelegate.h"
#import "GBGameboyInstance.h"

#define kGBPaletteFrameUpdateTrigger 30

@implementation GBPaletteWindow

@synthesize imageView = _imageView;

- (instancetype) init
{
    return (self = [self initWithWindowNibName:@"GBPaletteWindow"]);
}

- (void) updateFrame
{
    if (self->framesSinceUpdate++ >= kGBPaletteFrameUpdateTrigger)
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

- (BOOL) windowShouldClose:(NSWindow *)sender
{
    [[self window] orderOut:sender];

    return NO;
}

@end
