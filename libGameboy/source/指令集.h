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

static __attribute__((always_inline)) void __GBProcessorRead16(GBProcessor *cpu)
{
    UInt16 data = GBMemoryManagerRead(cpu->mmu, cpu->state.mar);
    data |= (GBMemoryManagerRead(cpu->mmu, cpu->state.mar + 1) << 8);

    cpu->state.mdr = data;
    cpu->state.accessed = true;
}

static __attribute__((always_inline)) void __GBProcessorWrite16(GBProcessor *cpu)
{
    GBMemoryManagerWrite(cpu->mmu, cpu->state.mar + 0, cpu->state.mdr & 0xFF);
    GBMemoryManagerWrite(cpu->mmu, cpu->state.mar + 1, cpu->state.mdr >> 8);

    cpu->state.accessed = true;
}

static __attribute__((always_inline)) void __GBProcessorRead8(GBProcessor *cpu)
{
    cpu->state.mdr = GBMemoryManagerRead(cpu->mmu, cpu->state.mar);
    cpu->state.accessed = true;
}

static __attribute__((always_inline)) void __GBProcessorWrite8(GBProcessor *cpu)
{
    GBMemoryManagerWrite(cpu->mmu, cpu->state.mar, cpu->state.mdr);
    cpu->state.accessed = true;
}

static __attribute__((always_inline)) void __GBProcessorSetupAccess(GBProcessor *cpu, UInt16 address)
{
    cpu->state.accessed = false;
    cpu->state.mar = address;
}

static __attribute__((always_inline)) void __GBProcessorStall(GBProcessor *cpu, UInt8 memCycles)
{
    // TODO: this.
}

#pragma mark - ALU

static __attribute__((always_inline)) void __ALUAdd8(GBProcessor *cpu, UInt8 *to, UInt8 *from, UInt8 carry)
{
    // Perform two 4-bit adds (this is how the gameboy actually does this)
    UInt8 lo = ((*from) & 0xF) + ((*to) & 0xF) + carry;
    cpu->state.f.bit.h = lo >> 4;

    UInt8 hi = ((*from) >> 4) + ((*to) >> 4) + cpu->state.f.bit.h;
    cpu->state.f.bit.c = hi >> 4;

    (*to) = (hi << 4) | (lo & 0xF);

    cpu->state.f.bit.z = !(*to);
    cpu->state.f.bit.n = 0;
}

static __attribute__((always_inline)) void __ALUAdd16(GBProcessor *cpu, UInt16 *to, UInt16 *from, UInt8 carry)
{
    // Perform two 8-bit adds (four 4-bit adds). Flags should automatically be right.
    __ALUAdd8(cpu, (UInt8 *)(to) + 0, (UInt8 *)(from) + 0, carry);
    __ALUAdd8(cpu, (UInt8 *)(to) + 1, (UInt8 *)(from) + 1, cpu->state.f.bit.c);

    // 16 bit ALU operations stall for a single memory access
    __GBProcessorStall(cpu, 1);
}

static __attribute__((always_inline)) void __ALUSub8(GBProcessor *cpu, SInt8 *to, SInt8 *from, UInt8 carry)
{
    // I forgot how 2's complemnet works so I did this instead
    SInt8 result = (*to) - (*from) + carry;

    cpu->state.f.bit.z = !result;
    cpu->state.f.bit.n = 1;
    cpu->state.f.bit.h = (((*to) & 0x0F) < ((*from) & 0x0F));
    cpu->state.f.bit.c = (result < 0);

    (*to) = result;
}

