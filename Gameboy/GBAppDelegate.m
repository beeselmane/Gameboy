#import "GBAppDelegate.h"

@interface GBAppDelegate ()

@property (weak) IBOutlet NSWindow *window;

@end

@implementation GBAppDelegate

@end

// Gameboy:
// --> Controller Options
// --> Color Options
// --> DMG/CGB (let's just do DMG first...)
// --> Remove cart
// --> Insert cart (load from file)
// --> Power toggle
// Emulation:
// --> Save state to file (note: stamp game name into save state)
// --> Load state from file
// --> Set emulation speed (tick rate)
// Debug:
// --> View status of basically everything

// Each main window is attached to a single emulator instance. You can run multiple at a time, I suppose
//
