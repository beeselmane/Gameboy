#ifndef __LIBGB_DISASM__
#define __LIBGB_DISASM__ 1

#include <stdint.h>

typedef struct {
    uint8_t opcode;
    uint8_t length;

    const char *format;
} GBDisassemblerOP;

typedef struct {
    GBDisassemblerOP *op;

    uint8_t instruction;
    uint16_t argument;
    uint32_t offset;

    char *string;
} GBDisassemblyInfo;

GBDisassemblyInfo **GBDisassemblerProcess(uint8_t *code, uint32_t length, uint32_t *instructions);

GBDisassemblyInfo *GBDisassembleSingle(uint8_t *code, uint32_t length, uint32_t *used);

void GBDisassembleSingleTo(uint8_t *code, uint32_t length, char *to, uint64_t size);

#endif /* !defined(__LIBGB_DISASM__) */