static __attribute__((always_inline)) void __ALUSub16(GBProcessor *cpu, SInt16 *to, SInt16 *from, UInt8 carry)
{
    __ALUSub8(cpu, (SInt8 *)(to) + 0, (SInt8 *)(from) + 0, carry);
    __ALUSub8(cpu, (SInt8 *)(to) + 1, (SInt8 *)(from) + 1, cpu->state.f.bit.c);

    // 16 bit ALU operations stall for a single memory access
    __GBProcessorStall(cpu, 1);
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

#pragma mark - Instruction Implementation

op(0x00, 1, "nop", { /* nop doesn't do anything, sorry */ });

op(0x01, 3, "ld bc, $0x%04X", {
    __GBProcessorSetupAccess(cpu, cpu->state.pc);
    __GBProcessorRead16(cpu);

    cpu->state.pc += 2;
    cpu->state.bc.val = cpu->state.mdr;
});

op(0x02, 1, "ld (bc), a", {
    __GBProcessorSetupAccess(cpu, cpu->state.bc.val);
    cpu->state.mdr = cpu->state.a;

    __GBProcessorWrite8(cpu);
});

op(0x03, 1, "inc bc", {
    __GBProcessorStall(cpu, 1);

    cpu->state.bc.val++;
});

op(0x04, 1, "inc b", {
    UInt8 carry = cpu->state.f.bit.c;
    UInt8 one = 1;

    __ALUAdd8(cpu, &cpu->state.bc.b, &one, 0);
    cpu->state.f.bit.c = carry;
});

op(0x05, 1, "dec b", {
    UInt8 carry = cpu->state.f.bit.c;
    SInt8 one = 1;

    __ALUSub8(cpu, (SInt8 *)&cpu->state.bc.b, &one, 0);
    cpu->state.f.bit.c = carry;
});

op(0x06, 2, "ld b, $0x%02X", {
    __GBProcessorSetupAccess(cpu, cpu->state.pc);
    __GBProcessorRead8(cpu);

    cpu->state.pc++;
    cpu->state.bc.b = cpu->state.mdr;
});

op(0x07, 1, "rcla", {
    __ALURotateLeft(&cpu->state.a, 1);

    cpu->state.f.bit.z = 0;
    cpu->state.f.bit.n = 0;
    cpu->state.f.bit.h = 0;
    cpu->state.f.bit.c = cpu->state.a & 1;
});

op(0x08, 3, "ld ($0x%04X), sp", {
    //
});

op(0x09, 1, "add hl, bc", {
    //
});

op(0x0A, 1, "ld a, (bc)", {
    //
});

op(0x0B, 1, "dec bc", {
    //
});

op(0x0C, 1, "inc c", {
    //
});

op(0x0D, 1, "dec c", {
    //
});

op(0x0E, 2, "ld c, $0x%02X", {
    //
});

op(0x0F, 1, "rrca", {
    //
});

op(0x10, 2, "stop 0", {
    //
});

op(0x11, 3, "ld de, $0x%04X", {
    //
});

op(0x12, 1, "ld (de), a", {
    //
});

op(0x13, 1, "inc de", {
    //
});

op(0x14, 1, "inc d", {
    //
});

op(0x15, 1, "dec d", {
    //
});

op(0x16, 2, "ld d, $0x%02X", {
    //
});

op(0x17, 1, "rla", {
    //
});

op(0x18, 2, "jr $0x%02X", {
    //
});

op(0x19, 1, "add hl, de", {
    //
});

op(0x1A, 1, "ld a, (de)", {
    //
});

op(0x1B, 1, "dec de", {
    //
});

op(0x1C, 1, "inc e", {
    //
});

op(0x1D, 1, "dec e", {
    //
});

op(0x1E, 2, "ld e, $0x%02X", {
    //
});

op(0x1F, 1, "rra", {
    //
});

op(0x20, 2, "jr nz, $0x%02X", {
    //
});

op(0x21, 3, "ld hl, $0x%04X", {
    //
});

op(0x22, 1, "ld (hl+), a", {
    //
});

op(0x23, 1, "inc hl", {
    //
});

op(0x24, 1, "inc h", {
    //
});

op(0x25, 1, "dec h", {
    //
});

op(0x26, 2, "ld h, $0x%02X", {
    //
});

op(0x27, 1, "daa", {
    //
});

op(0x28, 2, "jr z, $0x%02X", {
    //
});

op(0x29, 1, "add hl, hl", {
    //
});

op(0x2A, 1, "ld a, (hl+)", {
    //
});

op(0x2B, 1, "dec hl", {
    //
});

op(0x2C, 1, "inc l", {
    //
});

op(0x2D, 1, "dec l", {
    //
});

op(0x2E, 2, "ld l, $0x%02X", {
    //
});

op(0x2F, 1, "cpl", {
    //
});

op(0x30, 2, "jr nc, $0x%02X", {
    //
});

op(0x31, 3, "ld sp, $0x%04X", {
    //
});

op(0x32, 1, "ld (hl-), a", {
    //
});

op(0x33, 1, "inc sp", {
    //
});

op(0x34, 1, "inc (hl)", {
    //
});

op(0x35, 1, "dec (hl)", {
    //
});

op(0x36, 2, "ld (hl), $0x%02X", {
    //
});

op(0x37, 1, "scf", {
    //
});

op(0x38, 2, "jr c, $0x%02X", {
    //
});

op(0x39, 1, "add hl, sp", {
    //
});

op(0x3A, 1, "ld a, (hl-)", {
    //
});

op(0x3B, 1, "dec sp", {
    //
});

op(0x3C, 1, "inc a", {
    //
});

op(0x3D, 1, "dec a", {
    //
});

op(0x3E, 2, "ld a, $0x%02X", {
    //
});

op(0x3F, 1, "ccf", {
    //
});

#define ld_r(code, dst, dbase, src, sbase)              \
    op(code, 1, "ld " #dst ", " #src, {                 \
        cpu->state.dbase.dst = cpu->state.sbase.src;    \
    })

