#ifndef __gameboy_disasm__
#define __gameboy_disasm__ 1

#include "base.h"

typedef struct {
    UInt8 opcode;
    UInt8 length;

    const char *format;
} GBDisassemblerOP;

typedef struct {
    UInt8 instruction;
    UInt16 argument;
    UInt32 offset;

    char *string;
} GBDisassemblyInfo;

GBDisassemblyInfo **GBDisassemblerProcess(UInt8 *code, UInt32 length, UInt32 *instructions);

GBDisassemblyInfo *GBDisassembleSingle(UInt8 *code, UInt32 length, UInt32 *used);

#endif /* !defined(__gameboy_disasm__) */
