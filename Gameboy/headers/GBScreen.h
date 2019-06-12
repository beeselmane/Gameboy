#import <Cocoa/Cocoa.h>

@class GBScreenView;

@interface GBScreenWindow : NSWindowController <NSWindowDelegate>

@property (strong) IBOutlet GBScreenView *screenView;

- (instancetype) init;

- (void) cleanup;

- (void) resize:(NSUInteger)multiplier;

@end

@interface GBScreenView : NSOpenGLView

@property (atomic, readonly) CVDisplayLinkRef displayLink;
@property (atomic, readonly) NSInteger shaderProgram;
@property (atomic) SEL rendererName;

@property (atomic, readonly) CFAbsoluteTime lastFrameTime;
@property (atomic, readonly) CFTimeInterval frameDelta;

@property (atomic) NSSize newSize;
@property (atomic) BOOL resized;

- (NSData *) loadFile:(NSString *)path;
- (NSInteger) compileShader:(NSData *)data ofType:(NSInteger)type;
- (NSInteger) createShaderProgram:(NSDictionary *)shaders;

- (BOOL) acceptsFirstResponder;
- (void) keyDown:(NSEvent *)event;
- (void) keyUp:(NSEvent *)event;

- (void) setupContext;
- (void) setupDisplayLink;
- (void) cancelDisplayLink;
- (void) setup;
- (void) callRenderer:(SEL)cmd;

- (void) setupRenderer;
- (void) render;

@end