#define ld_r_a(code, dst, dbase)                        \
    op(code, 1, "ld " #dst ", a", {                     \
        cpu->state.dbase.dst = cpu->state.a;            \
    })

#define ld_a_r(code, src, sbase)                        \
    op(code, 1, "ld a, " #src, {                        \
        cpu->state.a = cpu->state.sbase.src;            \
    })

ld_r(0x40, b, bc, b, bc);
ld_r(0x41, b, bc, c, bc);
ld_r(0x42, b, bc, d, de);
ld_r(0x43, b, bc, e, de);
ld_r(0x44, b, bc, h, hl);
ld_r(0x45, b, bc, l, hl);

op(0x46, 1, "ld b, (hl)", {
    //
});

ld_a_r(0x47, b, bc);

ld_r(0x48, c, bc, b, bc);
ld_r(0x49, c, bc, c, bc);
ld_r(0x4A, c, bc, d, de);
ld_r(0x4B, c, bc, e, de);
ld_r(0x4C, c, bc, h, hl);
ld_r(0x4D, c, bc, l, hl);

op(0x4E, 1, "ld c, (hl)", {
    //
});

ld_a_r(0x4F, c, bc);

ld_r(0x50, d, de, b, bc);
ld_r(0x51, d, de, c, bc);
ld_r(0x52, d, de, d, de);
ld_r(0x53, d, de, e, de);
ld_r(0x54, d, de, h, hl);
ld_r(0x55, d, de, l, hl);

op(0x56, 1, "ld d, (hl)", {
    //
});

ld_a_r(0x57, d, de);

ld_r(0x58, e, de, b, bc);
ld_r(0x59, e, de, c, bc);
ld_r(0x5A, e, de, d, de);
ld_r(0x5B, e, de, e, de);
ld_r(0x5C, e, de, h, hl);
ld_r(0x5D, e, de, l, hl);

op(0x5E, 1, "ld e, (hl)", {
    //
});

ld_a_r(0x5F, e, de);

ld_r(0x60, h, hl, b, bc);
ld_r(0x61, h, hl, c, bc);
ld_r(0x62, h, hl, d, de);
ld_r(0x63, h, hl, e, de);
ld_r(0x64, h, hl, h, hl);
ld_r(0x65, h, hl, l, hl);

