// We can make a disasembler really easily if we play with these macros right.
// To create a disassembler, just redefine the op macros such that the implementation is left empty
// and you have an effective map of opcode --> string and length.
// Then, disassembly is pretty easy, it's just a simple map
#include <stdint.h>

#ifndef kGBDisassembler

// Declare an opcode struct calling a function __GBProcessorDo_<op>
#define declare(op, str)                                                                        \
    static void __GBProcessorDo_ ## op(GBProcessorOP *this, struct __GBProcessor *cpu);         \
                                                                                                \
    const static GBProcessorOP op_## op = {                                                     \
        .name = str,                                                                            \
        .value = op,                                                                            \
        .go = __GBProcessorDo_ ## op                                                            \
    }

// Declare an opcode struct with the given implementation function.
// This macro is overridden by the disassembler, which will use the `len` field for decoding.
#define op(code, len, str, impl)                                                                \
    declare(code, str);                                                                         \
                                                                                                \
    void __GBProcessorDo_ ## code(GBProcessorOP *this, struct __GBProcessor *cpu)               \
    impl

// Similar to the above but for CB-prefixed instructions.
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

#pragma mark - State Macros

#define _data           ((cpu)->state.data)
#define _mar            ((cpu)->state.mar)
#define _mdr            ((cpu)->state.mdr)
#define _reg(r)         ((cpu)->state.r)
#define _pc             ((cpu)->state.pc)
#define _sp             ((cpu)->state.sp)
#define _hl             ((cpu)->state.hl)
#define _a              ((cpu)->state.a)
#define _f              (cpu)->state.f

#pragma mark - Memory Access

// Request a read from the given address. The mdr register will be filled on the next memory bus tick.
#define __mmu_read(cpu, addr)                                                                   \
    do {                                                                                        \
        (cpu)->state.accessed = false;                                                          \
        _mar = (addr);                                                                          \
                                                                                                \
        GBMemoryManagerReadRequest((cpu)->mmu, &_mar, &_mdr, &(cpu)->state.accessed);           \
    } while (0)

// Request a write to the given address. The data will be written on the next memory bus tick.
#define __mmu_write(cpu, addr, data)                                                            \
    do {                                                                                        \
        (cpu)->state.accessed = false;                                                          \
        _mar = (addr);                                                                          \
        _mdr = (data);                                                                          \
                                                                                                \
        GBMemoryManagerWriteRequest((cpu)->mmu, &_mar, &_mdr, &(cpu)->state.accessed);          \
    } while (0)

// Request a read for an argument for the current instruction.
#define __mmu_read_arg(cpu) { __mmu_read((cpu), _pc); _pc++; }

#pragma mark - ALU

// Set all flags at once
#define __set_flags(nz, nn, nh, nc)     \
    do {                                \
        _f.z = (nz) & 1;                \
        _f.n = (nn) & 1;                \
        _f.h = (nh) & 1;                \
        _f.c = (nc) & 1;                \
    } while (0)

// Compute the 1's complement of the passed value.
#define __alu_1_cmpl(a)     (~(a))

// Compute the 2's complement of the passed value.
#define __alu_2_cmpl(a)     (~(a) + 1)

// Sign extend the passed value from 8 to 16 bits
#define __alu_extend(a)     ({ uint8_t v = (uint8_t)(a); (uint16_t)((v & 0x80) ? (0xFF00 | v) : v); })

// Add two 8 bit numbers, updating flags as necessary.
#define __alu_add8(cpu, to, from, carry)                                            \
    do {                                                                            \
        /* Perform two 4-bit adds (this is how the gameboy actually does this) */   \
        uint8_t lo = (((from) & 0xF) + ((to) & 0xF) + carry);                       \
        _f.h = (lo >> 4) & 1;                                                       \
                                                                                    \
        uint8_t hi = (((from) >> 4) + ((to) >> 4) + cpu->state.f.h);                \
        _f.c = (hi >> 4) & 1;                                                       \
                                                                                    \
        to = (hi << 4) | (lo & 0xF);                                                \
        _f.z = !(to);                                                               \
        _f.n = 0;                                                                   \
    } while (0)

// Add a sign extended 8-bit value to a 16-bit base.
// The h and c flags are set based on the lower addition.
#define __alu_add16_8(cpu, to, from)                        \
    do {                                                    \
        uint16_t ext = __alu_extend(from);                  \
        uint8_t _lo = (to) & 0xFF;                          \
                                                            \
        __alu_add8((cpu), _lo, ext & 0xFF, 0);              \
        uint8_t h = _f.h;                                   \
        uint8_t c = _f.c;                                   \
                                                            \
        uint16_t _hi = (to) >> 8;                           \
        __alu_add8((cpu), _hi, ext >> 8, _f.c);             \
                                                            \
        (to) = (_hi << 8) | _lo;                            \
        __set_flags(0, 0, h, c);                            \
    } while (0)

// Add two 16 bit numbers, updating flags as necessary (haly carry and carry are set for bit 11 and 15)
// Note that the zero flag is not affected by 16 bit addition.
#define __alu_add16(cpu, to, from, carry)                   \
    do {                                                    \
        uint8_t _lo = (to) & 0xFF;                          \
        uint16_t _hi = (to) >> 8;                           \
        uint8_t z = _f.z;                                   \
                                                            \
        __alu_add8((cpu), _lo, (from) & 0xFF, carry);       \
        __alu_add8((cpu), _hi, (from) >> 8, _f.c);          \
                                                            \
        (to) = (_hi << 8) | _lo;                            \
        _f.z = z;                                           \
    } while (0)

// Subtract two 8 bit numbers, updating flags as necessary.
#define __alu_sub8(cpu, to, from, borrow)                   \
    do {                                                    \
        __alu_add8(cpu, to, __alu_2_cmpl(from), (borrow));  \
                                                            \
        /* Set subtract flag; flip carries to borrow */     \
        _f.n  = 1;                                          \
        _f.c ^= 1;                                          \
        _f.h ^= 1;                                          \
    } while (0)

// Logical AND two values, updating flags as necessary.
#define __alu_and8(cpu, to, from)           \
    do {                                    \
        (to) &= (from);                     \
        __set_flags( !(to), 0, 1, 0);       \
    } while (0)

