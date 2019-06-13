#import "GBScreen.h"
#import "GBGameboyInstance.h"
#import "GBAppDelegate.h"

#import <OpenGL/gl3.h>

extern CVReturn GBRenderLoop(CVDisplayLinkRef link, const CVTimeStamp *now, const CVTimeStamp *time, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *context);

CVReturn GBRenderLoop(CVDisplayLinkRef link, const CVTimeStamp *now, const CVTimeStamp *time, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *context)
{
    GBScreenView *view = (__bridge GBScreenView *)context;

    [view callRenderer:[view rendererName]];

    return kCVReturnSuccess;
}

@interface GBScreenView ()

@property (atomic, readonly) NSInteger vertexArray;
@property (atomic, readonly) NSInteger texture;

@end

@implementation GBScreenWindow

@synthesize screenView = _screenView;

- (instancetype) init
{
    return (self = [self initWithWindowNibName:@"GBScreen"]);
}

- (void) cleanup
{
    [[self screenView] cancelDisplayLink];
}

- (void) resize:(NSUInteger)multiplier
{
    CGFloat titleHeight = [[self window] frame].size.height - [[self window] contentRectForFrameRect:[[self window] frame]].size.height;
    CGSize size = NSMakeSize(160.0F * multiplier, 144.0F * multiplier);
    size.height += titleHeight;

    [[self window] setFrame:(CGRect){[[self window] frame].origin, size} display:NO];
    [[self screenView] setFrame:(CGRect){CGPointZero, CGSizeMake(size.width, size.height - titleHeight)}];
}

- (void) windowDidLoad
{
    [super windowDidLoad];
    
    [[self screenView] setup];
}

- (BOOL) windowShouldClose:(NSWindow *)sender
{
    [NSApp terminate:self];

    return NO;
}

@end

@implementation GBScreenView

@synthesize displayLink = _displayLink;
@synthesize shaderProgram = _shaderProgram;
@synthesize rendererName = _rendererName;

@synthesize lastFrameTime = _lastFrameTime;
@synthesize frameDelta = _frameDelta;

@synthesize emulationSpeed = _emulationSpeed;

@synthesize newSize = _newSize;
@synthesize resized = _resized;

@synthesize vertexArray = _vertexArray;
@synthesize texture = _texture;

#pragma mark - OpenGL Shader Compilation

- (NSData *) loadFile:(NSString *)path
{
    return [[NSFileManager defaultManager] contentsAtPath:path];
}

- (NSInteger) compileShader:(NSData *)data ofType:(NSInteger)type
{
    unsigned int shader = glCreateShader((GLenum)type);
    UInt8 *source = malloc([data length] + 1);

    [data getBytes:source length:[data length]];
    source[[data length]] = 0;

    glShaderSource(shader, 1, (const GLchar *const *)&source, NULL);
    glCompileShader(shader);
    free(source);

    char log[512];
    int status;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (!status)
    {
        glGetShaderInfoLog(shader, 512, NULL, log);

        NSLog(@"[OpenGL Shader] Compilation error: %s\n", log);
        NSLog(@"[OpenGL Shader] Error for source:  %s\n", source);

        NSAssert(status, @"OpenGL Shader compilation error!");
    }

    return shader;
}

- (NSInteger) createShaderProgram:(NSDictionary *)shaders
{
    NSMutableArray *compiledShaders = [[NSMutableArray alloc] initWithCapacity:[shaders count]];
    NSInteger program = glCreateProgram();

    for (NSNumber *key in [shaders allKeys])
    {
        NSData *shaderSource = [self loadFile:[shaders objectForKey:key]];
        NSAssert(shaderSource, @"OpenGL Shader source not found at %@!", [shaders objectForKey:key]);

        NSInteger shader = [self compileShader:shaderSource ofType:[key integerValue]];
        [compiledShaders addObject:[NSNumber numberWithInteger:shader]];
        glAttachShader((GLuint)program, (GLuint)shader);
    }

    glLinkProgram((GLuint)program);

    char log[512];
    int status;

    glGetProgramiv((GLuint)program, GL_LINK_STATUS, &status);

    if (!status)
    {
        glGetProgramInfoLog((GLuint)program, 512, 0, log);

        NSLog(@"[OpenGL Shader] Program link error: %s\n", log);

        NSAssert(status, @"OpenGL Shader link error!");
    }

    glUseProgram((GLuint)program);

    for (NSNumber *shader in compiledShaders)
        glDeleteShader((GLuint)[shader integerValue]);

    return program;
}

