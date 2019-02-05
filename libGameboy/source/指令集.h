// We can make a disasembler really easily if we play with these macros right.
// To create a disassembler, just redefine the op macros such that the implementation is left empty
// and you have an effective map of opcode --> string and length.
// Then, disassembly is pretty easy, it's just a simple map

#ifndef kGBDisassembler

#define declare(op, str)                                                                        \
    static void __GBProcessorDo_ ## op(GBProcessorOP *this, struct __GBProcessor *cpu);         \
                                                                                                \
    const static GBProcessorOP op_## op = {                                                     \
        .name = str,                                                                            \
        .value = op,                                                                            \
        .go = __GBProcessorDo_ ## op                                                            \
    }

#define op(code, len, str, impl)                                                                \
    declare(code, str);                                                                         \
                                                                                                \
    void __GBProcessorDo_ ## code(GBProcessorOP *this, struct __GBProcessor *cpu)               \
    impl

#define op_pre(code, len, str, impl)                                                            \
    static void __GBProcessorDo_pre_ ## code(GBProcessorOP *this, struct __GBProcessor *cpu);   \
                                                                                                \
    const static GBProcessorOP op_pre_ ## code = {                                              \
        .name = str,                                                                            \
        .value = code,                                                                          \
        .go = __GBProcessorDo_pre_ ## code                                                      \
    };                                                                                          \
                                                                                                \
    void __GBProcessorDo_pre_ ## code(GBProcessorOP *this, struct __GBProcessor *cpu)           \
    impl

#pragma mark - Memory Access

static __attribute__((always_inline)) void __GBProcessorRead(GBProcessor *cpu, UInt16 address)
{
    cpu->state.accessed = false;
    cpu->state.mar = address;

    GBMemoryManagerReadRequest(cpu->mmu, &cpu->state.mar, &cpu->state.mdr, &cpu->state.accessed);
}

static __attribute__((always_inline)) void __GBProcessorWrite(GBProcessor *cpu, UInt16 address, UInt8 data)
{
    cpu->state.accessed = false;
    cpu->state.mar = address;
    cpu->state.mdr = data;

    GBMemoryManagerWriteRequest(cpu->mmu, &cpu->state.mar, &cpu->state.mdr, &cpu->state.accessed);
}

static __attribute__((always_inline)) void __GBProcessorReadArgument(GBProcessor *cpu)
{
    __GBProcessorRead(cpu, cpu->state.pc++);
}

#pragma mark - ALU

static __attribute__((always_inline)) void __ALUAdd8(GBProcessor *cpu, UInt8 *to, UInt8 *from, UInt8 carry)
{
    // Perform two 4-bit adds (this is how the gameboy actually does this)
    UInt8 lo = ((*from) & 0xF) + ((*to) & 0xF) + carry;
    cpu->state.f.h = lo >> 4;

    UInt8 hi = ((*from) >> 4) + ((*to) >> 4) + cpu->state.f.h;
    cpu->state.f.c = hi >> 4;

    (*to) = (hi << 4) | (lo & 0xF);

    cpu->state.f.z = !(*to);
    cpu->state.f.n = 0;
}

static __attribute__((always_inline)) void __ALUAdd16(GBProcessor *cpu, UInt16 *to, UInt16 *from, UInt8 carry)
{
    // Perform two 8-bit adds (four 4-bit adds). Flags should automatically be right.
    __ALUAdd8(cpu, (UInt8 *)(to) + 0, (UInt8 *)(from) + 0, carry);
    __ALUAdd8(cpu, (UInt8 *)(to) + 1, (UInt8 *)(from) + 1, cpu->state.f.c);
}

static __attribute__((always_inline)) void __ALUSub8(GBProcessor *cpu, SInt8 *to, SInt8 *from, UInt8 carry)
{
    // I forgot how 2's complement works so I did this instead
    SInt8 result = (*to) - (*from) + carry;

    cpu->state.f.z = !result;
    cpu->state.f.n = 1;
    cpu->state.f.h = (((*to) & 0x0F) < ((*from) & 0x0F));
    cpu->state.f.c = (result < 0);

    (*to) = result;
}

static __attribute__((always_inline)) UInt16 __ALUSignExtend(UInt8 value)
{
    if (value >> 7) {
        return 0xFF00 | value;
    } else {
        return 0x0000 | value;
    }
}

static __attribute__((always_inline)) void __ALUAnd8(GBProcessor *cpu, UInt8 *to, UInt8 *from)
{
    *to = *to & *from;

    cpu->state.f.z = !(*to);
    cpu->state.f.n = 0;
    cpu->state.f.h = 1;
    cpu->state.f.c = 0;
}

static __attribute__((always_inline)) void __ALUXor8(GBProcessor *cpu, UInt8 *to, UInt8 *from)
{
    *to = *to ^ *from;

    cpu->state.f.z = !(*to);
    cpu->state.f.n = 0;
    cpu->state.f.h = 0;
    cpu->state.f.c = 0;
}

static __attribute__((always_inline)) void __ALUOr8(GBProcessor *cpu, UInt8 *to, UInt8 *from)
{
    *to = *to | *from;

    cpu->state.f.z = !(*to);
    cpu->state.f.n = 0;
    cpu->state.f.h = 0;
    cpu->state.f.c = 0;
}

static __attribute__((always_inline)) void __ALUCP8(GBProcessor *cpu, UInt8 *to, UInt8 *from)
{
    UInt8 initial = *to;

    __ALUSub8(cpu, (SInt8 *)to, (SInt8 *)from, 0);

    *to = initial;
}

static __attribute__((always_inline)) void __ALURotateRight(UInt8 *reg, UInt8 amount)
{
    __asm__ ("rorb %1, %0"
             : "+mq" (*reg)
             : "cI"  (amount));
}

static __attribute__((always_inline)) void __ALURotateLeft(UInt8 *reg, UInt8 amount)
{
    __asm__ ("rolb %1, %0"
             : "+mq" (*reg)
             : "cI"  (amount));
}

#endif /* !defined(kGBDisassembler) */

#pragma mark - Data Formatting Macros

// Instruction data formats
#ifndef kGBNumberFormatting
    #define d16 "$0x%04X"
    #define d8  "$0x%02X"
    #define dIO "0xFF00"
#endif /* !defined(kGBNumberFormatting) */

#pragma mark - Argument Wrappers

#define op_simple(code, str, impl)                                          \
    op(code, 1, str, {                                                      \
        impl;                                                               \
                                                                            \
        cpu->state.mode = kGBProcessorModeFetch;                            \
    })

#define op_stall(code, len, str, impl)                                      \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            impl;                                                           \
                                                                            \
            __GBProcessorRead(cpu, 0x0000);                                 \
                                                                            \
            cpu->state.mode = kGBProcessorModeStalled;                      \
        } else if (cpu->state.mode == kGBProcessorModeStalled) {            \
            if (cpu->state.accessed)                                        \
                cpu->state.mode = kGBProcessorModeFetch;                    \
        }                                                                   \
    })

#define op_wait(code, len, str, pre, post)                                  \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            pre;                                                            \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                post;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeFetch;                    \
            }                                                               \
        }                                                                   \
    })