// Logical XOR two values, updating flags as necessary.
#define __alu_xor8(cpu, to, from)           \
    do {                                    \
        (to) ^= (from);                     \
        __set_flags(!(to), 0, 0, 0);        \
    } while (0)

// Logical OR two values, updating flags as necessary.
#define __alu_or8(cpu, to, from)            \
    do {                                    \
        (to) |= (from);                     \
        __set_flags(!(to), 0, 0, 0);        \
    } while (0)

// Compare two values, updating flags as necessary
#define __alu_cmp(cpu, to, from)            \
    do {                                    \
        uint8_t tmp = (to);                 \
        __alu_sub8(cpu, tmp, from, 0);      \
    } while (0)

// Rotate the passed value left by one, updating flags as necessary
#define __alu_rol(v)                        \
    do {                                    \
        (v) = (((v) >> 7) | ((v) << 1));    \
        __set_flags(!(v), 0, 0, (v) & 1);   \
    } while (0)


// Rotate the passed value right by one, updating flags as necessary
#define __alu_ror(v)                        \
    do {                                    \
        __set_flags(!(v), 0, 0, (v) & 1);   \
        (v) = (((v) << 7) | ((v) >> 1));    \
    } while (0)

// Rotate the passed value left by one through carry, updating flags as necessary
#define __alu_rolc(v)                       \
    do {                                    \
        /* Use h as a tmp flag */           \
        _f.h = _f.c;                        \
        _f.c = (v) >> 7;                    \
                                            \
        (v) <<= 1;                          \
        (v) |= _f.h;                        \
                                            \
        __set_flags(!(v), 0, 0, _f.c);      \
    } while (0)

// Rotate the passed value right by one through carry, updating flags as necessary
#define __alu_rorc(v)                       \
    do {                                    \
        /* Use h as a tmp flag */           \
        _f.h = _f.c;                        \
        _f.c = (v) & 1;                     \
                                            \
        (v) >>= 1;                          \
        (v) |= _f.h << 7;                   \
                                            \
        __set_flags(!(v), 0, 0, _f.c);      \
    } while (0)

#endif /* !defined(kGBDisassembler) */

#pragma mark - Data Formatting Macros

// Instruction data formats
#ifndef kGBNumberFormatting
    #define d16 "$0x%04X"
    #define d8  "$0x%02X"
    #define dIO "0xFF00"
#endif /* !defined(kGBNumberFormatting) */

#pragma mark - Argument Wrappers

// Macros for setting/testing states
#define __modetest(m)   ((cpu)->state.mode == kGBProcessorMode ## m)
#define __modeset(m)    ((cpu)->state.mode  = kGBProcessorMode ## m)
#define __accessed()    ((cpu)->state.accessed)

// Simple argument taking a single cycle
#define op_simple(code, str, impl) op(code, 1, str, { impl; __modeset(Fetch); })

// Simple argument taking two cycles
#define op_stall(code, len, str, impl)                                  \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            impl;                                                       \
                                                                        \
            __mmu_read(cpu, 0x0000);                                    \
            __modeset(Stalled);                                         \
        } else if (__modetest(Stalled) && __accessed()) {               \
             __modeset(Fetch);                                          \
        }                                                               \
    })

// Instruction taking two cycles.
#define op_wait(code, len, str, pre, post)                              \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            pre;                                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            post;                                                       \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Instruction taking three cycles including a stall.
#define op_wait_stall(code, len, str, pre, post)                        \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            pre;                                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            post;                                                       \
                                                                        \
            __mmu_read(cpu, 0x0000);                                    \
            __modeset(Stalled);                                         \
        } else if (__modetest(Stalled) && __accessed()) {               \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Instruction taking three cycles waiting twice.
#define op_wait2(code, len, str, pre, mid, post)                        \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            pre;                                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            mid;                                                        \
            __modeset(Wait2);                                           \
        } else if (__modetest(Wait2) && __accessed()) {                 \
            post;                                                       \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Instruction taking 4 cycles including a stall
#define op_wait2_stall(code, len, str, pre, mid, post)                  \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            pre;                                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            mid;                                                        \
            __modeset(Wait2);                                           \
        } else if (__modetest(Wait2) && __accessed()) {                 \
            post;                                                       \
                                                                        \
            __mmu_read(cpu, 0x0000);                                    \
            __modeset(Stalled);                                         \
        } else if (__modetest(Stalled) && __accessed()) {               \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Instruction taking 4 cycles waiting three times.
#define op_wait3(code, len, str, pre, mid1, mid2, post)                 \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            pre;                                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                  \
            mid1;                                                       \
            __modeset(Wait2);                                           \
        } else if (__modetest(Wait2) && __accessed()) {                 \
            mid2;                                                       \
            __modeset(Wait3);                                           \
        } else if (__modetest(Wait3) && __accessed()) {                  \
            post;                                                       \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Instruction taking 5 cycles waiting three times and stalling.        
#define op_wait3_stall(code, len, str, pre, mid1, mid2, post)           \
    op(code, len, str, {                                                \
        if (__modetest(Run)) {                                          \
            pre;                                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            mid1;                                                       \
            __modeset(Wait2);                                           \
        } else if (__modetest(Wait2) && __accessed()) {                 \
            mid2;                                                       \
            __modeset(Wait3);                                           \
        } else if (__modetest(Wait3) && __accessed()) {                 \
            post;                                                       \
                                                                        \
            __mmu_read(cpu, 0x0000);                                    \
            __modeset(Stalled);                                         \
        } else if (__modetest(Stalled) && __accessed()) {               \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Instruction operating on memory at (hl)
#define op_hl(code, str, impl)          op_wait(code, 1, str, { __mmu_read(cpu, _hl); }, impl)

// Instruction operating on an 8-bit argument from (pc)
#define op_arg(code, str, impl)         op_wait(code, 2, str, { __mmu_read_arg(cpu); }, impl)

// Instruction operating on a 16-bit argument from (pc)
#define op_arg16(code, str, intr, impl) op_wait2(code, 3, str, { __mmu_read_arg(cpu); }, { intr; __mmu_read_arg(cpu); }, impl)

// Instruction taking 1 cycle with CB prefix
#define op_pre_simple(code, str, impl)  op_pre(code, 1, str, { impl; __modeset(Fetch); })

// Instruction with CB prefix operating on memory at (hl)
#define op_pre_hl(code, str, impl)                                      \
    op_pre(code, 1, str, {                                              \
        if (__modetest(Prefix)) {                                       \
            __mmu_read(cpu, _hl);                                       \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            impl;                                                       \
                                                                        \
            __mmu_write(cpu, _hl, _mdr);                                \
            __modeset(Wait2);                                           \
        } else if (__modetest(Wait2) && __accessed()) {                 \
            __modeset(Fetch);                                           \
        }                                                               \
    })

#pragma mark - Load Macros

// Load register from register
#define ld_r(code, dst, src)                                            \
    op_simple(code, "ld " #dst ", " #src, {                             \
        cpu->state.dst = cpu->state.src;                                \
    })


// Load register pair with 16-bit argument
#define ld_rr(code, regs)                                               \
    op_arg16(code, "ld " #regs ", " d16, {                              \
        _reg(regs) = _mdr;                                              \
    }, {                                                                \
        _reg(regs) |= _mdr << 8;                                        \
    })

// Load accumulator with memory address at register pair
#define ld_a_rr(code, regs)                                             \
    op_wait(code, 1, "ld a, (" #regs ")", {                             \
        __mmu_read(cpu, _reg(regs));                                    \
    }, { _a = _mdr; })

// Load memory address at register pair with accumulator value
#define ld_rr_a(code, regs)                                             \
    op_wait(code, 1, "ld (" #regs "), a", {                             \
        __mmu_write(cpu, _reg(regs), _a);                               \
    }, { /* nothing */ })

