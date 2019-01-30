#define declare(op, str)                                                                \
    static void __GBProcessorDo_ ## op(GBProcessorOP *this, struct __GBProcessor *cpu); \
                                                                                        \
    const static GBProcessorOP op_## op = {                                             \
        .name = str,                                                                    \
        .value = op,                                                                    \
        .go = __GBProcessorDo_ ## op                                                    \
    }

#define op(code, str, impl)                                                             \
    declare(code, str);                                                                 \
                                                                                        \
    void __GBProcessorDo_ ## code(GBProcessorOP *this, struct __GBProcessor *cpu)       \
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

#pragma mark - Instruction Implementation

op(0x00, "nop", { /* nop doesn't do anything, sorry */ });

op(0x01, "ld bc, 0x%04X", {
    __GBProcessorSetupAccess(cpu, cpu->state.pc);
    __GBProcessorRead16(cpu);

    cpu->state.pc += 2;
    cpu->state.bc.val = cpu->state.mdr;
});

op(0x02, "ld (bc), a", {
    __GBProcessorSetupAccess(cpu, cpu->state.bc.val);
    cpu->state.mdr = cpu->state.a;

    __GBProcessorWrite8(cpu);
});

op(0x03, "inc bc", {
    __GBProcessorStall(cpu, 1);

    cpu->state.bc.val++;
});

op(0x04, "inc b", {
    UInt8 carry = cpu->state.f.bit.c;
    UInt8 one = 1;

    __ALUAdd8(cpu, &cpu->state.bc.b, &one, 0);
    cpu->state.f.bit.c = carry;
});

op(0x05, "dec b", {
    UInt8 carry = cpu->state.f.bit.c;
    SInt8 one = 1;

    __ALUSub8(cpu, (SInt8 *)&cpu->state.bc.b, &one, 0);
    cpu->state.f.bit.c = carry;
});

op(0x06, "ld b, 0x%02X", {
    __GBProcessorSetupAccess(cpu, cpu->state.pc);
    __GBProcessorRead8(cpu);

    cpu->state.pc++;
    cpu->state.bc.b = cpu->state.mdr;
});

op(0x07, "rcla", {
    __ALURotateLeft(&cpu->state.a, 1);

    cpu->state.f.bit.z = 0;
    cpu->state.f.bit.n = 0;
    cpu->state.f.bit.h = 0;
    cpu->state.f.bit.c = cpu->state.a & 1;
});

op(0x08, "ld (0x%04X), sp", {
    //
});

op(0x09, "add hl, bc", {
    //
});

op(0x10, "ld a, (bc)", {
    //
});

op(0x0A, "dec bc", {
    //
});

op(0x0B, "inc c", {
    //
});

op(0x0C, "dec c", {
    //
});

op(0x0D, "ld c, 0x%02X", {
    //
});

op(0x0E, "rrca", {
    //
});

op(0x0F, "ld bc, 0x%016X", {
    //
});

#undef op
#undef declare