#define op_wait_stall(code, len, str, pre, post)                            \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            pre;                                                            \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                post;                                                       \
                                                                            \
                __GBProcessorRead(cpu, 0x0000);                             \
                                                                            \
                cpu->state.mode = kGBProcessorModeStalled;                  \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeStalled) {            \
            if (cpu->state.accessed)                                        \
                cpu->state.mode = kGBProcessorModeFetch;                    \
        }                                                                   \
    })

#define op_wait2(code, len, str, pre, mid, post)                            \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            pre;                                                            \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                mid;                                                        \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait2;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait2) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                post;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeFetch;                    \
            }                                                               \
        }                                                                   \
    })

#define op_wait2_stall(code, len, str, pre, mid, post)                      \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            pre;                                                            \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                mid;                                                        \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait2;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait2) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                post;                                                       \
                                                                            \
                __GBProcessorRead(cpu, 0x0000);                             \
                                                                            \
                cpu->state.mode = kGBProcessorModeStalled;                  \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeStalled) {            \
            if (cpu->state.accessed)                                        \
                cpu->state.mode = kGBProcessorModeFetch;                    \
        }                                                                   \
    })

#define op_wait3(code, len, str, pre, mid1, mid2, post)                     \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            pre;                                                            \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                mid1;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait2;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait2) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                mid2;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait3;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait3) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                post;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeFetch;                    \
            }                                                               \
        }                                                                   \
    })

#define op_wait3_stall(code, len, str, pre, mid1, mid2, post)               \
    op(code, len, str, {                                                    \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            pre;                                                            \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                mid1;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait2;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait2) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                mid2;                                                       \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait3;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait3) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                post;                                                       \
                                                                            \
                __GBProcessorRead(cpu, 0x0000);                             \
                                                                            \
                cpu->state.mode = kGBProcessorModeStalled;                  \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeStalled) {            \
            if (cpu->state.accessed)                                        \
                cpu->state.mode = kGBProcessorModeFetch;                    \
        }                                                                   \
    })

#define op_hl(code, str, impl)                                              \
    op_wait(code, 1, str, {                                                 \
        __GBProcessorRead(cpu, cpu->state.hl);                              \
    }, impl)

#define op_arg(code, str, impl)                                             \
    op_wait(code, 2, str, {                                                 \
        __GBProcessorReadArgument(cpu);                                     \
    }, impl)

#define op_arg16(code, str, intr, impl)                                     \
    op_wait2(code, 3, str, {                                                \
        __GBProcessorReadArgument(cpu);                                     \
    }, {                                                                    \
        intr;                                                               \
                                                                            \
        __GBProcessorReadArgument(cpu);                                     \
    }, impl)

#define op_pre_simple(code, str, impl)                                      \
    op_pre(code, 1, str, {                                                  \
        impl;                                                               \
                                                                            \
        cpu->state.mode = kGBProcessorModeFetch;                            \
    })

#define op_pre_hl(code, str, impl)                                          \
    op_pre(code, 1, str, {                                                  \
        if (cpu->state.mode == kGBProcessorModeRun) {                       \
            __GBProcessorRead(cpu, cpu->state.hl);                          \
                                                                            \
            cpu->state.mode = kGBProcessorModeWait1;                        \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {              \
            if (cpu->state.accessed)                                        \
            {                                                               \
                impl;                                                       \
                                                                            \
                __GBProcessorWrite(cpu, cpu->state.hl, cpu->state.mdr);     \
                                                                            \
                cpu->state.mode = kGBProcessorModeWait2;                    \
            }                                                               \
        } else if (cpu->state.mode == kGBProcessorModeWait2) {              \
            if (cpu->state.accessed)                                        \
                cpu->state.mode = kGBProcessorModeFetch;                    \
        }                                                                   \
    })

#pragma mark - Load Macros

#define ld_r(code, dst, src)                                                \
    op_simple(code, "ld " #dst ", " #src, {                                 \
        cpu->state.dst = cpu->state.src;                                    \
    })


#define ld_rr(code, regs)                                                   \
    op_arg16(code, "ld " #regs ", " d16, {                                  \
        cpu->state.regs = cpu->state.mdr;                                   \
    }, {                                                                    \
        cpu->state.regs |= cpu->state.mdr << 8;                             \
    })

#define ld_a_rr(code, regs)                                                 \
    op_wait(code, 1, "ld a, (" #regs ")", {                                 \
        __GBProcessorRead(cpu, cpu->state.regs);                            \
    }, {                                                                    \
        cpu->state.a = cpu->state.mdr;                                      \
    })

#define ld_rr_a(code, regs)                                                 \
    op_wait(code, 1, "ld (" #regs "), a", {                                 \
        __GBProcessorWrite(cpu, cpu->state.regs, cpu->state.a);             \
    }, {                                                                    \
        /* nothing */                                                       \
    })

#define ld_m(code, reg)                                                     \
    op_arg(code, "ld " #reg ", " d8, {                                      \
        cpu->state.reg = cpu->state.mdr;                                    \
    })

#define ld_hl(code, reg)                                                    \
    op_wait(code, 1, "ld (hl), " #reg, {                                    \
        __GBProcessorWrite(cpu, cpu->state.hl, cpu->state.reg);             \
    }, {                                                                    \
        /* nothing */                                                       \
    })

#define ld_r_hl(code, reg)                                                  \
    op_hl(code, "ld " #reg ", (hl)", {                                      \
        cpu->state.reg = cpu->state.mdr;                                    \
    })

#pragma mark - Control flow macros

// TODO: Implement these
#define jr(code, str, cond)                                                             \
    op_wait_stall(code, 2, "jr " str d8, {                                              \
        __GBProcessorReadArgument(cpu);                                                 \
    }, {                                                                                \
        if (!cond)                                                                      \
        {                                                                               \
            cpu->state.mode = kGBProcessorModeFetch;                                    \
                                                                                        \
            return;                                                                     \
        }                                                                               \
                                                                                        \
        cpu->state.pc += __ALUSignExtend(cpu->state.mdr);                               \
    })

#define jmp(code, str, cond)                                                            \
    op_wait2_stall(code, 3, "jp " str d16, {                                            \
        __GBProcessorReadArgument(cpu);                                                 \
    }, {                                                                                \
        __GBProcessorReadArgument(cpu);                                                 \
                                                                                        \
        cpu->state.data = cpu->state.mdr;                                               \
    }, {                                                                                \
        if (!cond)                                                                      \
        {                                                                               \
            cpu->state.mode = kGBProcessorModeFetch;                                    \
                                                                                        \
            return;                                                                     \
        }                                                                               \
                                                                                        \
        cpu->state.pc = cpu->state.data | (cpu->state.mdr << 8);                        \
    })

