#import <Cocoa/Cocoa.h>

@class GBGameboyInstance;
@class GBScreenView;

@interface GBScreenController : NSWindowController

@property (strong) IBOutlet GBScreenView *screenView;

- (instancetype) initWithInstance:(GBGameboyInstance *)instance;

@end
