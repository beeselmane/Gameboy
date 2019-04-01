#import <Cocoa/Cocoa.h>

@class GBGameboyInstance;

@interface GBScreenView : NSOpenGLView

@property (atomic, readonly) CVDisplayLinkRef displayLink;
@property (atomic, readonly) NSInteger shaderProgram;
@property (atomic) SEL rendererName;

@property (atomic, readonly) CFAbsoluteTime lastFrameTime;
@property (atomic, readonly) CFTimeInterval frameDelta;

@property (atomic) NSSize newSize;
@property (atomic) BOOL resized;

@property (strong, atomic) GBGameboyInstance *gameboyInstance;

- (NSData *) loadFile:(NSString *)path;
- (NSInteger) compileShader:(NSData *)data ofType:(NSInteger)type;
- (NSInteger) createShaderProgram:(NSDictionary *)shaders;

- (BOOL) acceptsFirstResponder;
- (void) keyDown:(NSEvent *)event;
- (void) keyUp:(NSEvent *)event;

- (void) setupContext;
- (void) setupDisplayLink;
- (void) setup;
- (void) callRenderer:(SEL)cmd;

- (void) setupRenderer;
- (void) render;

@end

extern CVReturn GBRenderLoop(CVDisplayLinkRef link, const CVTimeStamp *now, const CVTimeStamp *time, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *context);
