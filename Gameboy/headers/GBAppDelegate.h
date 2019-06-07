#import <Cocoa/Cocoa.h>

#define kGBSettingsKeymap @"Keycode Map"

@class GBGameboyInstance;

@class GBBackgroundWindow;
@class GBControllerWindow;
@class GBPaletteWindow;
@class GBScreenWindow;
@class GBStateWindow;

@interface GBAppDelegate : NSObject <NSApplicationDelegate>

@property (strong) GBGameboyInstance *gameboy;

@property (strong) GBBackgroundWindow *backgroundWindow;
@property (strong) GBControllerWindow *controllerWindow;
@property (strong) GBPaletteWindow *paletteWindow;
@property (strong) GBScreenWindow *screenWindow;
@property (strong) GBStateWindow *stateWindow;

+ (instancetype) instance;

- (void) updateFrame;

- (IBAction) focusScreen:(id)sender;
- (IBAction) focusState:(id)sender;
- (IBAction) focusPalette:(id)sender;
- (IBAction) focusController:(id)sender;
- (IBAction) focusBackground:(id)sender;
- (IBAction) resizeScreen:(NSMenuItem *)sender;

- (IBAction) startEmulation:(id)sender;
- (IBAction) pauseEmulation:(id)sender;

- (IBAction) beginEmulation:(id)sender;
- (IBAction) openROM:(id)sender;

@end
