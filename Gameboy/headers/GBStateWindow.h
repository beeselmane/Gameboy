#import <Cocoa/Cocoa.h>

@interface GBStateWindow : NSWindowController <NSWindowDelegate>
{
    UInt32 framesSinceUpdate;
}

@property (strong) IBOutlet NSTextField *textBoxA;
@property (strong) IBOutlet NSTextField *textBoxB;
@property (strong) IBOutlet NSTextField *textBoxC;
@property (strong) IBOutlet NSTextField *textBoxD;
@property (strong) IBOutlet NSTextField *textBoxE;
@property (strong) IBOutlet NSTextField *textBoxH;
@property (strong) IBOutlet NSTextField *textBoxL;

@property (strong) IBOutlet NSTextField *textBoxSP;
@property (strong) IBOutlet NSTextField *textBoxPC;

@property (strong) IBOutlet NSTextField *textBoxFZ;
@property (strong) IBOutlet NSTextField *textBoxFN;
@property (strong) IBOutlet NSTextField *textBoxFH;
@property (strong) IBOutlet NSTextField *textBoxFC;

@property (strong) IBOutlet NSTextField *textBoxIME;
@property (strong) IBOutlet NSTextField *textBoxMAR;
@property (strong) IBOutlet NSTextField *textBoxMDR;
@property (strong) IBOutlet NSTextField *textBoxAccessed;

@property (strong) IBOutlet NSTextField *textBoxIE;
@property (strong) IBOutlet NSTextField *textBoxIF;
@property (strong) IBOutlet NSTextField *textBoxOP;
@property (strong) IBOutlet NSTextField *textBoxPrefix;
@property (strong) IBOutlet NSTextField *textBoxASM;

@property (strong) IBOutlet NSTextField *textBoxMode;
@property (strong) IBOutlet NSTextField *textBoxTicks;

@property (strong) IBOutlet NSTextField *textBoxMemoryAddress;
@property (strong) IBOutlet NSTextField *textBoxMemoryValue;

@property (strong) IBOutlet NSTextField *textBoxBreakpoint;
@property (strong) IBOutlet NSTextField *textBoxStepCount;

- (UInt16) textFieldHexValue:(NSTextField *)textField;

- (void) updateFrame;
- (void) updateText;

- (IBAction) tickButtonPressed:(id)sender;
- (IBAction) stepButtonPressed:(id)sender;
- (IBAction) breakButtonPressed:(id)sender;
- (IBAction) runButtonPressed:(id)sender;
- (IBAction) examineMemory:(id)sender;

@end