#pragma mark - User Interface Controls

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) keyDown:(NSEvent *)event
{
    NSDictionary *map = [[NSUserDefaults standardUserDefaults] dictionaryForKey:kGBSettingsKeymap];
    NSString *keys = [event charactersIgnoringModifiers];

    for (NSUInteger i = 0; i < [keys length]; i++)
    {
        NSString *keycode = [keys substringWithRange:NSMakeRange(i, 1)];

        if ([map objectForKey:keycode])
            [[[GBAppDelegate instance] gameboy] pressKey:[[map objectForKey:keycode] integerValue]];
    }
}

- (void) keyUp:(NSEvent *)event
{
    NSDictionary *map = [[NSUserDefaults standardUserDefaults] dictionaryForKey:kGBSettingsKeymap];
    NSString *keys = [event charactersIgnoringModifiers];

    for (NSUInteger i = 0; i < [keys length]; i++)
    {
        NSString *keycode = [keys substringWithRange:NSMakeRange(i, 1)];

        if ([map objectForKey:keycode])
            [[[GBAppDelegate instance] gameboy] liftKey:[[map objectForKey:keycode] integerValue]];
    }
}

#pragma mark - GL Setup

- (void) setupContext
{
    NSOpenGLPixelFormatAttribute pixelAttributes[6] = {NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 24, NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core, 0};
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:pixelAttributes];

    NSOpenGLContext *context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
    [self setPixelFormat:pixelFormat];
    [self setOpenGLContext:context];
}

- (void) setupDisplayLink
{
    CGLPixelFormatObj pixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CGLContextObj context = [[self openGLContext] CGLContextObj];

    CVDisplayLinkCreateWithActiveCGDisplays(&self->_displayLink);
    CVDisplayLinkSetOutputCallback([self displayLink], GBRenderLoop, (__bridge void *)(self));
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext([self displayLink], context, pixelFormat);
    CVDisplayLinkStart([self displayLink]);
}

- (void) cancelDisplayLink
{
    CVDisplayLinkStop([self displayLink]);
    CVDisplayLinkRelease([self displayLink]);
}

- (void) setup
{
    [self setEmulationSpeed:1.0F];

    [self setupContext];
    [self setupDisplayLink];

    [self setRendererName:@selector(setupRenderer)];
}

- (void) callRenderer:(SEL)cmd
{
    @autoreleasepool
    {
        CGLLockContext([[self openGLContext] CGLContextObj]);
        [[self openGLContext] makeCurrentContext];

        CFAbsoluteTime time = CFAbsoluteTimeGetCurrent();
        self->_frameDelta = time - [self lastFrameTime];

        // Workaround for possible leak warning on '[self performSelector:cmd]'
        // This line is logically and functionally equivalent to the above.
        ((void (*)(id, SEL))[self methodForSelector:cmd])(self, cmd);
        self->_lastFrameTime = time;

        // Called every frame
        [[GBAppDelegate instance] updateFrame];

        [[self openGLContext] flushBuffer];
        CGLUnlockContext([[self openGLContext] CGLContextObj]);
    }
}

#pragma mark - Rendering

- (void) setupRenderer
{
    NSString *fragmentShader = [[NSBundle mainBundle] pathForResource:@"GBFragment" ofType:@"glsl"];
    NSString *vertexShader   = [[NSBundle mainBundle] pathForResource:@"GBVertex"   ofType:@"glsl"];

    NSAssert(fragmentShader && vertexShader, @"Could not find OpenGL shaders!");

    self->_shaderProgram = [self createShaderProgram:@{
                                                       @GL_FRAGMENT_SHADER : fragmentShader,
                                                       @GL_VERTEX_SHADER   : vertexShader
                                                       }];

    NSAssert([self shaderProgram] != -1, @"Error compiling OpenGL shaders!");

    glGenVertexArrays(1, (GLuint *)&_vertexArray);
    glBindVertexArray((GLuint)[self vertexArray]);

    glGenTextures(1, (GLuint *)&_texture);
    glBindTexture(GL_TEXTURE_2D, (GLuint)[self texture]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 160, 144, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, [[[GBAppDelegate instance] gameboy] screen]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glActiveTexture(GL_TEXTURE0);

    NSInteger location = glGetUniformLocation((GLuint)[self shaderProgram], "screenBuffer");
    glUniform1i((GLuint)location, 0);

    [self setRendererName:@selector(render)];
}

- (void) render
{
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    CGFloat cps = 4194304;
    CGFloat ticks = cps * [self frameDelta];
    ticks *= [self emulationSpeed];

    [[[GBAppDelegate instance] gameboy] tick:(NSUInteger)ticks];
    glBindVertexArray((GLuint)[self vertexArray]);

    glActiveTexture(GL_TEXTURE0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 160, 144, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, [[[GBAppDelegate instance] gameboy] screen]);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

@end