// Load register with 8-bit argument
#define ld_m(code, r) op_arg(code, "ld " #r ", " d8, { _reg(r) = _mdr; })

// Load memory at (hl) with value from register
#define ld_hl(code, r)                                                  \
    op_wait(code, 1, "ld (hl), " #r, {                                  \
        __mmu_write(cpu, _hl, _reg(r));                                 \
    }, { /* nothing */ })

// Load register with value of memory at (hl)
#define ld_r_hl(code, r) op_hl(code, "ld " #r ", (hl)", { _reg(r) = _mdr; })

#pragma mark - Control flow macros

// Conditional relative jump to sign extended 8-bit argument
#define jr(code, str, cond)                                             \
    op_wait_stall(code, 2, "jr " str d8, { __mmu_read_arg(cpu); }, {    \
        if (!cond) {                                                    \
            __modeset(Fetch);                                           \
        } else {                                                        \
            _pc += __alu_extend(_mdr);                                  \
        }                                                               \
    })

// Conditional absolute jump to 16-bit argument
#define jmp(code, str, cond)                                            \
    op_wait2_stall(code, 3, "jp " str d16, {                            \
        __mmu_read_arg(cpu);                                            \
    }, {                                                                \
        _data = _mdr;                                                   \
        __mmu_read_arg(cpu);                                            \
    }, {                                                                \
        if (!cond) {                                                    \
            __modeset(Fetch);                                           \
            return;                                                     \
        } else {                                                        \
            _pc = _data | (uint16_t)(_mdr << 8);                        \
        }                                                               \
    })

// Conditionally call subroutine at address, pushing return address to the stack
#define call(code, str, cond)                                           \
    op(code, 3, "call " str d16, {                                      \
        if (__modetest(Run)) {                                          \
            __mmu_read_arg(cpu);                                        \
            __modeset(Wait1);                                           \
        } else if (__modetest(Wait1) && __accessed()) {                 \
            _data = _mdr;                                               \
            __mmu_read_arg(cpu);                                        \
            __modeset(Wait2);                                           \
        } else if (__modetest(Wait2) && __accessed()) {                 \
            _data |= (_mdr << 8);                                       \
                                                                        \
            if (!cond) {                                                \
                __modeset(Fetch);                                       \
            } else {                                                    \
                __mmu_write(cpu, _sp - 1, _pc >> 8);                    \
                __modeset(Wait3);                                       \
            }                                                           \
        } else if (__modetest(Wait3) && __accessed()) {                 \
            __mmu_write(cpu, _sp - 2, _pc & 0xFF);                      \
            __modeset(Wait4);                                           \
        } else if (__modetest(Wait4) && __accessed()) {                 \
            __mmu_read(cpu, 0x0000);                                    \
            _pc = _data;                                                \
            _sp -= 2;                                                   \
                                                                        \
            __modeset(Stalled);                                         \
        } else if (__modetest(Stalled) && __accessed()) {               \
            __modeset(Fetch);                                           \
        }                                                               \
    })

// Return from subroutine to address on the top of stack
#define ret(code, str, cond)                                            \
    op_wait3_stall(code, 1, "ret " str, { __mmu_read(cpu, 0x0000); }, { \
        if (!cond) {                                                    \
            __modeset(Fetch);                                           \
        } else {                                                        \
            __mmu_read(cpu, _sp + 0);                                   \
        }                                                               \
    }, {                                                                \
        _pc = _mdr;                                                     \
        __mmu_read(cpu, _sp + 1);                                       \
    }, {                                                                \
        _pc |= (uint16_t)(_mdr << 8);                                   \
        _sp += 2;                                                       \
    })

// Jump to a predefined reset routine, saving return address to the top of the stack
#define rst(code, dst)                                                  \
    op_wait2_stall(code, 1, "rst $" #dst, {                             \
        __mmu_write(cpu, _sp - 1, _pc >> 8);                            \
    }, {                                                                \
        __mmu_write(cpu, _sp - 2, _pc & 0xFF);                          \
    }, {                                                                \
        _pc = dst;                                                      \
        _sp -= 2;                                                       \
    })

#pragma mark - Stack manipulation macros

// Pop a register pair from the top of the stack
#define pop(code, regs)                                                 \
    op_wait2_stall(code, 1, "pop " #regs, {                             \
        __mmu_read(cpu, _sp + 0);                                       \
        _sp++;                                                          \
    }, {                                                                \
        _reg(regs) = _mdr;                                              \
        __mmu_read(cpu, _sp + 1);                                       \
    }, {                                                                \
        _reg(regs) |= (uint16_t)(_mdr << 8);                            \
        _sp += 2;                                                       \
    })

