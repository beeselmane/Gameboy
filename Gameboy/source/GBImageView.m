#import "GBImageView.h"

@implementation GBImageView

- (instancetype) initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];

    if (self)
        [self setImageScaling:NSImageScaleProportionallyUpOrDown];

    return self;
}

- (void) drawRect:(NSRect)rect
{
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationNone];

    [super drawRect:rect];
}

@end
