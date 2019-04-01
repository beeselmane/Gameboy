#import "GBScreenController.h"
#import "GBScreenView.h"

@interface GBScreenController ()

@property (weak) GBGameboyInstance *gameboyInstance;

@end

@implementation GBScreenController

@synthesize gameboyInstance = _gameboyInstance;

- (instancetype) initWithWindowNibName:(NSNibName)windowNibName
{
    self = [super initWithWindowNibName:windowNibName];

    return self;
}

- (instancetype) initWithInstance:(GBGameboyInstance *)instance;
{
    self = [self initWithWindowNibName:@"GBScreenController"];

    if (self)
        [self setGameboyInstance:instance];

    return self;
}

- (void) windowDidLoad
{
    [[self screenView] setGameboyInstance:[self gameboyInstance]];
    [[self screenView] setup];
}

@end