op(0x66, 1, "ld h, (hl)", {
    //
});

ld_a_r(0x67, h, hl);

ld_r(0x68, l, hl, b, bc);
ld_r(0x69, l, hl, c, bc);
ld_r(0x6A, l, hl, d, de);
ld_r(0x6B, l, hl, e, de);
ld_r(0x6C, l, hl, h, hl);
ld_r(0x6D, l, hl, l, hl);

op(0x6E, 1, "ld l, (hl)", {
    //
});

ld_a_r(0x6F, l, hl);

op(0x70, 1, "ld (hl), b", {
    //
});

op(0x71, 1, "ld (hl), c", {
    //
});

op(0x72, 1, "ld (hl), d", {
    //
});

op(0x73, 1, "ld (hl), e", {
    //
});

op(0x74, 1, "ld (hl), h", {
    //
});

op(0x75, 1, "ld (hl), l", {
    //
});

op(0x76, 1, "halt", {
    //
});

op(0x77, 1, "ld (hl), a", {
    //
});

op(0x78, 1, "ld a, b", {
    //
});

op(0x79, 1, "ld a, c", {
    //
});

op(0x7A, 1, "ld a, d", {
    //
});

op(0x7B, 1, "ld a, e", {
    //
});

op(0x7C, 1, "ld a, h", {
    //
});

op(0x7D, 1, "ld a, l", {
    //
});

op(0x7E, 1, "ld a, (hl)", {
    //
});

op(0x7F, 1, "ld a, a", {
    //
});

op(0x80, 1, "add a, b", {
    //
});

op(0x81, 1, "add a, c", {
    //
});

op(0x82, 1, "add a, d", {
    //
});

op(0x83, 1, "add a, e", {
    //
});

op(0x84, 1, "add a, h", {
    //
});

op(0x85, 1, "add a, l", {
    //
});

op(0x86, 1, "add a, (hl)", {
    //
});

op(0x87, 1, "add a, a", {
    //
});

op(0x88, 1, "adc a, b", {
    //
});

op(0x89, 1, "adc a, c", {
    //
});

op(0x8A, 1, "adc a, d", {
    //
});

op(0x8B, 1, "adc a, e", {
    //
});

op(0x8C, 1, "adc a, h", {
    //
});

op(0x8D, 1, "adc a, l", {
    //
});

op(0x8E, 1, "adc a, (hl)", {
    //
});

op(0x8F, 1, "adc a, a", {
    //
});

op(0x90, 1, "sub b", {
    //
});

op(0x91, 1, "sub c", {
    //
});

op(0x92, 1, "sub d", {
    //
});

op(0x93, 1, "sub e", {
    //
});

op(0x94, 1, "sub h", {
    //
});

op(0x95, 1, "sub l", {
    //
});

op(0x96, 1, "sub (hl)", {
    //
});

op(0x97, 1, "sub a", {
    //
});

op(0x98, 1, "sbc a, b", {
    //
});

op(0x99, 1, "sbc a, c", {
    //
});

op(0x9A, 1, "sbc a, d", {
    //
});

op(0x9B, 1, "sbc a, e", {
    //
});

op(0x9C, 1, "sbc a, h", {
    //
});

op(0x9D, 1, "sbc a, l", {
    //
});

op(0x9E, 1, "sbc a, (hl)", {
    //
});

op(0x9F, 1, "sbc a, a", {
    //
});

op(0xA0, 1, "and b", {
    //
});

op(0xA1, 1, "and c", {
    //
});

op(0xA2, 1, "and d", {
    //
});

op(0xA3, 1, "and e", {
    //
});

op(0xA4, 1, "and h", {
    //
});

op(0xA5, 1, "and l", {
    //
});

op(0xA6, 1, "and (hl)", {
    //
});

op(0xA7, 1, "and a", {
    //
});

op(0xA8, 1, "xor b", {
    //
});

op(0xA9, 1, "xor c", {
    //
});