// Push a register pair to the top of the stack
#define push(code, regs)                                                \
    op_wait2_stall(code, 1, "push " #regs, {                            \
        __mmu_write(cpu, _sp - 1, _reg(regs) >> 8);                     \
    }, {                                                                \
        __mmu_write(cpu, _sp - 2, _reg(regs) & 0xFF);                   \
    }, { _sp -= 2; })

#pragma mark - Undefined instruction macros

#define udef(code)                                                                      \
    op(code, 1, "ud " #code, {                                                          \
        fprintf(stderr, "Error: Undefined instruction 0x%02X called.\n", this->value);  \
        fprintf(stderr, "Note: A real gameboy would lockup here.\n");                   \
                                                                                        \
        __modeset(Off);                                                                 \
    })

#pragma mark - Logical operation macros

// Increment a given register by 1
#define inc_r(code, r)                                                  \
    op_simple(code, "inc " #r, {                                        \
        uint8_t carry = _f.c;                                           \
        __alu_add8(cpu, _reg(r), 1, 0);                                 \
        _f.c = carry;                                                   \
    })

// Decrement a given register by 1
#define dec_r(code, r)                                                  \
    op_simple(code, "dec " #r, {                                        \
        uint8_t carry = _f.c;                                           \
        __alu_sub8(cpu, _reg(r), 1, 0);                                 \
        _f.c = carry;                                                   \
    })

// Increment a register pair. Don't update flags.
#define inc_rr(code, regs) op_stall(code, 1, "inc " #regs, { _reg(regs)++; })

// Decrement a register pair. Don't update flags.
#define dec_rr(code, regs) op_stall(code, 1, "dec " #regs, { _reg(regs)--; })

// Add a register pair to hl. Don't update zero flag.
#define add_hl(code, regs)                                              \
    op_stall(code, 1, "add hl, " #regs, {                               \
        uint8_t zero = _f.z;                                            \
        __alu_add16(cpu, _hl, _reg(regs), 0);                           \
        _f.z = zero;                                                    \
    })

// Simple ALU operation on the accumulator with a register argument and carry
#define op_alu_carry(code, name, r, func, c)                            \
    op_simple(code, name " a, " #r, {                                   \
        func(cpu, _a, _reg(r), c);                                      \
    })

// Add register to accumulator (no carry)
#define add(code, reg)                  op_alu_carry(code, "add", reg, __alu_add8, 0)

// Add register to accumulator (with carry)
#define adc(code, reg)                  op_alu_carry(code, "adc", reg, __alu_add8, _f.c)

// Subtract register from accumulator (no carry)
#define sub(code, reg)                  op_alu_carry(code, "sub", reg, __alu_sub8, 0)

// Subtract register from accumulator (with carry)
#define sbc(code, reg)                  op_alu_carry(code, "sbc", reg, __alu_sub8, !_f.c)

// Simple ALU operation on accumulator (no carry)
#define op_alu(code, name, r, func)     op_simple(code, name " " #r, { func(cpu, _a, _reg(r)); })

// Simple ALU operation on memory at (hl)
#define op_alu_hl(code, name, func)     op_hl(code, name " (hl)", { func(cpu, _a, _mdr); })

// Simple ALU operation on literal argument
#define op_alu_arg(code, name, func)    op_arg(code, name " " d8, { func(cpu, _a, _mdr); })

// Logical AND instruction
#define and(code, reg)                  op_alu(code, "and", reg, __alu_and8)

// Logical XOR instruction
#define xor(code, reg)                  op_alu(code, "xor", reg, __alu_xor8)

// Logical OR instruction
#define or(code, reg)                   op_alu(code, "or", reg, __alu_or8)

// Comparison instruction
#define cmp(code, reg)                  op_alu(code, "cp", reg, __alu_cmp)

#pragma mark - Instruction Implementation

op_simple(0x00, "nop", { /* nop doesn't do anything, sorry */ });

ld_rr  (0x01, bc);  // ld bc, d16
ld_rr_a(0x02, bc);  // ld (bc), a
inc_rr (0x03, bc);  // inc bc
inc_r  (0x04, b);   // inc b
dec_r  (0x05, b);   // dec b
ld_m   (0x06, b);   // ld b, d8

// Rotate left accumulator (rlca)
op_simple(0x07, "rlca", {
    __alu_rol(_a);
    _f.z = 0;
});

// ld (d16), sp (save stack pointer at literal address)
op(0x08, 3, "ld (" d16 "), sp", {
    if (__modetest(Run)) {
        __mmu_read_arg(cpu);
        __modeset(Wait1);
    } else if (__modetest(Wait1) && __accessed()) {
        _data = _mdr;

        __mmu_read_arg(cpu);
        __modeset(Wait2);
    } else if (__modetest(Wait2) && __accessed()) {
        _mar = _data | (uint16_t)(_mdr << 8);

        __mmu_write(cpu, _mar, _sp & 0xFF);
        __modeset(Wait3);
    } else if (__modetest(Wait3) && __accessed()) {
        _mar++;

        __mmu_write(cpu, _mar, _sp >> 8);
        __modeset(Wait4);
    } else if (__modetest(Wait4) && __accessed()) {
        _sp |= ((uint16_t)_mar) << 8;
        __modeset(Fetch);
    }
});

add_hl (0x09, bc); // add hl, bc
ld_a_rr(0x0A, bc); // ld a, (bc)
dec_rr (0x0B, bc); // dec bc
inc_r  (0x0C, c);  // inc c
dec_r  (0x0D, c);  // dec c
ld_m   (0x0E, c);  // ld c, d8

// Rotate right accumulator (rrca)
op_simple(0x0F, "rrca", {
    __alu_ror(_a);
    _f.z = 0;
});

// stop
op(0x10, 2, "stop 0", {
    __modeset(Stopped);

    if (!__GBMemoryManagerRead(cpu->mmu, _pc + 1)) {
        _pc++;
    }
});

ld_rr  (0x11, de); // ld de, d16
ld_rr_a(0x12, de); // ld a, (de)
inc_rr (0x13, de); // inc de
inc_r  (0x14, d);  // inc d
dec_r  (0x15, d);  // dec d
ld_m   (0x16, d);  // ld d, d8

// Rotate left accumulator through carry (rla)
op_simple(0x17, "rla", {
    __alu_rolc(_a);
    _f.z = 0;
});

