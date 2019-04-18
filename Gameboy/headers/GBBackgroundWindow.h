#import <Cocoa/Cocoa.h>

@interface GBBackgroundWindow : NSWindowController
{
    UInt32 framesSinceUpdate;
}

@property (strong) IBOutlet NSImageView *imageView0;
@property (strong) IBOutlet NSImageView *imageView1;

- (void) updateImages;
- (void) updateFrame;

@end