op(0xAA, 1, "xor d", {
    //
});

op(0xAB, 1, "xor e", {
    //
});

op(0xAC, 1, "xor h", {
    //
});

op(0xAD, 1, "xor l", {
    //
});

op(0xAE, 1, "xor (hl)", {
    //
});

op(0xAF, 1, "xor a", {
    //
});

op(0xB0, 1, "or b", {
    //
});

op(0xB1, 1, "or c", {
    //
});

op(0xB2, 1, "or d", {
    //
});

op(0xB3, 1, "or e", {
    //
});

op(0xB4, 1, "or h", {
    //
});

op(0xB5, 1, "or l", {
    //
});

op(0xB6, 1, "or (hl)", {
    //
});

op(0xB7, 1, "or a", {
    //
});

op(0xB8, 1, "cp b", {
    //
});

op(0xB9, 1, "cp c", {
    //
});

op(0xBA, 1, "cp d", {
    //
});

op(0xBB, 1, "cp e", {
    //
});

op(0xBC, 1, "cp h", {
    //
});

op(0xBD, 1, "cp l", {
    //
});

op(0xBE, 1, "cp (hl)", {
    //
});

op(0xBF, 1, "cp a", {
    //
});

op(0xC0, 1, "ret nz", {
    //
});

op(0xC1, 1, "pop bc", {
    //
});

op(0xC2, 3, "jp nz, $0x%04X", {
    //
});

op(0xC3, 3, "jp $0x%04X", {
    //
});

op(0xC4, 3, "call nz, $0x%04X", {
    //
});

op(0xC5, 1, "push bc", {
    //
});

op(0xC6, 2, "add a, $0x%02X", {
    //
});

op(0xC7, 1, "rst 00h", {
    //
});

op(0xC8, 1, "ret z", {
    //
});

op(0xC9, 1, "ret", {
    //
});

op(0xCA, 3, "jp z, $0x%04X", {
    //
});

op(0xCB, 1, "cb.", {
    /* This is a prefix opcode */
});

op(0xCC, 3, "call z, $0x%04X", {
    //
});

op(0xCD, 3, "call $0x%04X", {
    //
});

op(0xCE, 2, "adc a, $0x%02X", {
    //
});

op(0xCF, 1, "rst 08h", {
    //
});

op(0xD0, 1, "ret nc", {
    //
});

op(0xD1, 1, "pop de", {
    //
});

op(0xD2, 3, "jp nc, $0x%04X", {
    //
});

op(0xD3, 1, "ud 0xD3", {
    //
});

op(0xD4, 3, "call nc, $0x%04X", {
    //
});

op(0xD5, 1, "push de", {
    //
});

op(0xD6, 2, "sub $0x%02X", {
    //
});

op(0xD7, 1, "rst 10h", {
    //
});

op(0xD8, 1, "ret c", {
    //
});

op(0xD9, 1, "reti", {
    //
});

op(0xDA, 3, "jp c, $0x%04X", {
    //
});

op(0xDB, 1, "ud 0xDB", {
    //
});

op(0xDC, 3, "call c, $0x%04X", {
    //
});

op(0xDD, 1, "ud 0xDD", {
    //
});

op(0xDE, 2, "sbc a, $0x%02X", {
    //
});

op(0xDF, 1, "rst 18h", {
    //
});

op(0xE0, 2, "ldh $0x%02X(0xFF00), a", {
    //
});

op(0xE1, 1, "pop hl", {
    //
});

op(0xE2, 1, "ld c(0xFF00), a", {
    //
});

op(0xE3, 1, "ud 0xE3", {
    //
});

op(0xE4, 1, "ud 0xE4", {
    //
});

op(0xE5, 1, "push hl", {
    //
});

op(0xE6, 2, "and $0x%02X", {
    //
});

op(0xE7, 1, "rst 20h", {
    //
});

op(0xE8, 2, "add sp, $0x%02X", {
    //
});

op(0xE9, 1, "jp (hl)", {
    //
});