// Unconditional relative jump (jr s8)
jr(0x18, "", true);

add_hl (0x19, de); // add hl, de
ld_a_rr(0x1A, de); // ld a, (de)
dec_rr (0x1B, de); // dec de
inc_r  (0x1C, e);  // inc e
dec_r  (0x1D, e);  // dec e
ld_m   (0x1E, e);  // ld e, d8

// Rotate right accumulator through carry (rra)
op_simple(0x1F, "rra", {
    __alu_rorc(_a);
    _f.z = 0;
});

// Relative jump if non zero (jr nz, s8)
jr(0x20, "nz, ", !_f.z);

// ld hl, d16
ld_rr(0x21, hl);

// Write a to (hl) and increment hl
op_wait(0x22, 1, "ld (hl+), a", {
    __mmu_write(cpu, _hl, _a);
}, { _hl++; });

inc_rr(0x23, hl); // inc hl
inc_r (0x24, h);  // inc h
dec_r (0x25, h);  // dec h
ld_m  (0x26, h);  // ld h, d8

// Decimal adjust accumulator.
op_simple(0x27, "daa", {
    // Here, we use half carry and carry along with
    //   the subtraction (N) flag to determine how
    //   to adjust the accumulator value.
    if (_f.n) {
        if (_f.c) { _a -= 0x60; }
        if (_f.h) { _a -= 0x06; }
    } else {
        if (_f.c || _a > 0x99)
        {
            _a += 0x60;
            _f.c = 1;
        }

        if (_f.h || (_a & 0x0F) > 0x09) {
            _a += 0x06;
        }
    }

    // TODO: How is the carry flag affected by this operation?
    _f.z = !_a;
    _f.h = 0;
});

// Relative jump if zero (jr z, s8)
jr(0x28, "z, ", _f.z);

// Double hl pair (add hl, hl)
add_hl(0x29, hl);

// Load a from (hl) and increment hl
op_hl(0x2A, "ld a, (hl+)", {
    _a = _mdr;
    _hl++;
});

dec_rr(0x2B, hl); // dec hl
inc_r (0x2C, l);  // inc l
dec_r (0x2D, l);  // dec l
ld_m  (0x2E, l);  // ld l, d8

// Complement accumulator
op_simple(0x2F, "cpl", {
    _a = __alu_1_cmpl(_a);
    _f.h = 1;
    _f.n = 1;
});

// Relative jump if not carry (jr nc, s8)
jr(0x30, "nc, ", !_f.c);

// ld sp, d16
ld_rr(0x31, sp);

// Write a to (hl) and decrement hl
op_wait(0x32, 1, "ld (hl-), a", {
    __mmu_write(cpu, _hl, _a);
}, { _hl--; });

// inc sp
inc_rr(0x33, sp);

// Increment value at (hl)
op_wait2(0x34, 1, "inc (hl)", {
    __mmu_read(cpu, _hl);
}, {
    uint8_t carry = _f.c;
    __alu_add8(cpu, _mdr, 1, 0);
    _f.c = carry;

    __mmu_write(cpu, _hl, _mdr);
}, { /* Just stall here */ });

// Decrement value at (hl)
op_wait2(0x35, 1, "dec (hl)", {
    __mmu_read(cpu, _hl);
}, {
    uint8_t carry = _f.c;
    __alu_sub8(cpu, _mdr, 1, 0);
    _f.c = carry;

    __mmu_write(cpu, _hl, _mdr);
}, { /* Just stall here */ });

// Store literal value at (hl)
op_wait2(0x36, 2, "ld (hl), " d8, {
    __mmu_read_arg(cpu);
}, {
    __mmu_write(cpu, _hl, _mdr);
}, { /* Just stall here */ });

// Set carry flag
op_simple(0x37, "scf", {
    __set_flags(_f.z, 0, 0, 1);
});

// Relative jump if carry (jr c, s8)
jr(0x38, "c, ", _f.c);

// Add hl to sp (add hl, sp)
add_hl(0x39, sp);

// Load a from (hl) and decrement hl
op_hl(0x3A, "ld a, (hl-)", {
    _a = _mdr;
    _hl--;
});

dec_rr(0x3B, sp); // dec sp
inc_r (0x3C, a);  // inc a
dec_r (0x3D, a);  // dec a
ld_m  (0x3E, a);  // ld a, d8

// Clear carry flag
op_simple(0x3F, "ccf", {
    __set_flags(_f.z, 0, 0, !_f.c);
});

// ld b, <reg>
ld_r(0x40, b, b); ld_r(0x41, b, c); ld_r(0x42, b, d); ld_r(0x43, b, e);
ld_r(0x44, b, h); ld_r(0x45, b, l); ld_r_hl(0x46, b); ld_r(0x47, b, a);

// ld c, <reg>
ld_r(0x48, c, b); ld_r(0x49, c, c); ld_r(0x4A, c, d); ld_r(0x4B, c, e);
ld_r(0x4C, c, h); ld_r(0x4D, c, l); ld_r_hl(0x4E, c); ld_r(0x4F, c, a);

// ld d, <reg>
ld_r(0x50, d, b); ld_r(0x51, d, c); ld_r(0x52, d, d); ld_r(0x53, d, e);
ld_r(0x54, d, h); ld_r(0x55, d, l); ld_r_hl(0x56, d); ld_r(0x57, d, a);

// ld e, <reg>
ld_r(0x58, e, b); ld_r(0x59, e, c); ld_r(0x5A, e, d); ld_r(0x5B, e, e);
ld_r(0x5C, e, h); ld_r(0x5D, e, l); ld_r_hl(0x5E, e); ld_r(0x5F, e, a);

// ld h, <reg>
ld_r(0x60, h, b); ld_r(0x61, h, c); ld_r(0x62, h, d); ld_r(0x63, h, e);
ld_r(0x64, h, h); ld_r(0x65, h, l); ld_r_hl(0x66, h); ld_r(0x67, h, a);

// ld l, <reg>
ld_r(0x68, l, b); ld_r(0x69, l, c); ld_r(0x6A, l, d); ld_r(0x6B, l, e);
ld_r(0x6C, l, h); ld_r(0x6D, l, l); ld_r_hl(0x6E, l); ld_r(0x6F, l, a);

