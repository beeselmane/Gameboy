#import <Cocoa/Cocoa.h>

@interface GBControllerWindow : NSWindowController <NSWindowDelegate>
{
    UInt32 framesSinceUpdate;
}

@property (strong) IBOutlet NSButton *buttonUpLeft;
@property (strong) IBOutlet NSButton *buttonUp;
@property (strong) IBOutlet NSButton *buttonUpRight;
@property (strong) IBOutlet NSButton *buttonRight;
@property (strong) IBOutlet NSButton *buttonDownRight;
@property (strong) IBOutlet NSButton *buttonDown;
@property (strong) IBOutlet NSButton *buttonDownLeft;
@property (strong) IBOutlet NSButton *buttonLeft;
@property (strong) IBOutlet NSButton *buttonStart;
@property (strong) IBOutlet NSButton *buttonSelect;
@property (strong) IBOutlet NSButton *buttonA;
@property (strong) IBOutlet NSButton *buttonB;

- (IBAction) toggle:(NSButton *)sender;

- (void) updateButtons;
- (void) updateFrame;

@end