#define call(code, str, cond)                                                           \
    op(code, 3, "call " str d16, {                                                      \
        if (cpu->state.mode == kGBProcessorModeRun) {                                   \
            __GBProcessorReadArgument(cpu);                                             \
                                                                                        \
            cpu->state.mode = kGBProcessorModeWait1;                                    \
        } else if (cpu->state.mode == kGBProcessorModeWait1) {                          \
            if (cpu->state.accessed)                                                    \
            {                                                                           \
                cpu->state.data = cpu->state.mdr;                                       \
                                                                                        \
                __GBProcessorReadArgument(cpu);                                         \
                                                                                        \
                cpu->state.mode = kGBProcessorModeWait2;                                \
            }                                                                           \
        } else if (cpu->state.mode == kGBProcessorModeWait2) {                          \
            if (cpu->state.accessed)                                                    \
            {                                                                           \
                cpu->state.data |= (cpu->state.mdr << 8);                               \
                                                                                        \
                if (!cond)                                                              \
                {                                                                       \
                    cpu->state.mode = kGBProcessorModeFetch;                            \
                                                                                        \
                    return;                                                             \
                }                                                                       \
                                                                                        \
                __GBProcessorWrite(cpu, cpu->state.sp - 1, cpu->state.pc >> 8);         \
                cpu->state.mode = kGBProcessorModeWait3;                                \
            }                                                                           \
        } else if (cpu->state.mode == kGBProcessorModeWait3) {                          \
            if (cpu->state.accessed)                                                    \
            {                                                                           \
                __GBProcessorWrite(cpu, cpu->state.sp - 2, cpu->state.pc & 0xFF);       \
                                                                                        \
                cpu->state.mode = kGBProcessorModeWait4;                                \
            }                                                                           \
        } else if (cpu->state.mode == kGBProcessorModeWait4) {                          \
            if (cpu->state.accessed)                                                    \
            {                                                                           \
                __GBProcessorRead(cpu, 0x0000);                                         \
                                                                                        \
                cpu->state.pc = cpu->state.data;                                        \
                cpu->state.sp -= 2;                                                     \
                                                                                        \
                cpu->state.mode = kGBProcessorModeStalled;                              \
            }                                                                           \
        } else if (cpu->state.mode == kGBProcessorModeStalled) {                        \
            if (cpu->state.accessed)                                                    \
                cpu->state.mode = kGBProcessorModeFetch;                                \
        }                                                                               \
    })

#define ret(code, str, cond)                                                            \
    op_wait3_stall(code, 1, "ret " str, {                                               \
        __GBProcessorRead(cpu, 0x0000);                                                 \
    }, {                                                                                \
        if (!cond)                                                                      \
        {                                                                               \
            cpu->state.mode = kGBProcessorModeFetch;                                    \
                                                                                        \
            return;                                                                     \
        }                                                                               \
                                                                                        \
        __GBProcessorRead(cpu, cpu->state.sp + 0);                                      \
    }, {                                                                                \
        __GBProcessorRead(cpu, cpu->state.sp + 1);                                      \
                                                                                        \
        cpu->state.pc = cpu->state.mdr;                                                 \
    }, {                                                                                \
        cpu->state.pc |= cpu->state.mdr << 8;                                           \
        cpu->state.sp += 2;                                                             \
    })

#define rst(code, dst)                                                                  \
    op_wait2_stall(code, 1, "rst $" #dst, {                                             \
        cpu->state.pc = dst;                                                            \
    }, {                                                                                \
        __GBProcessorRead(cpu, 0x0000);                                                 \
    }, {                                                                                \
        /* nothing */                                                                   \
    })

#pragma mark - Stack manipulation macros

#define pop(code, regs)                                                     \
    op_wait2_stall(code, 1, "pop " #regs, {                                 \
        __GBProcessorRead(cpu, cpu->state.sp + 0);                          \
    }, {                                                                    \
        cpu->state.regs = cpu->state.mdr;                                   \
                                                                            \
        __GBProcessorRead(cpu, cpu->state.sp + 1);                          \
    }, {                                                                    \
        cpu->state.regs |= cpu->state.mdr << 8;                             \
        cpu->state.sp += 2;                                                 \
    })

#define push(code, regs)                                                    \
    op_wait2_stall(code, 1, "push " #regs, {                                \
        __GBProcessorWrite(cpu, cpu->state.sp - 1, cpu->state.regs >> 8);   \
    }, {                                                                    \
        __GBProcessorWrite(cpu, cpu->state.sp - 2, cpu->state.regs & 0xFF); \
    }, {                                                                    \
        cpu->state.sp -= 2;                                                 \
    })

#pragma mark - Undefined instruction macros

#define udef(code)                                                                      \
    op(code, 1, "ud " #code, {                                                          \
        fprintf(stderr, "Error: Undefined instruction 0x%02X called.\n", this->value);  \
        fprintf(stderr, "Note: A real gameboy would lockup here.\n");                   \
                                                                                        \
        cpu->state.mode = kGBProcessorModeOff;                                          \
    })

#pragma mark - Logical operation macros

#define inc_r(code, reg)                                                    \
    op_simple(code, "inc " #reg, {                                          \
        UInt8 carry = cpu->state.f.c;                                       \
        UInt8 one = 1;                                                      \
                                                                            \
        __ALUAdd8(cpu, &cpu->state.reg, &one, 0);                           \
        cpu->state.f.c = carry;                                             \
    })

#define dec_r(code, reg)                                                    \
    op_simple(code, "dec " #reg, {                                          \
        UInt8 carry = cpu->state.f.c;                                       \
        SInt8 one = 1;                                                      \
                                                                            \
        __ALUSub8(cpu, (SInt8 *)&cpu->state.reg, &one, 0);                  \
        cpu->state.f.c = carry;                                             \
    })

#define inc_rr(code, regs)                                                  \
    op_stall(code, 1, "inc " #regs, {                                       \
            cpu->state.regs++;                                              \
    })

#define dec_rr(code, regs)                                                  \
    op_stall(code, 1, "dec " #regs, {                                       \
        cpu->state.regs--;                                                  \
    })

#define add_hl(code, reg)                                                   \
    op_stall(code, 1, "add hl, " #reg, {                                    \
        UInt8 zero = cpu->state.f.z;                                        \
                                                                            \
        __ALUAdd16(cpu, &cpu->state.hl, &cpu->state.reg, 0);                \
                                                                            \
        cpu->state.f.z = zero;                                              \
    })

#define math_op(code, name, reg, func, c)                                   \
    op_simple(code, name " a, " #reg, {                                     \
        func(cpu, (void *)&cpu->state.a, (void *)&cpu->state.reg, c);       \
    })

#define add(code, reg) math_op(code, "add", reg, __ALUAdd8, 0)

#define adc(code, reg) math_op(code, "adc", reg, __ALUAdd8, cpu->state.f.c)

#define sub(code, reg) math_op(code, "sub", reg, __ALUSub8, 0)

#define sbc(code, reg) math_op(code, "sbc", reg, __ALUSub8, cpu->state.f.c)

#define alu_op(code, name, reg, func)                                       \
    op_simple(code, name " " #reg, {                                        \
        func(cpu, &cpu->state.a, &cpu->state.reg);                          \
    })

#define alu_op_hl(code, name, func)                                         \
    op_hl(code, name " (hl)", {                                             \
        func(cpu, &cpu->state.a, &cpu->state.mdr);                          \
    })

#define alu_op_arg(code, name, func)                                        \
    op_arg(code, name " " d8, {                                             \
        func(cpu, &cpu->state.a, &cpu->state.mdr);                          \
    })