// ld (hl, <reg>
ld_hl(0x70, b); ld_hl(0x71, c); ld_hl(0x72, d); ld_hl(0x73, e);
ld_hl(0x74, h); ld_hl(0x75, l);

op(0x76, 1, "halt", {
    __modeset(Halted);

    // TODO: There is a hardware bug in this instruction that
    //   is meant to stall once if IME is not set and an interrupt is pending.
    cpu->state.enableIME = cpu->state.ime;
    cpu->state.ime = true;
});

ld_hl(0x77, a);

// ld a, <reg>
ld_r(0x78, a, b); ld_r(0x79, a, c); ld_r(0x7A, a, d); ld_r(0x7B, a, e);
ld_r(0x7C, a, h); ld_r(0x7D, a, l); ld_r_hl(0x7E, a); ld_r(0x7F, a, a);

// add a, <reg>
add(0x80, b); add(0x81, c); add(0x82, d); add(0x83, e);
add(0x84, h); add(0x85, l); /*   0x86  */ add(0x87, a);
op_hl(0x86, "add a, (hl)", { __alu_add8(cpu, _a, _mdr, 0); });

// adc a, <reg>
adc(0x88, b); adc(0x89, c); adc(0x8A, d); adc(0x8B, e);
adc(0x8C, h); adc(0x8D, l); /*   0x8E  */ adc(0x8F, a);
op_hl(0x8E, "adc a, (hl)", { __alu_add8(cpu, _a, _mdr, _f.c); });

// sub a, <reg>
sub(0x90, b); sub(0x91, c); sub(0x92, d); sub(0x93, e);
sub(0x94, h); sub(0x95, l); /*   0x96  */ sub(0x97, a);
op_hl(0x96, "sub a, (hl)", { __alu_sub8(cpu, _a, _mdr, 0); });

// sbc a, <reg>
sbc(0x98, b); sbc(0x99, c); sbc(0x9A, d); sbc(0x9B, e);
sbc(0x9C, h); sbc(0x9D, l); /*   0x9E  */ sbc(0x9F, a);
op_hl(0x9E, "sbc a, (hl)", { __alu_sub8(cpu, _a, _mdr, !_f.c); });

// and <reg>
and(0xA0, b); and(0xA1, c); and(0xA2, d); and(0xA3, e);
and(0xA4, h); and(0xA5, l); /*   0xA6  */ and(0xA7, a);
op_alu_hl(0xA6, "and", __alu_and8);

// xor <reg>
xor(0xA8, b); xor(0xA9, c); xor(0xAA, d); xor(0xAB, e);
xor(0xAC, h); xor(0xAD, l); /*   0xAE  */ xor(0xAF, a);
op_alu_hl(0xAE, "xor", __alu_xor8);

// or <reg>
or(0xB0, b); or(0xB1, b); or(0xB2, b); or(0xB3, b);
or(0xB4, b); or(0xB5, b); /*  0xB6  */ or(0xB7, b);
op_alu_hl(0xB6, "or", __alu_or8);

// cp <reg>
cmp(0xB8, b); cmp(0xB9, c); cmp(0xBA, d); cmp(0xBB, e);
cmp(0xBC, h); cmp(0xBD, l); /*   0xBE  */ cmp(0xBF, a);
op_alu_hl(0xBE, "cp", __alu_cmp);

// Return if not zero (ret nz)
ret(0xC0, "nz", !_f.z);

// pop bc
pop(0xC1, bc);

// Absolute jump if non zero (jp nz, a16)
jmp(0xC2, "nz, ", !_f.z);

// Unconditional absolute jump (jp a16)
jmp(0xC3, "", true);

// Call subroutine if non zero (call nz, a16)
call(0xC4, "nz, ", !_f.z);

// push bc
push(0xC5, bc);

// Add 8-bit constant
op_arg(0xC6, "add a, " d8, { __alu_add8(cpu, _a, _mdr, 0); });

// Reset to vector 0x00 (rst 0x00)
rst(0xC7, 0x00);

// Return from subroutine if zero (ret z)
ret(0xC8, "z", _f.z);

// Unconditional return from subroutine
op_wait2_stall(0xC9, 1, "ret", { __mmu_read(cpu, _sp); }, {
    _pc = _mdr;
    __mmu_read(cpu, _sp + 1);
}, {
    _pc |= (uint16_t)(_mdr << 8);
    _sp += 2;
});

// Absolute jump if zero (jp z, a16)
jmp(0xCA, "z, ", _f.z);

op(0xCB, 1, "cb.", { /* This is the prefix opcode */ });

// Call subroutine if zero (call z, a16)
call(0xCC, "z, ", _f.z);

// Unconditional call subroutine (call a16)
call(0xCD, "", true);

// Add 8-bit constant with carry
op_arg(0xCE, "adc a, " d8, { __alu_add8(cpu, _a, _mdr, _f.c); });

// Reset to vector 0x08 (rst 0x08)
rst(0xCF, 0x08);

// Return from subroutine if not carry (ret nc)
ret(0xD0, "nc", !_f.c);

// pop de
pop(0xD1, de);

// Absolute jump if not carry (jp nc, a16)
jmp(0xD2, "nc, ", !_f.c);

// Undefined 0xD3
udef(0xD3);

// Call subroutine if not carry (call nc, a16)
call(0xD4, "nc, ", !_f.c);

// push de
push(0xD5, de);

// Subtract 8-bit constant
op_arg(0xD6, "sub " d8, { __alu_sub8(cpu, _a, _mdr, 0); });

// Reset to vector 0x10 (rst 0x10)
rst(0xD7, 0x10);

// Return from subroutine if carry (ret c)
ret(0xD8, "c", _f.c);

// Return from subroutine and enable interrupts
op_wait2_stall(0xD9, 1, "reti", {
    cpu->state.enableIME = true;
    __mmu_read(cpu, _sp);
}, {
    _pc = _mdr;
    __mmu_read(cpu, _sp + 1);
}, {
    _pc |= (uint16_t)(_mdr << 8);
    _sp += 2;
});

// Absolute jump if carry (jp c, a16)
jmp(0xDA, "c, ", _f.c);

// Undefined 0xDB
udef(0xDB);

