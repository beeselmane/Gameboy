#include "disasm.h"
#include <stdio.h>

#define kGBDisassembler 1

#define op(code, len, str, impl)                        \
    const static GBDisassemblerOP op_ ## code = {       \
        .opcode = code,                                 \
        .length = len,                                  \
        .format = str                                   \
    }

#define op_pre(code, len, str, impl)                    \
    const static GBDisassemblerOP op_pre_ ## code = {   \
        .opcode = code,                                 \
        .length = len,                                  \
        .format = str                                   \
    }

#include "指令集.h"

#define expand(v, p)                                                                        \
    &op_ ## p ## v ## 0, &op_ ## p ## v ## 1, &op_ ## p ## v ## 2, &op_ ## p ## v ## 3,     \
    &op_ ## p ## v ## 4, &op_ ## p ## v ## 5, &op_ ## p ## v ## 6, &op_ ## p ## v ## 7,     \
    &op_ ## p ## v ## 8, &op_ ## p ## v ## 9, &op_ ## p ## v ## A, &op_ ## p ## v ## B,     \
    &op_ ## p ## v ## C, &op_ ## p ## v ## D, &op_ ## p ## v ## E, &op_ ## p ## v ## F

const static GBDisassemblerOP *const gGBDisassemblerTableCB[0x100] = {
    expand(0x0, pre_),  expand(0x1, pre_), expand(0x2, pre_), expand(0x3, pre_),
    expand(0x4, pre_),  expand(0x5, pre_), expand(0x6, pre_), expand(0x7, pre_),
    expand(0x8, pre_),  expand(0x9, pre_), expand(0xA, pre_), expand(0xB, pre_),
    expand(0xC, pre_),  expand(0xD, pre_), expand(0xE, pre_), expand(0xF, pre_)
};

const static GBDisassemblerOP *const gGBDisassemblerTable[0x100] = {
    expand(0, 0x), expand(1, 0x), expand(2, 0x), expand(3, 0x),
    expand(4, 0x), expand(5, 0x), expand(6, 0x), expand(7, 0x),
    expand(8, 0x), expand(9, 0x), expand(A, 0x), expand(B, 0x),
    expand(C, 0x), expand(D, 0x), expand(E, 0x), expand(F, 0x)
};

#undef expand

GBDisassemblyInfo **GBDisassemblerProcess(UInt8 *code, UInt32 length, UInt32 *instructions)
{
    GBDisassemblyInfo **output = malloc(length * sizeof(GBDisassemblyInfo *));
    UInt32 read = 0;
    UInt32 i = 0;

    if (!output)
        return NULL;

    while (i < length)
    {
        const GBDisassemblerOP *op;
        UInt8 next = code[i++];

        if (next == 0xCB) {
            op = gGBDisassemblerTableCB[code[i++]];

            if (i >= length)
                break;
        } else {
            op = gGBDisassemblerTable[next];
        }

        if ((i + op->length - 2) >= length)
            break;

        output[read] = malloc(sizeof(GBDisassemblyInfo));
        if (!output[read]) break;

        output[read]->instruction = op->opcode;
        output[read]->offset = (i - 1);

        if (next == 0xCB)
            output[read]->offset--;

        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wformat-security"
        if (op->opcode != 0x10) {
            switch (op->length)
            {
                case 1: {
                    asprintf(&output[read]->string, op->format);
                    output[read]->argument = 0;
                } break;
                case 2: {
                    UInt8 argument = code[i++];
                    output[read]->argument = argument;

                    asprintf(&output[read]->string, op->format, argument);
                } break;
                case 3: {
                    UInt16 argument = code[i++];
                    argument |= ((UInt16)(code[i++])) << 8;

                    asprintf(&output[read]->string, op->format, argument);
                    output[read]->argument = argument;
                } break;
                default:
                    free(output[read]);
                    output[read] = NULL;
            }
        } else {
            asprintf(&output[read]->string, op->format);

            // Only some assemblers put the extra 0x00 here apparently.
            if (code[i] == 0x00)
                i++;
        }
        #pragma clang diagnostic pop

        if (!output[read++])
            break;
    }

    GBDisassemblyInfo **result = malloc((read + 1) * sizeof(GBDisassemblyInfo *));
    memcpy(result, output, read * sizeof(GBDisassemblyInfo *));
    result[read] = NULL;

    if (instructions)
        *instructions = read;

    return result;
}

GBDisassemblyInfo *GBDisassembleSingle(UInt8 *code, UInt32 length, UInt32 *used)
{
    if (length < 1)
        return NULL;

    const GBDisassemblerOP *op = NULL;
    UInt8 opcode = code[0];

    if (opcode == 0xCB)
    {
        if (length < 2)
            return NULL;

        op = gGBDisassemblerTableCB[code[1]];
        opcode = code[1];

        GBDisassemblyInfo *result = malloc(sizeof(GBDisassemblyInfo));
        if (!result) return NULL;

        result->instruction = opcode;
        result->argument = 0;
        result->offset = 0;

        asprintf(&result->string, "%s", op->format);

        if (!result->string)
        {
            free(result);

            return NULL;
        }

        return result;
    }

    op = gGBDisassemblerTable[opcode];

    if (op->length > length)
        return NULL;

    GBDisassemblyInfo *result = malloc(sizeof(GBDisassemblyInfo));
    if (!result) return NULL;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    switch (op->length)
    {
        case 1: {
            asprintf(&result->string, op->format);
            result->argument = 0;
        } break;
        case 2: {
            UInt8 argument = code[1];
            result->argument = argument;

            asprintf(&result->string, op->format, argument);
        } break;
        case 3: {
            UInt16 argument = code[1];
            argument |= ((UInt16)(code[2])) << 8;

            asprintf(&result->string, op->format, argument);
            result->argument = argument;
        } break;
    }
#pragma clang diagnostic pop

    result->instruction = opcode;
    result->offset = 0;

    return result;
}