#define and(code, reg) alu_op(code, "and", reg, __ALUAnd8)

#define xor(code, reg) alu_op(code, "xor", reg, __ALUXor8)

#define or(code, reg) alu_op(code, "or", reg, __ALUOr8)

#define cmp(code, reg) alu_op(code, "cp", reg, __ALUCP8)

#pragma mark - Instruction Implementation

op_simple(0x00, "nop", { /* nop doesn't do anything, sorry */ });

ld_rr(0x01, bc);

ld_rr_a(0x02, bc);

inc_rr(0x03, bc);

inc_r(0x04, b);
dec_r(0x05, b);

ld_m(0x06, b);

op_simple(0x07, "rcla", {
    __ALURotateLeft(&cpu->state.a, 1);

    cpu->state.f.z = 0;
    cpu->state.f.n = 0;
    cpu->state.f.h = 0;
    cpu->state.f.c = cpu->state.a & 1;
});

op(0x08, 3, "ld (" d16 "), sp", {
    if (cpu->state.mode == kGBProcessorModeRun) {
        __GBProcessorReadArgument(cpu);

        cpu->state.mode = kGBProcessorModeWait1;
    } else if (cpu->state.mode == kGBProcessorModeWait1) {
        if (cpu->state.accessed)
        {
            cpu->state.data = cpu->state.mdr;

            __GBProcessorReadArgument(cpu);

            cpu->state.mode = kGBProcessorModeWait2;
        }
    } else if (cpu->state.mode == kGBProcessorModeWait2) {
        if (cpu->state.accessed)
        {
            cpu->state.mar = cpu->state.data || (cpu->state.mdr << 8);

            __GBProcessorWrite(cpu, cpu->state.mar, cpu->state.sp & 0xFF);

            cpu->state.mode = kGBProcessorModeWait3;
        }
    } else if (cpu->state.mode == kGBProcessorModeWait3) {
        if (cpu->state.accessed)
        {
            cpu->state.mar++;

            __GBProcessorWrite(cpu, cpu->state.mar, cpu->state.sp >> 8);

            cpu->state.mode = kGBProcessorModeWait4;
        }
    } else if (cpu->state.mode == kGBProcessorModeWait4) {
        if (cpu->state.accessed)
        {
            cpu->state.sp |= ((UInt16)cpu->state.mdr) << 8;
            cpu->state.mode = kGBProcessorModeFetch;
        }
    }
});

add_hl(0x09, bc);

ld_a_rr(0x0A, bc);

dec_rr(0x0B, bc);

inc_r(0x0C, c);
dec_r(0x0D, c);

ld_m(0x0E, c);

op_simple(0x0F, "rrca", {
    __ALURotateRight(&cpu->state.a, 1);

    cpu->state.f.z = 0;
    cpu->state.f.n = 0;
    cpu->state.f.h = 0;
    cpu->state.f.c = cpu->state.a & 1;
});

op(0x10, 2, "stop 0", {
    cpu->state.mode = kGBProcessorModeStopped;

    if (!__GBMemoryManagerRead(cpu->mmu, cpu->state.pc + 1))
        cpu->state.pc++;
});

ld_rr(0x11, de);

ld_rr_a(0x12, de);

inc_rr(0x13, de);

inc_r(0x14, d);
dec_r(0x15, d);

ld_m(0x16, d);

op_simple(0x17, "rla", {
    cpu->state.f.z = 0;
    cpu->state.f.n = 0;
    cpu->state.f.h = cpu->state.f.c;
    cpu->state.f.c = cpu->state.a >> 7;

    cpu->state.a <<= 1;
    cpu->state.a |= cpu->state.f.h;
    cpu->state.f.h = 0;
});

jr(0x18, "", true);

add_hl(0x19, de);

ld_a_rr(0x1A, de);

dec_rr(0x1B, de);

inc_r(0x1C, e);
dec_r(0x1D, e);

ld_m(0x1E, e);

op_simple(0x1F, "rra", {
    cpu->state.f.z = 0;
    cpu->state.f.n = 0;
    cpu->state.f.h = cpu->state.f.c;
    cpu->state.f.c = cpu->state.a & 0;

    cpu->state.a >>= 1;
    cpu->state.a |= cpu->state.f.h << 7;
    cpu->state.f.h = 0;
});

jr(0x20, "nz, ", !cpu->state.f.z);

ld_rr(0x21, hl);

op_wait(0x22, 1, "ld (hl+), a", {
    __GBProcessorWrite(cpu, cpu->state.hl++, cpu->state.a);
}, {
    // We don't need to do anything after
});

inc_rr(0x23, hl);

inc_r(0x24, h);
dec_r(0x25, h);

ld_m(0x26, h);

op(0x27, 1, "daa", {
    //
});

jr(0x28, "z, ", cpu->state.f.z);

add_hl(0x29, hl);

op_hl(0x2A, "ld a, (hl+)", {
    cpu->state.a = cpu->state.mdr;
    cpu->state.hl++;
});

dec_rr(0x2B, hl);

inc_r(0x2C, l);
dec_r(0x2D, l);

ld_m(0x2E, l);

op_simple(0x2F, "cpl", {
    cpu->state.a = ~cpu->state.a;

    cpu->state.f.h = 1;
    cpu->state.f.n = 1;
});

jr(0x30, "nc, ", !cpu->state.f.c);

ld_rr(0x31, sp);

op_wait(0x32, 1, "ld (hl-), a", {
    __GBProcessorWrite(cpu, cpu->state.hl--, cpu->state.a);
}, {
    // We don't need to do anything after
});

inc_rr(0x33, sp);

op_wait2(0x34, 1, "inc (hl)", {
    __GBProcessorRead(cpu, cpu->state.hl);
}, {
    UInt8 carry = cpu->state.f.c;
    UInt8 one = 1;

    __ALUAdd8(cpu, &cpu->state.mdr, &one, 0);
    cpu->state.f.c = carry;

    __GBProcessorWrite(cpu, cpu->state.hl, cpu->state.mdr);
}, {
    // We're done now
});

op_wait2(0x35, 1, "dec (hl)", {
    __GBProcessorRead(cpu, cpu->state.hl);
}, {
    UInt8 carry = cpu->state.f.c;
    SInt8 one = 1;

    __ALUSub8(cpu, (SInt8 *)&cpu->state.mdr, &one, 0);
    cpu->state.f.c = carry;

    __GBProcessorWrite(cpu, cpu->state.hl, cpu->state.mdr);
}, {
    // We're done now
});

op_wait2(0x36, 2, "ld (hl), " d8, {
    __GBProcessorReadArgument(cpu);
}, {
    __GBProcessorWrite(cpu, cpu->state.hl, cpu->state.mdr);
}, {
    // We're done now
});

op_simple(0x37, "scf", {
    cpu->state.f.c = 1;

    cpu->state.f.h = 0;
    cpu->state.f.n = 0;
});

jr(0x38, "c, ", cpu->state.f.c);

add_hl(0x39, sp);

op_hl(0x3A, "ld a, (hl-)", {
    cpu->state.a = cpu->state.mdr;
    cpu->state.hl--;
});