// Call subroutine if carry (call c, a16)
call(0xDC, "c, ", _f.c);

// Undefined 0xDD
udef(0xDD);

// Subtract 8-bit constant
op_arg(0xDE, "sbc " d8, { __alu_sub8(cpu, _a, _mdr, !_f.c); });

// Reset to vector 0x18 (rst 0x18)
rst(0xDF, 0x18);

// Store accumulator to high address (0xFF00 | immediate 8-bit value)
op_wait2(0xE0, 2, "ld " d8 "(" dIO "), a", { __mmu_read_arg(cpu); }, {
    __mmu_write(cpu, 0xFF00 | _mdr, _a);
}, { /* stall */ });

// pop hl
pop(0xE1, hl);

// Store accumulator to high address (0xFF | C register)
op_wait(0xE2, 1, "ld c(" dIO "), a", {
    __mmu_write(cpu, 0xFF00 | cpu->state.c, _a);
}, { /* stall */ });

// Undefined 0xE3
udef(0xE3);

// Undefined 0xE4
udef(0xE4);

// push hl
push(0xE5, hl);

// Logical AND 8-bit constant
op_alu_arg(0xE6, "and", __alu_and8);

// Reset to vector 0xE7 (rst 0x20)
rst(0xE7, 0x20);

// Add a signed 8-bit offset to the stack pointer.
// Effectively, grow or shrink the stack by -127 to 128 bytes.
op_wait3_stall(0xE8, 1, "add sp, " d8, { __mmu_read_arg(cpu); }, {
    __alu_add16_8(cpu, _sp, _mdr);
}, { __mmu_read(cpu, 0x0000); }, { /* stall */ });

// Unconditionally jump to the value in hl
op_simple(0xE9, "jp hl", { _pc = _hl; });

// Store the value of the accumulator into memory at a 16-bit constant address
op_wait3(0xEA, 3, "ld (" d16 "), a", { __mmu_read_arg(cpu); }, {
    _data = _mdr;
    __mmu_read_arg(cpu);
}, {
    uint16_t addr = (_mdr << 8) | _data;
    __mmu_write(cpu, addr, _a);
}, { /* stall */ });

// Undefined 0xEB, 0xEC, 0xED
udef(0xEB);
udef(0xEC);
udef(0xED);

// Logical XOR with 8-bit constant
op_alu_arg(0xEE, "xor", __alu_xor8);

// Reset to vector 0x28 (rst 0x28)
rst(0xEF, 0x28);

// Load accumulator from high address (0xFF00 | immidiate 8-bit value)
op_wait2(0xF0, 2, "ld a, " d8 "(" dIO ")", { __mmu_read_arg(cpu); }, {
    __mmu_read(cpu, 0xFF00 | _mdr);
}, { _a = _mdr; });

// Pop accumulator and flags from the stack
op_wait2_stall(0xF1, 1, "pop af", {
    __mmu_read(cpu, _sp);
}, {
    _f.reg = _mdr & 0xF0;
    __mmu_read(cpu, _sp + 1);
}, {
    _a = _mdr;
    _sp += 2;
});

// Load accumulator from high address (0xFF00 | C register)
op_wait(0xF2, 1, "ld a, c(" dIO ")", {
    __mmu_read(cpu, 0xFF00 | cpu->state.c);
}, { _a = _mdr; });

// Disable interrupts
op_simple(0xF3, "di", {
    //printf("Interrupts disabled.\n");
    cpu->state.ime = false;
});

// Undefined 0xF4
udef(0xF4);

// Push accumulator and flags to the stack
op_wait2_stall(0xF5, 1, "push af", {
    __mmu_write(cpu, _sp - 1, _a);
}, {
    __mmu_write(cpu, _sp - 2, _f.reg & 0xF0);
}, { _sp -= 2; });

// Logical OR with 8-bit constant
op_alu_arg(0xF6, "or", __alu_or8);

// Reset to vector 0x30 (rst 0x30)
rst(0xF7, 0x30);

// Add a signed 8-bit offset to the stack pointer.
// Store the result in hl.
op_wait2_stall(0xF8, 2, "ld hl, " d8 "(sp)", { __mmu_read_arg(cpu); }, {
    _hl = _sp;
    __alu_add16_8(cpu, _hl, _mdr);
}, { /* stall */ });

// Set stack pointer from hl
op_stall(0xF9, 1, "ld sp, hl", { _sp = _hl; });

// Load the value of the accumulator from memory at a 16-bit constant address
op_wait3(0xFA, 3, "ld a, (" d16 ")", { __mmu_read_arg(cpu); }, {
    _data = _mdr;
    __mmu_read_arg(cpu);
}, {
    uint16_t addr = (_mdr << 8) | _data;
    __mmu_read(cpu, addr);
}, { _a = _mdr; });

// Enable interrupts
op_simple(0xFB, "ei", {
    //printf("Interrupts enabled.\n");
    cpu->state.enableIME = true;
});

// Undefined 0xFC, 0xFD
udef(0xFC);
udef(0xFD);

// Compare with 8-bit constant
op_alu_arg(0xFE, "cp", __alu_cmp);

// Reset to vector 0x38 (rst 0x38)
rst(0xFF, 0x38);

#pragma mark - Prefixed Instructions

// Rotate register left
#define rlc(code, r) op_pre_simple(code, "rlc " #r, { __alu_rol(_reg(r)); })

// Rotate value at (hl) left
#define rlc_hl(code) op_pre_hl(code, "rlc (hl)", { __alu_rol(_mdr); })

// Rotate register right
#define rrc(code, r) op_pre_simple(code, "rrc " #r, { __alu_ror(_reg(r)); })

// Rotate value at (hl) right
#define rrc_hl(code) op_pre_hl(code, "rrc (hl)", { __alu_ror(_mdr); })

// Rotate register left through carry
#define rl(code, r) op_pre_simple(code, "rl " #r, { __alu_rolc(_reg(r)); })

// Rotate value at (hl) left through carry
#define rl_hl(code) op_pre_hl(code, "rl (hl)", { __alu_rolc(_mdr); })

// Rotate register right through carry
#define rr(code, r) op_pre_simple(code, "rr " #r, { __alu_rorc(_reg(r)); })

