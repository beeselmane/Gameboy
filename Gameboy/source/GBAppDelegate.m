#import "GBAppDelegate.h"

#import "GBGameboyInstance.h"
#import "GBStateWindow.h"
#import "GBScreen.h"

@implementation GBAppDelegate

@synthesize screenWindow = _screenWindow;
@synthesize gameboy = _gameboy;

+ (instancetype) instance
{
    return (GBAppDelegate *)[NSApp delegate];
}

- (void) updateFrame
{
    if ([[[self stateWindow] window] isVisible])
        [[self stateWindow] updateFrame];

    //
}

- (void) applicationWillFinishLaunching:(NSNotification *)notification
{
    _gameboy = [[GBGameboyInstance alloc] init];

    _screenWindow = [[GBScreenWindow alloc] init];
    _stateWindow = [[GBStateWindow alloc] init];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
    [[[self stateWindow] window] orderOut:self];

    [[self screenWindow] showWindow:self];
}

- (NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication *)sender
{
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Are you sure you want to stop emulation and exit?"];
    [alert setInformativeText:@"Exiting now will destroy any unsaved progress in your game. Are you sure you want to exit now?"];
    [alert addButtonWithTitle:@"Quit"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setAlertStyle:NSAlertStyleWarning];

    if ([alert runModal] == NSAlertFirstButtonReturn) {
        return NSTerminateNow;
    } else {
        return NSTerminateCancel;
    }
}

- (IBAction) focusScreen:(id)sender
{
    [[self screenWindow] showWindow:sender];
}

- (IBAction) focusState:(id)sender
{
    [[self stateWindow] showWindow:sender];

    [[self gameboy] setIsRunning:NO];
}

- (IBAction) startEmulation:(id)sender
{
    [[self gameboy] setIsRunning:YES];
}

- (IBAction) pauseEmulation:(id)sender
{
    [[self gameboy] setIsRunning:NO];
}

- (IBAction) beginEmulation:(id)sender
{
    [[self gameboy] powerOn];
}

- (IBAction) openROM:(id)sender
{
    NSOpenPanel *openPanel = [[NSOpenPanel alloc] init];
    [openPanel setCanChooseFiles:YES];
    //[openPanel setAllowedFileTypes:[[NSBundle mainBundle] fileTypes?]];
    [openPanel setAllowsOtherFileTypes:YES];
    [openPanel setAllowsMultipleSelection:NO];
    [openPanel setDirectoryURL:nil];

    if ([openPanel runModal] == NSModalResponseOK)
    {
        if ([[self gameboy] cartInstalled])
        {
            NSAlert *alert;
        }

        [[self gameboy] installCartFromData:[NSData dataWithContentsOfURL:[openPanel URL]]];
    }
}

@end

// Gameboy - title
// title - Registers
// title - Video memory
// title - ROM/RAM/VRAM
// title - I/O Ports
// title - Sprites

// Default title on Gameboy is empty. Otherwise it's "[No Game]"

// pop af fails checksums maybe? This would cause a lot of these failures on instructions which I know work...

// 0xC32A (end of pop af test)
// 0xC31D (push bc in pop af test)
// Fails on second test with 0x1301 because 0x01 & 0xF0 != 0x01

// 01 - Passed, yay, credit to this thread: http://forums.nesdev.com/viewtopic.php?f=20&t=15944
// 02 - Failed (timer doesn't work)
// 03 - Failed (E8 F8)
// 04 - Passed
// 05 - Passed
// 06 - Passed
// 07 - Passed
// 08 - Passed
// 09 - Failed (B8 B9 BA BB BC BD 90 91 92 93 94 95 98 99 9A 9B 9C 9D 9F 05 0D 15 1D 25 2D 3D) [Everything that use __ALUSub8; Everything else passed]
// 10 - Passed
// 11 - Failed (BE 96 9E 35) [Again, just things dependant on __ALUSub8]

// 0xFEA0 - 0xFEFF returns 0

// EI delays a single cycle after completion (the instruction after EI completes before the interrupt)
// HALT can fail?
// 10 nn - STOP with screen on
// Undefined only hang CPU.
// IME simply disables calling of interrupts.
// Upper bits of IF are 1
// Writing to IF overwrites natural sets in the same cycle
// Interrupts checked before fetch
// IF bit is cleared only on jump
// It takes 20 clocks to interrupt. 24 if halted.
// Interrupts: (two waits, push, push, set PC)
// Exiting halt mode always takes 4 cycles, even without interrupt
// Halt bugs if an interrupt is pending and ime is disabled when executed.
// --> In this case, we don't increment PC after the next instruction, and don't clear any IF bits

// Div is upper 8 of internal clock. This is a direct mapping.
// Bit 14 of the internal clock
// Lots more timer stuff to fix. See timing PDF...

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

// Each main window is attached to a single emulator instance. You can run multiple at a time, I suppose...
//