dec_rr(0x3B, sp);

inc_r(0x3C, a);
dec_r(0x3D, a);

ld_m(0x3E, a);

op_simple(0x3F, "ccf", {
    cpu->state.f.c = ~cpu->state.f.c;

    cpu->state.f.h = 0;
    cpu->state.f.n = 0;
});

ld_r(0x40, b, b);
ld_r(0x41, b, c);
ld_r(0x42, b, d);
ld_r(0x43, b, e);
ld_r(0x44, b, h);
ld_r(0x45, b, l);
ld_r_hl(0x46, b);
ld_r(0x47, b, a);

ld_r(0x48, c, b);
ld_r(0x49, c, c);
ld_r(0x4A, c, d);
ld_r(0x4B, c, e);
ld_r(0x4C, c, h);
ld_r(0x4D, c, l);
ld_r_hl(0x4E, c);
ld_r(0x4F, c, a);

ld_r(0x50, d, b);
ld_r(0x51, d, c);
ld_r(0x52, d, d);
ld_r(0x53, d, e);
ld_r(0x54, d, h);
ld_r(0x55, d, l);
ld_r_hl(0x56, d);
ld_r(0x57, d, a);

ld_r(0x58, e, b);
ld_r(0x59, e, c);
ld_r(0x5A, e, d);
ld_r(0x5B, e, e);
ld_r(0x5C, e, h);
ld_r(0x5D, e, l);
ld_r_hl(0x5E, e);
ld_r(0x5F, e, a);

ld_r(0x60, h, b);
ld_r(0x61, h, c);
ld_r(0x62, h, d);
ld_r(0x63, h, e);
ld_r(0x64, h, h);
ld_r(0x65, h, l);
ld_r_hl(0x66, h);
ld_r(0x67, h, a);

ld_r(0x68, l, b);
ld_r(0x69, l, c);
ld_r(0x6A, l, d);
ld_r(0x6B, l, e);
ld_r(0x6C, l, h);
ld_r(0x6D, l, l);
ld_r_hl(0x6E, l);
ld_r(0x6F, l, a);

ld_hl(0x70, b);
ld_hl(0x71, c);
ld_hl(0x72, d);
ld_hl(0x73, e);
ld_hl(0x74, h);
ld_hl(0x75, l);

op(0x76, 1, "halt", {
    cpu->state.mode = kGBProcessorModeHalted;
});

ld_hl(0x77, a);

ld_r(0x78, a, b);
ld_r(0x79, a, c);
ld_r(0x7A, a, d);
ld_r(0x7B, a, e);
ld_r(0x7C, a, h);
ld_r(0x7D, a, l);
ld_r_hl(0x7E, a);
ld_r(0x7F, a, a);

add(0x80, b);
add(0x81, c);
add(0x82, d);
add(0x83, e);
add(0x84, h);
add(0x85, l);

op_hl(0x86, "add a, (hl)", {
    __ALUAdd8(cpu, &cpu->state.a, &cpu->state.mdr, 0);
});

add(0x87, a);

adc(0x88, b);
adc(0x89, c);
adc(0x8A, d);
adc(0x8B, e);
adc(0x8C, h);
adc(0x8D, l);

op_hl(0x8E, "adc a, (hl)", {
    __ALUAdd8(cpu, &cpu->state.a, &cpu->state.mdr, cpu->state.f.c);
});

adc(0x8F, a);

sub(0x90, b);
sub(0x91, c);
sub(0x92, d);
sub(0x93, e);
sub(0x94, h);
sub(0x95, l);

op_hl(0x96, "sub a, (hl)", {
    __ALUSub8(cpu, (SInt8 *)&cpu->state.a, (SInt8 *)&cpu->state.mdr, 0);
});

sub(0x97, a);

sbc(0x98, b);
sbc(0x99, c);
sbc(0x9A, d);
sbc(0x9B, e);
sbc(0x9C, h);
sbc(0x9D, l);

op_hl(0x9E, "sbc a, (hl)", {
    __ALUSub8(cpu, (SInt8 *)&cpu->state.a, (SInt8 *)&cpu->state.mdr, cpu->state.f.c);
});

sbc(0x9F, a);

and(0xA0, b);
and(0xA1, c);
and(0xA2, d);
and(0xA3, e);
and(0xA4, h);
and(0xA5, l);

alu_op_hl(0xA6, "and", __ALUAnd8);

and(0xA7, a);

xor(0xA8, b);
xor(0xA9, c);
xor(0xAA, d);
xor(0xAB, e);
xor(0xAC, h);
xor(0xAD, l);

alu_op_hl(0xAE, "xor", __ALUXor8);

xor(0xAF, a);

or(0xB0, b);
or(0xB1, c);
or(0xB2, d);
or(0xB3, e);
or(0xB4, h);
or(0xB5, l);

alu_op_hl(0xB6, "or", __ALUOr8);

or(0xB7, a);

cmp(0xB8, b);
cmp(0xB9, c);
cmp(0xBA, d);
cmp(0xBB, e);
cmp(0xBC, h);
cmp(0xBD, l);

alu_op_hl(0xBE, "cp", __ALUCP8);

cmp(0xBF, a);

ret(0xC0, "nz", !cpu->state.f.z);

pop(0xC1, bc);
jmp(0xC2, "nz, ", !cpu->state.f.z);
jmp(0xC3, "", true);
call(0xC4, "nz, ", !cpu->state.f.z);

push(0xC5, bc);

op_arg(0xC6, "add a, " d8, {
    __ALUAdd8(cpu, &cpu->state.a, &cpu->state.mdr, 0);
});

rst(0xC7, 0x00);
ret(0xC8, "z", cpu->state.f.z);

op_wait2_stall(0xC9, 1, "ret", {
    __GBProcessorRead(cpu, cpu->state.sp + 0);
}, {
    cpu->state.pc = cpu->state.mdr;

    __GBProcessorRead(cpu, cpu->state.sp + 1);
}, {
    cpu->state.pc |= (cpu->state.mdr << 8);
    cpu->state.sp += 2;
});

jmp(0xCA, "z, ", cpu->state.f.z);

op(0xCB, 1, "cb.", { /* This is the prefix opcode */ });

call(0xCC, "z, ", cpu->state.f.z);
call(0xCD, "", true);

op_arg(0xCE, "adc a, " d8, {
    __ALUAdd8(cpu, &cpu->state.a, &cpu->state.mdr, cpu->state.f.c);
});

rst(0xCF, 0x08);
ret(0xD0, "nc", !cpu->state.f.c);
pop(0xD1, de);
jmp(0xD2, "nc, ", !cpu->state.f.c);
udef(0xD3);
call(0xD4, "nc, ", !cpu->state.f.c);
push(0xD5, de);

op_arg(0xD6, "sub " d8, {
    __ALUSub8(cpu, (SInt8 *)&cpu->state.a, (SInt8 *)&cpu->state.mdr, 0);
});

rst(0xD7, 0x10);
ret(0xD8, "c", cpu->state.f.c);

