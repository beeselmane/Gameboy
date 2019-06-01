#import "GBBackgroundWindow.h"
#import "GBAppDelegate.h"
#import "GBGameboyInstance.h"

#define kGBBackgroundFrameUpdateTrigger 30

@implementation GBBackgroundWindow

@synthesize imageView0 = _imageView0;
@synthesize imageView1 = _imageView1;

- (instancetype) init
{
    return (self = [self initWithWindowNibName:@"GBBackgroundWindow"]);
}

- (void) updateImages
{
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];

    [[self imageView0] setImage:[gameboy generateImage:0 isHighMap:NO]];
    [[self imageView1] setImage:[gameboy generateImage:1 isHighMap:NO]];
}

- (void) updateFrame
{
    if (self->framesSinceUpdate++ >= kGBBackgroundFrameUpdateTrigger)
    {
        self->framesSinceUpdate = 0;

        dispatch_async(dispatch_get_main_queue(), ^() {
            [self updateImages];
        });
    }
}

- (void) windowDidLoad
{
    [super windowDidLoad];

    [self updateImages];
}

- (BOOL) windowShouldClose:(NSWindow *)sender
{
    [[self window] orderOut:sender];

    return NO;
}

@end
