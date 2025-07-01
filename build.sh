SDLLIBS="-Wl,-framework,CoreMedia -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-weak_framework,UniformTypeIdentifiers -Wl,-framework,IOKit -Wl,-framework,ForceFeedback -Wl,-framework,Carbon -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox -Wl,-framework,AVFoundation -Wl,-framework,Foundation -Wl,-framework,GameController -Wl,-framework,Metal -Wl,-framework,QuartzCore -Wl,-weak_framework,CoreHaptics -lpthread -lm"
LDFLAGS="-framework CoreFoundation -framework Security ${SDLLIBS} -Wno-unused-command-line-argument"
CFLAGS="-Ilibgb -I${HOME}/opt/sdl3/include"

set -x

make -C libgb

mkdir -pv build
cc ${CFLAGS} -o build/main.o -c sdl/main.c 
cc ${CFLAGS} -o build/gameboy.o -c sdl/gameboy.c 
cc ${LDFLAGS} -o build/sdlgb build/main.o build/gameboy.o libgb/build/libgb.a ~/opt/sdl3/lib/libSDL3.a