op_wait2_stall(0xD9, 1, "reti", {
    __GBProcessorRead(cpu, cpu->state.sp + 0);
}, {
    cpu->state.pc = cpu->state.mdr;

    __GBProcessorRead(cpu, cpu->state.sp + 1);
}, {
    cpu->state.pc |= (cpu->state.mdr << 8);
    cpu->state.sp += 2;

    cpu->state.ime = true;
});

jmp(0xDA, "c, ", cpu->state.f.c);
udef(0xDB);

call(0xDC, "c, ", cpu->state.f.c);
udef(0xDD);

op_arg(0xDE, "sbc " d8, {
    __ALUSub8(cpu, (SInt8 *)&cpu->state.a, (SInt8 *)&cpu->state.mdr, cpu->state.f.c);
});

rst(0xDF, 0x18);

op_wait2(0xE0, 2, "ld " d8 "(" dIO "), a", {
    __GBProcessorReadArgument(cpu);
}, {
    __GBProcessorWrite(cpu, 0xFF00 + cpu->state.mdr, cpu->state.a);
}, {
    // We're done
});

pop(0xE1, hl);

op_wait(0xE2, 1, "ld c(" dIO "), a", {
    __GBProcessorWrite(cpu, 0xFF00 + cpu->state.c, cpu->state.a);
}, {
    // We're done
});

udef(0xE3);
udef(0xE4);
push(0xE5, hl);

alu_op_arg(0xE6, "and", __ALUAnd8);

rst(0xE7, 0x20);

op_wait3_stall(0xE8, 1, "add sp, " d8, {
    __GBProcessorReadArgument(cpu);
}, {
    UInt16 v = __ALUSignExtend(cpu->state.mdr);

    __ALUAdd16(cpu, &cpu->state.sp, &v, 0);
}, {
    __GBProcessorRead(cpu, 0x0000);

    cpu->state.f.z = 0;
}, {
    // Stalling more...
});

op_simple(0xE9, "jp (hl)", {
    cpu->state.pc = cpu->state.hl;
});

op_wait3(0xEA, 3, "ld (" d16 "), a", {
    __GBProcessorReadArgument(cpu);
}, {
    cpu->state.data = cpu->state.mdr;

    __GBProcessorReadArgument(cpu);
}, {
    __GBProcessorWrite(cpu, cpu->state.data || (cpu->state.mdr << 8), cpu->state.a);
}, {
    // That's it
});

udef(0xEB);
udef(0xEC);
udef(0xED);

alu_op_arg(0xEE, "xor", __ALUXor8);

rst(0xEF, 0x28);

op_wait2(0xF0, 2, "ld a, " d8 "(" dIO ")", {
    __GBProcessorReadArgument(cpu);
}, {
    __GBProcessorRead(cpu, 0xFF00 + cpu->state.mdr);
}, {
    cpu->state.a = cpu->state.mdr;
});

op_wait2_stall(0xF1, 1, "pop af", {
    __GBProcessorRead(cpu, cpu->state.sp + 0);
}, {
    cpu->state.f.reg = cpu->state.mdr;

    __GBProcessorRead(cpu, cpu->state.sp + 1);
}, {
    cpu->state.a = cpu->state.mdr;
    cpu->state.sp += 2;
});

op_wait(0xF2, 1, "ld a, c(" dIO ")", {
    __GBProcessorRead(cpu, 0xFF00 + cpu->state.c);
}, {
    cpu->state.a = cpu->state.mdr;
});

op_simple(0xF3, "di", {
    printf("Interrupts disabled.\n");

    cpu->state.ime = false;
});

udef(0xF4);

op_wait2_stall(0xF5, 1, "push af", {
    __GBProcessorWrite(cpu, cpu->state.sp - 1, cpu->state.a);
}, {
    __GBProcessorWrite(cpu, cpu->state.sp - 2, cpu->state.f.reg);
}, {
    cpu->state.sp -= 2;
});

alu_op_arg(0xF6, "or", __ALUOr8);

rst(0xF7, 0x30);

op_wait2_stall(0xF8, 2, "ld hl, " d8 "(sp)", {
    __GBProcessorReadArgument(cpu);
}, {
    UInt16 v = __ALUSignExtend(cpu->state.mdr);
    UInt16 sp = cpu->state.sp;

    __ALUAdd16(cpu, &cpu->state.sp, &v, 0);

    cpu->state.hl = cpu->state.sp;
    cpu->state.sp = sp;
    cpu->state.f.z = 0;
}, {
    // That's it
});

op_stall(0xF9, 1, "ld sp, hl", {
    cpu->state.sp = cpu->state.hl;
});

op_wait3(0xFA, 3, "ld a, (" d16 ")", {
    __GBProcessorReadArgument(cpu);
}, {
    cpu->state.a = cpu->state.mdr;

    __GBProcessorReadArgument(cpu);
}, {
    __GBProcessorRead(cpu, cpu->state.a || (cpu->state.mdr << 8));
}, {
    cpu->state.a = cpu->state.mdr;
});

op_simple(0xFB, "ei", {
    printf("Interrupts enabled.\n");

    cpu->state.ime = true;
});

udef(0xFC);
udef(0xFD);

alu_op_arg(0xFE, "cp", __ALUCP8);

rst(0xFF, 0x38);

#pragma mark - Prefixed Instructions

#define rlc(code, reg)                                                      \
    op_pre_simple(code, "rlc " #reg, {                                      \
        __ALURotateLeft(&cpu->state.reg, 1);                                \
                                                                            \
        cpu->state.f.z = !cpu->state.reg;                                   \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.reg & 1;                                \
    })

#define rlc_hl(code)                                                        \
    op_pre_hl(code, "rlc (hl)", {                                           \
        __ALURotateLeft(&cpu->state.mdr, 1);                                \
                                                                            \
        cpu->state.f.z = !cpu->state.mdr;                                   \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.mdr & 1;                                \
    })

#define rrc(code, reg)                                                      \
    op_pre_simple(code, "rrc " #reg, {                                      \
        cpu->state.f.z = !cpu->state.reg;                                   \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.reg & 1;                                \
                                                                            \
        __ALURotateRight(&cpu->state.reg, 1);                               \
    })

#define rrc_hl(code)                                                        \
    op_pre_simple(code, "rrc (hl)", {                                       \
        cpu->state.f.z = !cpu->state.mdr;                                   \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.mdr & 1;                                \
                                                                            \
        __ALURotateRight(&cpu->state.mdr, 1);                               \
    })

#define rl(code, reg)                                                       \
    op_pre_simple(code, "rl " #reg, {                                       \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = cpu->state.f.c;                                    \
        cpu->state.f.c = cpu->state.reg >> 7;                               \
                                                                            \
        cpu->state.reg <<= 1;                                               \
        cpu->state.reg |= cpu->state.f.h;                                   \
        cpu->state.f.z = !cpu->state.reg;                                   \
        cpu->state.f.h = 0;                                                 \
    })

#define rl_hl(code)                                                         \
    op_pre_hl(code, "rl (hl)", {                                            \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = cpu->state.f.c;                                    \
        cpu->state.f.c = cpu->state.mdr >> 7;                               \
                                                                            \
        cpu->state.mdr <<= 1;                                               \
        cpu->state.mdr |= cpu->state.f.h;                                   \
        cpu->state.f.z = !cpu->state.mdr;                                   \
        cpu->state.f.h = 0;                                                 \
    })