op(0xEA, 3, "ld ($0x%04X), a", {
    //
});

op(0xEB, 1, "ud 0xEB", {
    //
});

op(0xEC, 1, "ud 0xEC", {
    //
});

op(0xED, 1, "ud 0xED", {
    //
});

op(0xEE, 2, "xor $0x%02X", {
    //
});

op(0xEF, 1, "rst 28h", {
    //
});

op(0xF0, 2, "ldh a, $0x%02X(0xFF00)", {
    //
});

op(0xF1, 1, "pop af", {
    //
});

op(0xF2, 1, "ld a, c(0xFF00)", {
    //
});

op(0xF3, 1, "di", {
    //
});

op(0xF4, 1, "ud 0xF4", {
    //
});

op(0xF5, 1, "push af", {
    //
});

op(0xF6, 2, "or $0x%02X", {
    //
});

op(0xF7, 1, "rst 30h", {
    //
});

op(0xF8, 2, "ld hl, $0x%02X(sp)", {
    //
});

op(0xF9, 1, "ld sp, hl", {
    //
});

op(0xFA, 3, "ld a, ($0x%04X)", {
    //
});

op(0xFB, 1, "ei", {
    //
});

op(0xFC, 1, "ud 0xFC", {
    //
});

op(0xFD, 1, "ud 0xFD", {
    //
});

op(0xFE, 2, "cp $0x%02X", {
    //
});

op(0xFF, 1, "rst 38h", {
    //
});

#pragma mark - Prefixed Instructions

#define rlc(code, reg)                          \
    op_pre(code, 1, "rlc " #reg, {              \
        /* */                                   \
    })

#define rrc(code, reg)                          \
    op_pre(code, 1, "rrc " #reg, {              \
        /* */                                   \
    })

#define rl(code, reg)                           \
    op_pre(code, 1, "rl " #reg, {               \
        /* */                                   \
    })

#define rr(code, reg)                           \
    op_pre(code, 1, "rr " #reg, {               \
        /* */                                   \
    })

#define sla(code, reg)                          \
    op_pre(code, 1, "sla " #reg, {              \
        /* */                                   \
    })

#define sra(code, reg)                          \
    op_pre(code, 1, "sra " #reg, {              \
        /* */                                   \
    })

#define swap(code, reg)                         \
    op_pre(code, 1, "swap " #reg, {             \
        /* */                                   \
    })

#define srl(code, reg)                          \
    op_pre(code, 1, "srl " #reg, {              \
        /* */                                   \
    })

#define bit(code, pos, reg)                     \
    op_pre(code, 1, "bit " #pos ", " #reg, {    \
        /* */                                   \
    })

#define res(code, pos, reg)                     \
    op_pre(code, 1, "res " #pos ", " #reg, {    \
        /* */                                   \
    })

#define set(code, pos, reg)                     \
    op_pre(code, 1, "set " #pos ", " #reg, {    \
        /* */                                   \
    })

rlc(0x00, b);    rlc(0x01, c);
rlc(0x02, d);    rlc(0x03, e);
rlc(0x04, h);    rlc(0x05, l);
rlc(0x06, (hl)); rlc(0x07, a);

rrc(0x08, b);    rrc(0x09, c);
rrc(0x0A, d);    rrc(0x0B, e);
rrc(0x0C, h);    rrc(0x0D, l);
rrc(0x0E, (hl)); rrc(0x0F, a);

rl(0x10, b);    rl(0x11, c);
rl(0x12, d);    rl(0x13, e);
rl(0x14, h);    rl(0x15, l);
rl(0x16, (hl)); rl(0x17, a);

rr(0x18, b);    rr(0x19, c);
rr(0x1A, d);    rr(0x1B, e);
rr(0x1C, h);    rr(0x1D, l);
rr(0x1E, (hl)); rr(0x1F, a);

sla(0x20, b);    sla(0x21, c);
sla(0x22, d);    sla(0x23, e);
sla(0x24, h);    sla(0x25, l);
sla(0x26, (hl)); sla(0x27, a);

