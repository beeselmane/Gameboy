#import <Cocoa/Cocoa.h>

@interface GBPaletteWindow : NSWindowController
{
    UInt32 framesSinceUpdate;
}

@property (strong) IBOutlet NSImageView *imageView;

- (void) updateFrame;
- (void) updateImage;

@end
