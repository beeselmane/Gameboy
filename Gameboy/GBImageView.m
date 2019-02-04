#import "GBImageView.h"

@implementation GBImageView

- (void) drawRect:(NSRect)rect
{
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];

    [super drawRect:rect];
}

@end