sra(0x28, b);    sra(0x29, c);
sra(0x2A, d);    sra(0x2B, e);
sra(0x2C, h);    sra(0x2D, l);
sra(0x2E, (hl)); sra(0x2F, a);

swap(0x30, b);    swap(0x31, c);
swap(0x32, d);    swap(0x33, e);
swap(0x34, h);    swap(0x35, l);
swap(0x36, (hl)); swap(0x37, a);

srl(0x38, b);    srl(0x39, c);
srl(0x3A, d);    srl(0x3B, e);
srl(0x3C, h);    srl(0x3D, l);
srl(0x3E, (hl)); srl(0x3F, a);

bit(0x40, 0, b); bit(0x41, 0, c); bit(0x42, 0, d); bit(0x43, 0, e); bit(0x44, 0, h); bit(0x45, 0, l); bit(0x46, 0, (hl)); bit(0x47, 0, a);
bit(0x48, 1, b); bit(0x49, 1, c); bit(0x4A, 1, d); bit(0x4B, 1, e); bit(0x4C, 1, h); bit(0x4D, 1, l); bit(0x4E, 1, (hl)); bit(0x4F, 1, a);
bit(0x50, 2, b); bit(0x51, 2, c); bit(0x52, 2, d); bit(0x53, 2, e); bit(0x54, 2, h); bit(0x55, 2, l); bit(0x56, 2, (hl)); bit(0x57, 2, a);
bit(0x58, 3, b); bit(0x59, 3, c); bit(0x5A, 3, d); bit(0x5B, 3, e); bit(0x5C, 3, h); bit(0x5D, 3, l); bit(0x5E, 3, (hl)); bit(0x5F, 3, a);
bit(0x60, 4, b); bit(0x61, 4, c); bit(0x62, 4, d); bit(0x63, 4, e); bit(0x64, 4, h); bit(0x65, 4, l); bit(0x66, 4, (hl)); bit(0x67, 4, a);
bit(0x68, 5, b); bit(0x69, 5, c); bit(0x6A, 5, d); bit(0x6B, 5, e); bit(0x6C, 5, h); bit(0x6D, 5, l); bit(0x6E, 5, (hl)); bit(0x6F, 5, a);
bit(0x70, 6, b); bit(0x71, 6, c); bit(0x72, 6, d); bit(0x73, 6, e); bit(0x74, 6, h); bit(0x75, 6, l); bit(0x76, 6, (hl)); bit(0x77, 6, a);
bit(0x78, 7, b); bit(0x79, 7, c); bit(0x7A, 7, d); bit(0x7B, 7, e); bit(0x7C, 7, h); bit(0x7D, 7, l); bit(0x7E, 7, (hl)); bit(0x7F, 7, a);

res(0x80, 0, b); res(0x81, 0, c); res(0x82, 0, d); res(0x83, 0, e); res(0x84, 0, h); res(0x85, 0, l); res(0x86, 0, (hl)); res(0x87, 0, a);
res(0x88, 1, b); res(0x89, 1, c); res(0x8A, 1, d); res(0x8B, 1, e); res(0x8C, 1, h); res(0x8D, 1, l); res(0x8E, 1, (hl)); res(0x8F, 1, a);
res(0x90, 2, b); res(0x91, 2, c); res(0x92, 2, d); res(0x93, 2, e); res(0x94, 2, h); res(0x95, 2, l); res(0x96, 2, (hl)); res(0x97, 2, a);
res(0x98, 3, b); res(0x99, 3, c); res(0x9A, 3, d); res(0x9B, 3, e); res(0x9C, 3, h); res(0x9D, 3, l); res(0x9E, 3, (hl)); res(0x9F, 3, a);
res(0xA0, 4, b); res(0xA1, 4, c); res(0xA2, 4, d); res(0xA3, 4, e); res(0xA4, 4, h); res(0xA5, 4, l); res(0xA6, 4, (hl)); res(0xA7, 4, a);
res(0xA8, 5, b); res(0xA9, 5, c); res(0xAA, 5, d); res(0xAB, 5, e); res(0xAC, 5, h); res(0xAD, 5, l); res(0xAE, 5, (hl)); res(0xAF, 5, a);
res(0xB0, 6, b); res(0xB1, 6, c); res(0xB2, 6, d); res(0xB3, 6, e); res(0xB4, 6, h); res(0xB5, 6, l); res(0xB6, 6, (hl)); res(0xB7, 6, a);
res(0xB8, 7, b); res(0xB9, 7, c); res(0xBA, 7, d); res(0xBB, 7, e); res(0xBC, 7, h); res(0xBD, 7, l); res(0xBE, 7, (hl)); res(0xBF, 7, a);