#define rr(code, reg)                                                       \
    op_pre_simple(code, "rr " #reg, {                                       \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = cpu->state.f.c;                                    \
        cpu->state.f.c = cpu->state.reg & 1;                                \
                                                                            \
        cpu->state.reg >>= 1;                                               \
        cpu->state.reg |= cpu->state.f.h << 7;                              \
        cpu->state.f.z = !cpu->state.reg;                                   \
        cpu->state.f.h = 0;                                                 \
    })

#define rr_hl(code)                                                         \
    op_pre_hl(code, "rr (hl)", {                                            \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = cpu->state.f.c;                                    \
        cpu->state.f.c = cpu->state.mdr & 1;                                \
                                                                            \
        cpu->state.mdr >>= 1;                                               \
        cpu->state.mdr |= cpu->state.f.h << 7;                              \
        cpu->state.f.z = !cpu->state.mdr;                                   \
        cpu->state.f.h = 0;                                                 \
    })

#define sla(code, reg)                                                      \
    op_pre_simple(code, "sla " #reg, {                                      \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.reg >> 7;                               \
                                                                            \
        cpu->state.reg <<= 1;                                               \
        cpu->state.f.z = !cpu->state.reg;                                   \
    })

#define sla_hl(code)                                                        \
    op_pre_hl(code, "sla (hl)", {                                           \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.mdr >> 7;                               \
                                                                            \
        cpu->state.mdr <<= 1;                                               \
        cpu->state.f.z = !cpu->state.mdr;                                   \
    })

#define sra(code, reg)                                                      \
    op_pre_simple(code, "sra " #reg, {                                      \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.reg & 1;                                \
                                                                            \
        cpu->state.reg >>= 1;                                               \
        cpu->state.reg |= (cpu->state.reg >> 6);                            \
        cpu->state.f.z = !cpu->state.reg;                                   \
    })

#define sra_hl(code)                                                        \
    op_pre_hl(code, "sra (hl)", {                                           \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.mdr & 1;                                \
                                                                            \
        cpu->state.mdr >>= 1;                                               \
        cpu->state.mdr |= (cpu->state.mdr >> 6);                            \
        cpu->state.f.z = !cpu->state.mdr;                                   \
    })

#define swap(code, reg)                                                     \
    op_pre_simple(code, "swap " #reg, {                                     \
        cpu->state.reg = (cpu->state.reg >> 4) | (cpu->state.reg << 4);     \
                                                                            \
        cpu->state.f.z = !cpu->state.reg;                                   \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = 0;                                                 \
    })

#define swap_hl(code)                                                       \
    op_pre_simple(code, "swap (hl)", {                                      \
        cpu->state.mdr = (cpu->state.mdr >> 4) | (cpu->state.mdr << 4);     \
                                                                            \
        cpu->state.f.z = !cpu->state.mdr;                                   \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = 0;                                                 \
    })

#define srl(code, reg)                                                      \
    op_pre_simple(code, "srl " #reg, {                                      \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.reg & 1;                                \
                                                                            \
        cpu->state.reg >>= 1;                                               \
        cpu->state.f.z = !cpu->state.reg;                                   \
    })

#define srl_hl(code)                                                        \
    op_pre_hl(code, "srl (hl)", {                                           \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 0;                                                 \
        cpu->state.f.c = cpu->state.mdr & 1;                                \
                                                                            \
        cpu->state.mdr >>= 1;                                               \
        cpu->state.f.z = !cpu->state.mdr;                                   \
    })

#define bit(code, pos, reg)                                                 \
    op_pre_simple(code, "bit " #pos ", " #reg, {                            \
        cpu->state.f.z = !((cpu->state.reg >> pos) & 1);                    \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 1;                                                 \
    })

#define bit_hl(code, pos)                                                   \
    op_pre_hl(code, "bit " #pos ", (hl)", {                                 \
        cpu->state.f.z = !((cpu->state.mdr >> pos) & 1);                    \
        cpu->state.f.n = 0;                                                 \
        cpu->state.f.h = 1;                                                 \
    })

#define res(code, pos, reg)                                                 \
    op_pre_simple(code, "res " #pos ", " #reg, {                            \
        cpu->state.reg &= ~(1 << pos);                                      \
    })

#define res_hl(code, pos)                                                   \
    op_pre_hl(code, "res " #pos ", (hl)", {                                 \
        cpu->state.mdr &= ~(1 << pos);                                      \
    })

#define set(code, pos, reg)                                                 \
    op_pre_simple(code, "set " #pos ", " #reg, {                            \
        cpu->state.reg |= (1 << pos);                                       \
    })

#define set_hl(code, pos)                                                   \
    op_pre_hl(code, "set " #pos ", (hl)", {                                 \
        cpu->state.mdr |= (1 << pos);                                       \
    })

rlc(0x00, b); rlc(0x01, c);
rlc(0x02, d); rlc(0x03, e);
rlc(0x04, h); rlc(0x05, l);
rlc_hl(0x06); rlc(0x07, a);

rrc(0x08, b); rrc(0x09, c);
rrc(0x0A, d); rrc(0x0B, e);
rrc(0x0C, h); rrc(0x0D, l);
rrc_hl(0x0E); rrc(0x0F, a);

rl(0x10, b); rl(0x11, c);
rl(0x12, d); rl(0x13, e);
rl(0x14, h); rl(0x15, l);
rl_hl(0x16); rl(0x17, a);

rr(0x18, b); rr(0x19, c);
rr(0x1A, d); rr(0x1B, e);
rr(0x1C, h); rr(0x1D, l);
rr_hl(0x1E); rr(0x1F, a);

sla(0x20, b); sla(0x21, c);
sla(0x22, d); sla(0x23, e);
sla(0x24, h); sla(0x25, l);
sla_hl(0x26); sla(0x27, a);

sra(0x28, b); sra(0x29, c);
sra(0x2A, d); sra(0x2B, e);
sra(0x2C, h); sra(0x2D, l);
sra_hl(0x2E); sra(0x2F, a);

swap(0x30, b); swap(0x31, c);
swap(0x32, d); swap(0x33, e);
swap(0x34, h); swap(0x35, l);
swap_hl(0x36); swap(0x37, a);

srl(0x38, b); srl(0x39, c);
srl(0x3A, d); srl(0x3B, e);
srl(0x3C, h); srl(0x3D, l);
srl_hl(0x3E); srl(0x3F, a);

