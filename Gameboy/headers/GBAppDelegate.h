#import <Cocoa/Cocoa.h>
#import "GBGameboyController.h"

@interface GBAppDelegate : NSObject <NSApplicationDelegate>

@property (strong) GBGameboyController *mainController;

- (IBAction) tick:(id)sender;
- (IBAction) step:(id)sender;
- (IBAction) run:(id)sender;
- (IBAction) image:(id)sender;
- (IBAction) breakProgram:(id)sender;
- (IBAction)loadMemory:(id)sender;

@property (strong) IBOutlet NSTextField *boxA;
@property (strong) IBOutlet NSTextField *boxB;
@property (strong) IBOutlet NSTextField *boxC;
@property (strong) IBOutlet NSTextField *boxD;
@property (strong) IBOutlet NSTextField *boxE;
@property (strong) IBOutlet NSTextField *boxH;
@property (strong) IBOutlet NSTextField *boxL;
@property (strong) IBOutlet NSTextField *boxSP;
@property (strong) IBOutlet NSTextField *boxPC;
@property (strong) IBOutlet NSTextField *boxIME;
@property (strong) IBOutlet NSTextField *boxAccessed;
@property (strong) IBOutlet NSTextField *boxMAR;
@property (strong) IBOutlet NSTextField *boxMDR;
@property (strong) IBOutlet NSTextField *boxOP;
@property (strong) IBOutlet NSTextField *boxASM;
@property (strong) IBOutlet NSTextField *boxMode;

@property (weak) IBOutlet NSTextField *boxTicks;
@property (weak) IBOutlet NSTextField *ticksBox;
@property (weak) IBOutlet NSTextField *boxZF;
@property (weak) IBOutlet NSTextField *boxNF;
@property (weak) IBOutlet NSTextField *boxHF;
@property (weak) IBOutlet NSTextField *boxCF;
@property (weak) IBOutlet NSTextField *breakpointBox;
@property (weak) IBOutlet NSTextField *boxPrefix;
@property (weak) IBOutlet NSTextField *boxIE;
@property (weak) IBOutlet NSTextField *boxIF;
@property (weak) IBOutlet NSTextField *boxMemoryAt;
@property (weak) IBOutlet NSTextField *boxMemoryData;

@property (weak) IBOutlet NSImageView *bgImage0;
@property (weak) IBOutlet NSImageView *bgImage1;
@property (weak) IBOutlet NSImageView *paletteImage;

@end