// Rotate value at (hl) right through carry
#define rr_hl(code) op_pre_hl(code, "rr (hl)", { __alu_rorc(_mdr); })

// Shift register left
#define sla(code, r)                                        \
    op_pre_simple(code, "sla " #r, {                        \
        __set_flags(!(_reg(r) & 0x7F), 0, 0, _reg(r) >> 7); \
        _reg(r) <<= 1;                                      \
    })

// Shift left value at (hl)
#define sla_hl(code)                                        \
    op_pre_hl(code, "sla (hl)", {                           \
        __set_flags(!(_mdr & 0x7F), 0, 0, _mdr >> 7);       \
        _mdr <<= 1;                                         \
    })

// Shift right register, preserving the high bit.
#define sra(code, r)                                        \
    op_pre_simple(code, "sra " #r, {                        \
        __set_flags(!(_reg(r) & 0xFE), 0, 0, _reg(r) & 1);  \
                                                            \
        _reg(r) >>= 1;                                      \
        _reg(r) |= (_reg(r) & 0x40) << 1;                   \
    })

// Shift right value at (hl), preserving the high bit.
#define sra_hl(code)                                        \
    op_pre_hl(code, "sra (hl)", {                           \
        __set_flags(!(_mdr & 0xFE), 0, 0, _mdr & 1);        \
                                                            \
        _mdr >>= 1;                                         \
        _mdr |= (_mdr & 0x40) << 1;                         \
    })

// Swap high and low 4 bits of register
#define swap(code, r)                                       \
    op_pre_simple(code, "swap " #r, {                       \
        _reg(r) = ((_reg(r)) >> 4) | ((_reg(r)) << 4);      \
        __set_flags(!_reg(r), 0, 0, 0);                     \
    })

// Swap high and low 4 bits of value at (hl)
#define swap_hl(code)                                       \
    op_pre_hl(code, "swap (hl)", {                          \
        _mdr = ((_mdr) >> 4) | ((_mdr) << 4);               \
        __set_flags(!_mdr, 0, 0, 0);                        \
    })

// Logical shift register right
#define srl(code, r)                                        \
    op_pre_simple(code, "srl " #r, {                        \
        __set_flags(!(_reg(r) & 0xFE), 0, 0, _reg(r) & 1);  \
        _reg(r) >>= 1;                                      \
    })

// Logical shift value at (hl) right
#define srl_hl(code)                                        \
    op_pre_hl(code, "srl (hl)", {                           \
        __set_flags(!(_mdr & 0xFE), 0, 0, _mdr & 1);        \
        _mdr >>= 1;                                         \
    })

// Test if the bit as `pos` in a register is set or not, updating Z flag.
#define bit(code, pos, r)                                   \
    op_pre_simple(code, "bit " #pos ", " #r, {              \
        __set_flags(!((_reg(r) >> pos) & 1), 0, 1, _f.c);   \
    })

// Test if the bit as `pos` of value at (hl) is set or not, updating Z flag.
#define bit_hl(code, pos)                                   \
    op_pre_hl(code, "bit " #pos ", (hl)", {                 \
        __set_flags(!((_mdr >> pos) & 1), 0, 1, _f.c);      \
    })

// Unset a bit at `pos` in a register
#define res(code, pos, r) op_pre_simple(code, "res " #pos ", " #r, { _reg(r) &= ~(1 << pos); })

// Unset a bit at `pos` of value at (hl)
#define res_hl(code, pos)   op_pre_hl(code, "res " #pos ", (hl)", { cpu->state.mdr &= ~(1 << pos); })

// Set a bit at `pos` in a register
#define set(code, pos, r) op_pre_simple(code, "set " #pos ", " #r, { _reg(r) |= (1 << pos); })

// Set a bit at `pos` of value at (hl)
#define set_hl(code, pos)   op_pre_hl(code, "set " #pos ", (hl)", { cpu->state.mdr |= (1 << pos); })

rlc(0x00, b); rlc(0x01, c); rlc(0x02, d); rlc(0x03, e);
rlc(0x04, h); rlc(0x05, l); rlc_hl(0x06); rlc(0x07, a);

rrc(0x08, b); rrc(0x09, c); rrc(0x0A, d); rrc(0x0B, e);
rrc(0x0C, h); rrc(0x0D, l); rrc_hl(0x0E); rrc(0x0F, a);

rl(0x10, b); rl(0x11, c); rl(0x12, d); rl(0x13, e);
rl(0x14, h); rl(0x15, l); rl_hl(0x16); rl(0x17, a);

rr(0x18, b); rr(0x19, c); rr(0x1A, d); rr(0x1B, e);
rr(0x1C, h); rr(0x1D, l); rr_hl(0x1E); rr(0x1F, a);

sla(0x20, b); sla(0x21, c); sla(0x22, d); sla(0x23, e);
sla(0x24, h); sla(0x25, l); sla_hl(0x26); sla(0x27, a);

sra(0x28, b); sra(0x29, c); sra(0x2A, d); sra(0x2B, e);
sra(0x2C, h); sra(0x2D, l); sra_hl(0x2E); sra(0x2F, a);

swap(0x30, b); swap(0x31, c); swap(0x32, d); swap(0x33, e);
swap(0x34, h); swap(0x35, l); swap_hl(0x36); swap(0x37, a);

srl(0x38, b); srl(0x39, c); srl(0x3A, d); srl(0x3B, e);
srl(0x3C, h); srl(0x3D, l); srl_hl(0x3E); srl(0x3F, a);

#define _pre_bit_group(of, from, pos)                   \
    of((from + 0), pos, b);     of((from + 1), pos, c); \
    of((from + 2), pos, d);     of((from + 3), pos, e); \
    of((from + 4), pos, h);     of((from + 5), pos, l); \
    of ## _hl((from + 6), pos); of((from + 7), pos, a)

#define _pre_bit_byte(of, from)                                             \
    _pre_bit_group(of, (from +  0), 0); _pre_bit_group(of, (from +  8), 1); \
    _pre_bit_group(of, (from + 16), 2); _pre_bit_group(of, (from + 12), 3); \
    _pre_bit_group(of, (from + 32), 4); _pre_bit_group(of, (from + 40), 5); \
    _pre_bit_group(of, (from + 48), 6); _pre_bit_group(of, (from + 54), 7);

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
