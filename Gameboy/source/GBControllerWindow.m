#import "GBControllerWindow.h"
#import "GBGameboyInstance.h"
#import "GBAppDelegate.h"

#define kGBControllerFrameUpdateTrigger 15

@implementation GBControllerWindow

@synthesize buttonUpLeft = _buttonUpLeft;
@synthesize buttonUp = _buttonUp;
@synthesize buttonUpRight = _buttonUpRight;
@synthesize buttonRight = _buttonRight;
@synthesize buttonDownRight = _buttonDownRight;
@synthesize buttonDown = _buttonDown;
@synthesize buttonDownLeft = _buttonDownLeft;
@synthesize buttonLeft = _buttonLeft;
@synthesize buttonStart = _buttonStart;
@synthesize buttonSelect = _buttonSelect;
@synthesize buttonA = _buttonA;
@synthesize buttonB = _buttonB;

- (instancetype) init
{
    return (self = [self initWithWindowNibName:@"GBControllerWindow"]);
}

- (void) updateFrame
{
    if (self->framesSinceUpdate++ >= kGBControllerFrameUpdateTrigger)
    {
        self->framesSinceUpdate = 0;

        dispatch_async(dispatch_get_main_queue(), ^() {
            [self updateButtons];
        });
    }
}

- (void) updateButtons
{
    BOOL isToggled; // Note: technically NSOnState = 1 and NSoffState = 0 so this isn't necessary, but it fits the abstraction better
    BOOL isUp, isDown, isLeft, isRight;

    // Directions

    isUp = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadUp];
    [[self buttonUp] setState:(isUp ? NSOnState : NSOffState)];

    isRight = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadRight];
    [[self buttonRight] setState:(isRight ? NSOnState : NSOffState)];

    isDown = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadDown];
    [[self buttonDown] setState:(isDown ? NSOnState : NSOffState)];

    isLeft = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadLeft];
    [[self buttonLeft] setState:(isLeft ? NSOnState : NSOffState)];

    // Diagonals

    [[self buttonUpLeft]    setState:((isUp   && isLeft ) ? NSOnState : NSOffState)];
    [[self buttonUpRight]   setState:((isUp   && isRight) ? NSOnState : NSOffState)];
    [[self buttonDownRight] setState:((isDown && isRight) ? NSOnState : NSOffState)];
    [[self buttonDownLeft]  setState:((isDown && isLeft ) ? NSOnState : NSOffState)];

    // Start/Select

    isToggled = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadStart];
    [[self buttonStart] setState:(isToggled ? NSOnState : NSOffState)];

    isToggled = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadSelect];
    [[self buttonSelect] setState:(isToggled ? NSOnState : NSOffState)];

    // A/B

    isToggled = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadA];
    [[self buttonA] setState:(isToggled ? NSOnState : NSOffState)];

    isToggled = [[[GBAppDelegate instance] gameboy] isKeyPressed:kGBGamepadB];
    [[self buttonB] setState:(isToggled ? NSOnState : NSOffState)];
}

- (IBAction) toggle:(NSButton *)sender
{
    BOOL toggleOn = ([sender state] == NSOnState);

    switch ([sender tag])
    {
        // For buttons 0-7, the tag lines up with the proper value to toggle
        default:
            [[[GBAppDelegate instance] gameboy] setKey:[sender tag] pressed:toggleOn];
        break;
        case 8: {
            // Up + Right
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadUp    pressed:toggleOn];
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadRight pressed:toggleOn];
        } break;
        case 9: {
            // Up + Left
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadUp    pressed:toggleOn];
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadLeft  pressed:toggleOn];
        } break;
        case 10: {
            // Down + Right
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadDown  pressed:toggleOn];
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadRight pressed:toggleOn];
        } break;
        case 11: {
            // Down + Left
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadDown  pressed:toggleOn];
            [[[GBAppDelegate instance] gameboy] setKey:kGBGamepadLeft  pressed:toggleOn];
        } break;
    }

    [self updateButtons];
}

- (BOOL) windowShouldClose:(NSWindow *)sender
{
    [[self window] orderOut:sender];

    return NO;
}

@end