set(0xC0, 0, b); set(0xC1, 0, c); set(0xC2, 0, d); set(0xC3, 0, e); set(0xC4, 0, h); set(0xC5, 0, l); set(0xC6, 0, (hl)); set(0xC7, 0, a);
set(0xC8, 1, b); set(0xC9, 1, c); set(0xCA, 1, d); set(0xCB, 1, e); set(0xCC, 1, h); set(0xCD, 1, l); set(0xCE, 1, (hl)); set(0xCF, 1, a);
set(0xD0, 2, b); set(0xD1, 2, c); set(0xD2, 2, d); set(0xD3, 2, e); set(0xD4, 2, h); set(0xD5, 2, l); set(0xD6, 2, (hl)); set(0xD7, 2, a);
set(0xD8, 3, b); set(0xD9, 3, c); set(0xDA, 3, d); set(0xDB, 3, e); set(0xDC, 3, h); set(0xDD, 3, l); set(0xDE, 3, (hl)); set(0xDF, 3, a);
set(0xE0, 4, b); set(0xE1, 4, c); set(0xE2, 4, d); set(0xE3, 4, e); set(0xE4, 4, h); set(0xE5, 4, l); set(0xE6, 4, (hl)); set(0xE7, 4, a);
set(0xE8, 5, b); set(0xE9, 5, c); set(0xEA, 5, d); set(0xEB, 5, e); set(0xEC, 5, h); set(0xED, 5, l); set(0xEE, 5, (hl)); set(0xEF, 5, a);
set(0xF0, 6, b); set(0xF1, 6, c); set(0xF2, 6, d); set(0xF3, 6, e); set(0xF4, 6, h); set(0xF5, 6, l); set(0xF6, 6, (hl)); set(0xF7, 6, a);
set(0xF8, 7, b); set(0xF9, 7, c); set(0xFA, 7, d); set(0xFB, 7, e); set(0xFC, 7, h); set(0xFD, 7, l); set(0xFE, 7, (hl)); set(0xFF, 7, a);

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

const static GBProcessorOP *const gGBInstructionSetCB[0x100] = {
    expand(0, 0x), expand(1, 0x), expand(2, 0x), expand(3, 0x),
    expand(4, 0x), expand(5, 0x), expand(6, 0x), expand(7, 0x),
    expand(8, 0x), expand(9, 0x), expand(A, 0x), expand(B, 0x),
    expand(C, 0x), expand(D, 0x), expand(E, 0x), expand(F, 0x)
};

static const GBProcessorOP *gGBInstructionSet[0x100] = {
    expand(0x0, pre_),  expand(0x1, pre_), expand(0x2, pre_), expand(0x3, pre_),
    expand(0x4, pre_),  expand(0x5, pre_), expand(0x6, pre_), expand(0x7, pre_),
    expand(0x8, pre_),  expand(0x9, pre_), expand(0xA, pre_), expand(0xB, pre_),
    expand(0xC, pre_),  expand(0xD, pre_), expand(0xE, pre_), expand(0xF, pre_)
};

#undef expand

#undef op
#undef declare

#endif /* !defined(kGBDisassembler) */
