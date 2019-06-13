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

    [[self imageView0] setImage:[gameboy generateImage:0 isHighMap:[gameboy isUsingHighMap]]];
    [[self imageView1] setImage:[gameboy generateImage:1 isHighMap:[gameboy isUsingHighMap]]];
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

    CGSize aspectSize = [[self imageView0] frame].size;
    aspectSize.width += [[self imageView1] frame].size.width;

    [[self imageView0] setImageScaling:NSImageScaleProportionallyUpOrDown];
    [[self imageView1] setImageScaling:NSImageScaleProportionallyUpOrDown];
    [[self window] setAspectRatio:aspectSize];

    [self updateImages];
}

- (void) windowDidResize:(NSNotification *)notification
{
    //
}

- (BOOL) windowShouldClose:(NSWindow *)sender
{
    [[self window] orderOut:sender];

    return NO;
}

@end