bit(0x40, 0, b); bit(0x41, 0, c); bit(0x42, 0, d); bit(0x43, 0, e); bit(0x44, 0, h); bit(0x45, 0, l); bit_hl(0x46, 0); bit(0x47, 0, a);
bit(0x48, 1, b); bit(0x49, 1, c); bit(0x4A, 1, d); bit(0x4B, 1, e); bit(0x4C, 1, h); bit(0x4D, 1, l); bit_hl(0x4E, 1); bit(0x4F, 1, a);
bit(0x50, 2, b); bit(0x51, 2, c); bit(0x52, 2, d); bit(0x53, 2, e); bit(0x54, 2, h); bit(0x55, 2, l); bit_hl(0x56, 2); bit(0x57, 2, a);
bit(0x58, 3, b); bit(0x59, 3, c); bit(0x5A, 3, d); bit(0x5B, 3, e); bit(0x5C, 3, h); bit(0x5D, 3, l); bit_hl(0x5E, 3); bit(0x5F, 3, a);
bit(0x60, 4, b); bit(0x61, 4, c); bit(0x62, 4, d); bit(0x63, 4, e); bit(0x64, 4, h); bit(0x65, 4, l); bit_hl(0x66, 4); bit(0x67, 4, a);
bit(0x68, 5, b); bit(0x69, 5, c); bit(0x6A, 5, d); bit(0x6B, 5, e); bit(0x6C, 5, h); bit(0x6D, 5, l); bit_hl(0x6E, 5); bit(0x6F, 5, a);
bit(0x70, 6, b); bit(0x71, 6, c); bit(0x72, 6, d); bit(0x73, 6, e); bit(0x74, 6, h); bit(0x75, 6, l); bit_hl(0x76, 6); bit(0x77, 6, a);
bit(0x78, 7, b); bit(0x79, 7, c); bit(0x7A, 7, d); bit(0x7B, 7, e); bit(0x7C, 7, h); bit(0x7D, 7, l); bit_hl(0x7E, 7); bit(0x7F, 7, a);

res(0x80, 0, b); res(0x81, 0, c); res(0x82, 0, d); res(0x83, 0, e); res(0x84, 0, h); res(0x85, 0, l); res_hl(0x86, 0); res(0x87, 0, a);
res(0x88, 1, b); res(0x89, 1, c); res(0x8A, 1, d); res(0x8B, 1, e); res(0x8C, 1, h); res(0x8D, 1, l); res_hl(0x8E, 1); res(0x8F, 1, a);
res(0x90, 2, b); res(0x91, 2, c); res(0x92, 2, d); res(0x93, 2, e); res(0x94, 2, h); res(0x95, 2, l); res_hl(0x96, 2); res(0x97, 2, a);
res(0x98, 3, b); res(0x99, 3, c); res(0x9A, 3, d); res(0x9B, 3, e); res(0x9C, 3, h); res(0x9D, 3, l); res_hl(0x9E, 3); res(0x9F, 3, a);
res(0xA0, 4, b); res(0xA1, 4, c); res(0xA2, 4, d); res(0xA3, 4, e); res(0xA4, 4, h); res(0xA5, 4, l); res_hl(0xA6, 4); res(0xA7, 4, a);
res(0xA8, 5, b); res(0xA9, 5, c); res(0xAA, 5, d); res(0xAB, 5, e); res(0xAC, 5, h); res(0xAD, 5, l); res_hl(0xAE, 5); res(0xAF, 5, a);
res(0xB0, 6, b); res(0xB1, 6, c); res(0xB2, 6, d); res(0xB3, 6, e); res(0xB4, 6, h); res(0xB5, 6, l); res_hl(0xB6, 6); res(0xB7, 6, a);
res(0xB8, 7, b); res(0xB9, 7, c); res(0xBA, 7, d); res(0xBB, 7, e); res(0xBC, 7, h); res(0xBD, 7, l); res_hl(0xBE, 7); res(0xBF, 7, a);

set(0xC0, 0, b); set(0xC1, 0, c); set(0xC2, 0, d); set(0xC3, 0, e); set(0xC4, 0, h); set(0xC5, 0, l); set_hl(0xC6, 0); set(0xC7, 0, a);
set(0xC8, 1, b); set(0xC9, 1, c); set(0xCA, 1, d); set(0xCB, 1, e); set(0xCC, 1, h); set(0xCD, 1, l); set_hl(0xCE, 1); set(0xCF, 1, a);
set(0xD0, 2, b); set(0xD1, 2, c); set(0xD2, 2, d); set(0xD3, 2, e); set(0xD4, 2, h); set(0xD5, 2, l); set_hl(0xD6, 2); set(0xD7, 2, a);
set(0xD8, 3, b); set(0xD9, 3, c); set(0xDA, 3, d); set(0xDB, 3, e); set(0xDC, 3, h); set(0xDD, 3, l); set_hl(0xDE, 3); set(0xDF, 3, a);
set(0xE0, 4, b); set(0xE1, 4, c); set(0xE2, 4, d); set(0xE3, 4, e); set(0xE4, 4, h); set(0xE5, 4, l); set_hl(0xE6, 4); set(0xE7, 4, a);
set(0xE8, 5, b); set(0xE9, 5, c); set(0xEA, 5, d); set(0xEB, 5, e); set(0xEC, 5, h); set(0xED, 5, l); set_hl(0xEE, 5); set(0xEF, 5, a);
set(0xF0, 6, b); set(0xF1, 6, c); set(0xF2, 6, d); set(0xF3, 6, e); set(0xF4, 6, h); set(0xF5, 6, l); set_hl(0xF6, 6); set(0xF7, 6, a);
set(0xF8, 7, b); set(0xF9, 7, c); set(0xFA, 7, d); set(0xFB, 7, e); set(0xFC, 7, h); set(0xFD, 7, l); set_hl(0xFE, 7); set(0xFF, 7, a);

#undef bit
#undef res
#undef set

#undef srl
#undef swap
#undef sra
#undef sla

#undef rr
#undef rl
#undef rrc
#undef rlc

#ifndef kGBDisassembler

#define expand(v, p)                                                                        \
    &op_ ## p ## v ## 0, &op_ ## p ## v ## 1, &op_ ## p ## v ## 2, &op_ ## p ## v ## 3,     \
    &op_ ## p ## v ## 4, &op_ ## p ## v ## 5, &op_ ## p ## v ## 6, &op_ ## p ## v ## 7,     \
    &op_ ## p ## v ## 8, &op_ ## p ## v ## 9, &op_ ## p ## v ## A, &op_ ## p ## v ## B,     \
    &op_ ## p ## v ## C, &op_ ## p ## v ## D, &op_ ## p ## v ## E, &op_ ## p ## v ## F

const static GBProcessorOP *const gGBInstructionSet[0x100] = {
    expand(0, 0x), expand(1, 0x), expand(2, 0x), expand(3, 0x),
    expand(4, 0x), expand(5, 0x), expand(6, 0x), expand(7, 0x),
    expand(8, 0x), expand(9, 0x), expand(A, 0x), expand(B, 0x),
    expand(C, 0x), expand(D, 0x), expand(E, 0x), expand(F, 0x)
};

static const GBProcessorOP *const gGBInstructionSetCB[0x100] = {
    expand(0x0, pre_),  expand(0x1, pre_), expand(0x2, pre_), expand(0x3, pre_),
    expand(0x4, pre_),  expand(0x5, pre_), expand(0x6, pre_), expand(0x7, pre_),
    expand(0x8, pre_),  expand(0x9, pre_), expand(0xA, pre_), expand(0xB, pre_),
    expand(0xC, pre_),  expand(0xD, pre_), expand(0xE, pre_), expand(0xF, pre_)
};

#undef expand

#undef format_

#undef op
#undef declare

#endif /* !defined(kGBDisassembler) */
