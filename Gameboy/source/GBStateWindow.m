#import "GBStateWindow.h"
#import "GBGameboyInstance.h"
#import "GBAppDelegate.h"

// This is the only Objective-C class except GBGameboyInstance which references internal state directly
// This is simply a result of the amount of the overhead that would be required to go through the Obj-C object.
// This is just easier, in the end.
#import "gameboy.h"

#define kGBStateFrameUpdateTrigger 15

@interface GBStateWindow ()

- (void) setInterrupts:(NSTextField *)field value:(UInt8)value;
- (void) setDecimal:(NSTextField *)field value:(UInt32)value;
- (void) setHex16:(NSTextField *)field value:(UInt8)value;
- (void) setHex8:(NSTextField *)field value:(UInt8)value;

- (NSString *) modeString:(SInt8)mode;

@end

@implementation GBStateWindow

@synthesize textBoxA = _textBoxA;
@synthesize textBoxB = _textBoxB;
@synthesize textBoxC = _textBoxC;
@synthesize textBoxD = _textBoxD;
@synthesize textBoxE = _textBoxE;
@synthesize textBoxH = _textBoxH;
@synthesize textBoxL = _textBoxL;

@synthesize textBoxSP = _textBoxSP;
@synthesize textBoxPC = _textBoxPC;

@synthesize textBoxFZ = _textBoxFZ;
@synthesize textBoxFN = _textBoxFN;
@synthesize textBoxFH = _textBoxFH;
@synthesize textBoxFC = _textBoxFC;

@synthesize textBoxIME = _textBoxIME;
@synthesize textBoxMAR = _textBoxMAR;
@synthesize textBoxMDR = _textBoxMDR;
@synthesize textBoxAccessed = _textBoxAccessed;

@synthesize textBoxIE = _textBoxIE;
@synthesize textBoxIF = _textBoxIF;
@synthesize textBoxOP = _textBoxOP;
@synthesize textBoxPrefix = _textBoxPrefix;
@synthesize textBoxASM = _textBoxASM;

@synthesize textBoxMode = _textBoxMode;
@synthesize textBoxTicks = _textBoxTicks;

@synthesize textBoxMemoryAddress = _textBoxMemoryAddress;
@synthesize textBoxMemoryValue = _textBoxMemoryValue;

@synthesize textBoxBreakpoint = _textBoxBreakpoint;
@synthesize textBoxStepCount = _textBoxStepCount;

- (instancetype) init
{
    return (self = [self initWithWindowNibName:@"GBStateWindow"]);
}

- (void) updateFrame
{
    if (self->framesSinceUpdate++ >= kGBStateFrameUpdateTrigger)
    {
        self->framesSinceUpdate = 0;

        dispatch_async(dispatch_get_main_queue(), ^() {
            [[self textBoxASM] setStringValue:[[[GBAppDelegate instance] gameboy] currentOpcode]];

            [self updateText];
        });
    }
}

#pragma mark - Updating State

- (UInt16) textFieldHexValue:(NSTextField *)textField
{
    const char *stringValue = [[textField stringValue] UTF8String];

    UInt16 hex = strtol(stringValue, NULL, 16);

    return hex;
}

- (void) setInterrupts:(NSTextField *)field value:(UInt8)value
{
    bool interrupts[5];

    for (UInt8 i = 0; i < 5; i++)
        interrupts[i] = (value >> i) & 1;

    [field setStringValue:[NSString stringWithFormat:@"%d%d%d%d%d", interrupts[4], interrupts[3], interrupts[2], interrupts[1], interrupts[0]]];
}

- (void) setDecimal:(NSTextField *)field value:(UInt32)value
{
    [field setStringValue:[NSString stringWithFormat:@"%d", value]];
}

- (void) setHex16:(NSTextField *)field value:(UInt8)value
{
    [field setStringValue:[NSString stringWithFormat:@"0x%04X", value]];
}

- (void) setHex8:(NSTextField *)field value:(UInt8)value
{
    [field setStringValue:[NSString stringWithFormat:@"0x%02X", value]];
}

- (NSString *) modeString:(SInt8)mode
{
    switch (mode)
    {
        case kGBProcessorModeHalted:  return @"Halted";
        case kGBProcessorModeStopped: return @"Stopped";
        case kGBProcessorModeOff:     return @"Off";
        case kGBProcessorModeFetch:   return @"Fetch";
        case kGBProcessorModePrefix:  return @"Prefixed Fetch";
        case kGBProcessorModeStalled: return @"Stalled";
        case kGBProcessorModeRun:     return @"Running";
        case kGBProcessorModeWait1:   return @"Wait 1";
        case kGBProcessorModeWait2:   return @"Wait 2";
        case kGBProcessorModeWait3:   return @"Wait 3";
        case kGBProcessorModeWait4:   return @"Wait 4";
    }

    return @"<Unknown Mode>";
}

- (void) updateText
{
    GBGameboyInstance *instance = [[GBAppDelegate instance] gameboy];
    GBGameboy *gameboy = instance->gameboy;
    GBProcessorState *state = &gameboy->cpu->state;

    [self setHex8:[self textBoxA] value:state->a];
    [self setHex8:[self textBoxB] value:state->b];
    [self setHex8:[self textBoxC] value:state->c];
    [self setHex8:[self textBoxD] value:state->d];
    [self setHex8:[self textBoxE] value:state->e];
    [self setHex8:[self textBoxH] value:state->h];
    [self setHex8:[self textBoxL] value:state->l];

    [self setHex16:[self textBoxSP] value:state->sp];
    [self setHex16:[self textBoxPC] value:state->pc];

    [self setDecimal:[self textBoxFZ] value:state->f.z];
    [self setDecimal:[self textBoxFN] value:state->f.n];
    [self setDecimal:[self textBoxFH] value:state->f.h];
    [self setDecimal:[self textBoxFC] value:state->f.c];

    [self setDecimal:[self textBoxIME] value:state->ime];
    [self setHex16:[self textBoxMAR] value:state->mar];
    [self setHex16:[self textBoxMDR] value:state->mdr];
    [self setDecimal:[self textBoxAccessed] value:state->accessed];

    [self setInterrupts:[self textBoxIE] value:gameboy->cpu->ic->interruptControl];
    [self setInterrupts:[self textBoxIF] value:gameboy->cpu->ic->interruptFlagPort->value];

    [self setHex8:[self textBoxOP] value:state->op];
    [self setDecimal:[self textBoxPrefix] value:state->prefix];

    if (state->mode == kGBProcessorModeFetch)
        [[self textBoxASM] setStringValue:[[[GBAppDelegate instance] gameboy] currentOpcode]];

    [[self textBoxMode] setStringValue:[self modeString:state->mode]];
    [[self textBoxTicks] setStringValue:[NSString stringWithFormat:@"%llu", gameboy->clock->internalTick]];
}

#pragma mark - Controlling State

- (IBAction) tickButtonPressed:(id)sender
{
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];

    if ([gameboy isRunning])
        return;

    NSInteger ticks = [[[self textBoxStepCount] stringValue] integerValue];

    for (NSInteger i = 0; i < ticks; i++)
        [gameboy singleTick];

    [self updateText];
}

- (IBAction) stepButtonPressed:(id)sender
{
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];

    if ([gameboy isRunning])
        return;

    NSInteger steps = [[[self textBoxStepCount] stringValue] integerValue];

    for (NSInteger i = 0; i < steps; )
    {
        [gameboy singleTick];

        if ([gameboy cpuMode] == kGBProcessorModeFetch)
            i++;
    }

    [self updateText];
}

- (IBAction) breakButtonPressed:(id)sender
{
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];

    if ([gameboy isRunning])
        [gameboy setIsRunning:NO];

    [gameboy setBreakpointActive:NO];
}

- (IBAction) runButtonPressed:(id)sender
{
    UInt16 breakpointAddress = [self textFieldHexValue:[self textBoxBreakpoint]];
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];

    [gameboy setBreakpointAddress:breakpointAddress];
    [gameboy setBreakpointActive:YES];

    [gameboy setIsRunning:YES];

    [self setHex16:[self textBoxBreakpoint] value:breakpointAddress];
}

- (IBAction) examineMemory:(id)sender
{
    UInt16 address = [self textFieldHexValue:[self textBoxMemoryAddress]];
    GBGameboyInstance *gameboy = [[GBAppDelegate instance] gameboy];

    [self setHex8:[self textBoxMemoryValue] value:[gameboy read:address]];
    [self setHex16:[self textBoxMemoryAddress] value:address];
}

#pragma mark - Window Functions

- (void) windowDidLoad
{
    [super windowDidLoad];

    [self updateText];
}

- (BOOL) windowShouldClose:(NSWindow *)sender
{
    [[self window] orderOut:sender];

    return NO;
}

@end
