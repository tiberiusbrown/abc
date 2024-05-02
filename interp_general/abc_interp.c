#include "abc_interp.h"

#include <assert.h>

enum
{
    I_NOP,
    I_PUSH,
    I_P0,
    I_P1,
    I_P2,
    I_P3,
    I_P4,
    I_P5,
    I_P6,
    I_P7,
    I_P8,
    I_P16,
    I_P32,
    I_P64,
    I_P128,
    I_P00,
    I_P000,
    I_P0000,
    I_PZ8,
    I_PZ16,
    I_PUSHG,
    I_PUSHL,
    I_PUSH4,
    I_SEXT,
    I_SEXT2,
    I_SEXT3,
    I_DUP,
    I_DUP2,
    I_DUP3,
    I_DUP4,
    I_DUP5,
    I_DUP6,
    I_DUP7,
    I_DUP8,
    I_DUPW,
    I_DUPW2,
    I_DUPW3,
    I_DUPW4,
    I_DUPW5,
    I_DUPW6,
    I_DUPW7,
    I_DUPW8,
    I_GETL,
    I_GETL2,
    I_GETL4,
    I_GETLN,
    I_SETL,
    I_SETL2,
    I_SETL4,
    I_SETLN,
    I_GETG,
    I_GETG2,
    I_GETG4,
    I_GETGN,
    I_GTGB,
    I_GTGB2,
    I_GTGB4,
    I_SETG,
    I_SETG2,
    I_SETG4,
    I_SETGN,
    I_GETP,
    I_GETPN,
    I_GETR,
    I_GETR2,
    I_GETRN,
    I_SETR,
    I_SETR2,
    I_SETRN,
    I_POP,
    I_POP2,
    I_POP3,
    I_POP4,
    I_POPN,
    I_AIXB1,
    I_AIDXB,
    I_AIDX,
    I_PIDXB,
    I_PIDX,
    I_UAIDX,
    I_UPIDX,
    I_ASLC,
    I_PSLC,
    I_REFL,
    I_REFGB,
    I_INC,
    I_DEC,
    I_LINC,
    I_PINC,
    I_PINC2,
    I_PINC3,
    I_PINC4,
    I_PDEC,
    I_PDEC2,
    I_PDEC3,
    I_PDEC4,
    I_PINCF,
    I_PDECF,
    I_ADD,
    I_ADD2,
    I_ADD3,
    I_ADD4,
    I_SUB,
    I_SUB2,
    I_SUB3,
    I_SUB4,
    I_ADD2B,
    I_ADD3B,
    I_SUB2B,
    I_MUL2B,
    I_MUL,
    I_MUL2,
    I_MUL3,
    I_MUL4,
    I_UDIV2,
    I_UDIV4,
    I_DIV2,
    I_DIV4,
    I_UMOD2,
    I_UMOD4,
    I_MOD2,
    I_MOD4,
    I_LSL,
    I_LSL2,
    I_LSL4,
    I_LSR,
    I_LSR2,
    I_LSR4,
    I_ASR,
    I_ASR2,
    I_ASR4,
    I_AND,
    I_AND2,
    I_AND4,
    I_OR,
    I_OR2,
    I_OR4,
    I_XOR,
    I_XOR2,
    I_XOR4,
    I_COMP,
    I_COMP2,
    I_COMP4,
    I_BOOL,
    I_BOOL2,
    I_BOOL3,
    I_BOOL4,
    I_CULT,
    I_CULT2,
    I_CULT3,
    I_CULT4,
    I_CSLT,
    I_CSLT2,
    I_CSLT3,
    I_CSLT4,
    I_CFEQ,
    I_CFLT,
    I_NOT,
    I_FADD,
    I_FSUB,
    I_FMUL,
    I_FDIV,
    I_F2I,
    I_F2U,
    I_I2F,
    I_U2F,
    I_BZ,
    I_BZ1,
    I_BNZ,
    I_BNZ1,
    I_BZP,
    I_BZP1,
    I_BNZP,
    I_BNZP1,
    I_JMP,
    I_JMP1,
    I_CALL,
    I_CALL1,
    I_RET,
    I_SYS,
};

static uint8_t space(abc_interp_t* interp, uint8_t n)
{
    return interp->sp < (uint8_t)(256 - n) ? 1 : 0;
}

static uint8_t imm8(abc_interp_t* interp, abc_host_t const* h)
{
    return h->prog(h->user, interp->pc++);
}

static uint16_t imm16(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t t = ((uint16_t)(h->prog(h->user, interp->pc++)) << 0);
    t +=         ((uint16_t)(h->prog(h->user, interp->pc++)) << 8);
    return t;
}

static uint32_t imm24(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t t = ((uint32_t)(h->prog(h->user, interp->pc++)) << 0);
    t +=         ((uint32_t)(h->prog(h->user, interp->pc++)) << 8);
    t +=         ((uint32_t)(h->prog(h->user, interp->pc++)) << 16);
    return t;
}

static abc_result_t push(abc_interp_t* interp, uint8_t x)
{
    if(!space(interp, 1)) return ABC_RESULT_ERROR;
    interp->stack[interp->sp++] = x;
    return ABC_RESULT_NORMAL;
}

static abc_result_t push16(abc_interp_t* interp, uint16_t x)
{
    if(!space(interp, 2)) return ABC_RESULT_ERROR;
    (void)push(interp, (uint8_t)(x >> 0));
    (void)push(interp, (uint8_t)(x >> 8));
    return ABC_RESULT_NORMAL;
}

static abc_result_t push24(abc_interp_t* interp, uint32_t x)
{
    if(!space(interp, 3)) return ABC_RESULT_ERROR;
    (void)push(interp, (uint8_t)(x >> 0));
    (void)push(interp, (uint8_t)(x >> 8));
    (void)push(interp, (uint8_t)(x >> 16));
    return ABC_RESULT_NORMAL;
}

static abc_result_t push32(abc_interp_t* interp, uint32_t x)
{
    if(!space(interp, 4)) return ABC_RESULT_ERROR;
    (void)push(interp, (uint8_t)(x >> 0));
    (void)push(interp, (uint8_t)(x >> 8));
    (void)push(interp, (uint8_t)(x >> 16));
    (void)push(interp, (uint8_t)(x >> 24));
    return ABC_RESULT_NORMAL;
}

static abc_result_t pushf(abc_interp_t* interp, float f)
{
    union { float f; uint32_t x; } u;
    u.f = f;
    return push32(interp, u.x);
}

static uint8_t pop8(abc_interp_t* interp)
{
    return interp->stack[--interp->sp];
}

static uint16_t pop16(abc_interp_t* interp)
{
    uint16_t t = pop8(interp);
    t = (t << 8) + pop8(interp);
    return t;
}

static uint32_t pop24(abc_interp_t* interp)
{
    uint32_t t = pop8(interp);
    t = (t << 8) + pop8(interp);
    t = (t << 8) + pop8(interp);
    return t;
}

static int32_t pop24signed(abc_interp_t* interp)
{
    uint32_t t = pop24(interp);
    if(t & 0x00800000)
        t |= 0xff000000;
    return (int32_t)t;
}

static uint32_t pop32(abc_interp_t* interp)
{
    uint32_t t = pop8(interp);
    t = (t << 8) + pop8(interp);
    t = (t << 8) + pop8(interp);
    t = (t << 8) + pop8(interp);
    return t;
}

static float popf(abc_interp_t* interp)
{
    union { float f; uint32_t x; } u;
    u.x = pop32(interp);
    return u.f;
}

static abc_result_t pushn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, h->prog(h->user, interp->pc++));
    return ABC_RESULT_NORMAL;
}

static abc_result_t push_zn(abc_interp_t* interp, uint8_t n)
{
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, 0);
    return ABC_RESULT_NORMAL;
}

static uint8_t headn(abc_interp_t* interp, uint8_t n)
{
    return interp->stack[(uint8_t)(interp->sp - n)];
}

static uint8_t head(abc_interp_t* interp)
{
    return headn(interp, 1);
}

static abc_result_t dupw(abc_interp_t* interp, uint8_t n)
{
    if(!space(interp, 2)) return ABC_RESULT_ERROR;
    (void)push(interp, headn(interp, n + 1));
    (void)push(interp, headn(interp, n + 1));
    return ABC_RESULT_NORMAL;
}

static abc_result_t getln(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    uint8_t t = imm8(interp, h);
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, headn(interp, t));
    return ABC_RESULT_NORMAL;
}

static abc_result_t setln(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    uint8_t t = imm8(interp, h);
    for(uint8_t i = 0; i < n; ++i)
    {
        uint8_t x = pop8(interp);
        interp->stack[(uint8_t)(interp->sp - t)] = x;
    }
    return ABC_RESULT_NORMAL;
}

static abc_result_t sextn(abc_interp_t* interp, uint8_t n)
{
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    uint8_t t = head(interp) & 0x80 ? 0xff : 0x00;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, t);
    return ABC_RESULT_NORMAL;
}

static abc_result_t getgn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    uint16_t t = imm16(interp, h);
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, interp->globals[(t + i) & 1023]);
    return ABC_RESULT_NORMAL;
}

static abc_result_t gtgbn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    uint16_t t = imm8(interp, h);
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, interp->globals[(t + i) & 1023]);
    return ABC_RESULT_NORMAL;
}

static abc_result_t setgn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    uint16_t t = imm16(interp, h) + n - 1;
    for(uint8_t i = 0; i < n; ++i)
        interp->globals[(t - i) & 1023] = interp->stack[--interp->sp];
    return ABC_RESULT_NORMAL;
}

static abc_result_t add(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a + b);
}

static abc_result_t add2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a + b);
}

static abc_result_t add3(abc_interp_t* interp)
{
    uint32_t b = pop24(interp);
    uint32_t a = pop24(interp);
    return push24(interp, a + b);
}

static abc_result_t add4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push32(interp, a + b);
}

static abc_result_t add2b(abc_interp_t* interp)
{
    uint16_t b = pop8(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a + b);
}

static abc_result_t add3b(abc_interp_t* interp)
{
    uint16_t b = pop8(interp);
    uint32_t a = pop24(interp);
    return push24(interp, a + b);
}

static abc_result_t sub(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a - b);
}

static abc_result_t sub2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a - b);
}

static abc_result_t sub3(abc_interp_t* interp)
{
    uint32_t b = pop24(interp);
    uint32_t a = pop24(interp);
    return push24(interp, a - b);
}

static abc_result_t sub4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push32(interp, a - b);
}

static abc_result_t sub2b(abc_interp_t* interp)
{
    uint16_t b = pop8(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a - b);
}

static abc_result_t mul(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a * b);
}

static abc_result_t mul2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a * b);
}

static abc_result_t mul3(abc_interp_t* interp)
{
    uint32_t b = pop24(interp);
    uint32_t a = pop24(interp);
    return push24(interp, a * b);
}

static abc_result_t mul4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push32(interp, a * b);
}

static abc_result_t mul2b(abc_interp_t* interp)
{
    uint16_t b = pop8(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a * b);
}

static abc_result_t udiv2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push16(interp, a / b);
}

static abc_result_t udiv4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push32(interp, a / b);
}

static abc_result_t div2(abc_interp_t* interp)
{
    uint16_t b = (int16_t)pop16(interp);
    uint16_t a = (int16_t)pop16(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push16(interp, (uint16_t)(a / b));
}

static abc_result_t div4(abc_interp_t* interp)
{
    int32_t b = (int32_t)pop32(interp);
    int32_t a = (int32_t)pop32(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push32(interp, (uint32_t)(a / b));
}

static abc_result_t umod2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push16(interp, a % b);
}

static abc_result_t umod4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push32(interp, a % b);
}

static abc_result_t mod2(abc_interp_t* interp)
{
    uint16_t b = (int16_t)pop16(interp);
    uint16_t a = (int16_t)pop16(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push16(interp, (uint16_t)(a % b));
}

static abc_result_t mod4(abc_interp_t* interp)
{
    int32_t b = (int32_t)pop32(interp);
    int32_t a = (int32_t)pop32(interp);
    if(b == 0) return ABC_RESULT_ERROR;
    return push32(interp, (uint32_t)(a % b));
}

static abc_result_t lsl(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, b >= 8 ? 0 : a << b);
}

static abc_result_t lsl2(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint16_t a = pop16(interp);
    return push16(interp, b >= 16 ? 0 : a << b);
}

static abc_result_t lsl4(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint32_t a = pop32(interp);
    return push32(interp, b >= 32 ? 0 : a << b);
}

static abc_result_t lsr(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, b >= 8 ? 0 : a >> b);
}

static abc_result_t lsr2(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint16_t a = pop16(interp);
    return push16(interp, b >= 16 ? 0 : a >> b);
}

static abc_result_t lsr4(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint32_t a = pop32(interp);
    return push32(interp, b >= 32 ? 0 : a >> b);
}

static uint32_t asr_helper(int32_t x, uint8_t n)
{
    uint32_t sign = 0x80000000u >> n;
    uint32_t mask = ~(0xffffffffu >> n);
    uint32_t t = (uint32_t)x >> n;
    if((uint32_t)x & sign)
        t |= mask;
    return t;
}

static abc_result_t asr(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    int8_t a = (int8_t)pop8(interp);
    return push(interp,
        b >= 8 ? (a < 0 ? 0xff : 0) : (uint8_t)asr_helper((int32_t)a, b));
}

static abc_result_t asr2(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    int16_t a = (int16_t)pop16(interp);
    return push16(interp,
        b >= 16 ? (a < 0 ? 0xffff : 0) : (uint16_t)asr_helper((int32_t)a, b));
}

static abc_result_t asr4(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    int32_t a = (int32_t)pop32(interp);
    return push32(interp,
        b >= 32 ? (a < 0 ? 0xffffffff : 0) : asr_helper((int32_t)a, b));
}

static abc_result_t bw_and(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a & b);
}

static abc_result_t bw_and2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a & b);
}

static abc_result_t bw_and4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push32(interp, a & b);
}

static abc_result_t bw_or(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a | b);
}

static abc_result_t bw_or2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a | b);
}

static abc_result_t bw_or4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push32(interp, a | b);
}

static abc_result_t bw_xor(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a ^ b);
}

static abc_result_t bw_xor2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push16(interp, a ^ b);
}

static abc_result_t bw_xor4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push32(interp, a ^ b);
}

static abc_result_t bw_comp(abc_interp_t* interp)
{
    uint8_t a = pop8(interp);
    return push(interp, ~a);
}

static abc_result_t bw_comp2(abc_interp_t* interp)
{
    uint16_t a = pop16(interp);
    return push16(interp, ~a);
}

static abc_result_t bw_comp4(abc_interp_t* interp)
{
    uint32_t a = pop32(interp);
    return push32(interp, ~a);
}

static abc_result_t logical_not(abc_interp_t* interp)
{
    uint8_t a = pop8(interp);
    return push(interp, a != 0 ? 0 : 1);
}

static abc_result_t logical_bool(abc_interp_t* interp)
{
    uint8_t a = pop8(interp);
    return push(interp, a != 0 ? 1 : 0);
}

static abc_result_t logical_bool2(abc_interp_t* interp)
{
    uint16_t a = pop16(interp);
    return push(interp, a != 0 ? 1 : 0);
}

static abc_result_t logical_bool3(abc_interp_t* interp)
{
    uint32_t a = pop24(interp);
    return push(interp, a != 0 ? 1 : 0);
}

static abc_result_t logical_bool4(abc_interp_t* interp)
{
    uint32_t a = pop32(interp);
    return push(interp, a != 0 ? 1 : 0);
}

static abc_result_t getpn(abc_interp_t* interp, abc_host_t const* h, uint8_t n)
{
    uint32_t t = pop24(interp);
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, h->prog(h->user, t + i));
    return ABC_RESULT_NORMAL;
}

static abc_result_t getrn(abc_interp_t* interp, uint8_t n)
{
    uint16_t t = pop16(interp);
    if(t < 0x100 || t >= 0x600) return ABC_RESULT_ERROR;
    if(!space(interp, n)) return ABC_RESULT_ERROR;
    if(t < 0x200)
    {
        /* stack reference */
        for(uint8_t i = 0; i < n; ++i)
            (void)push(interp, interp->stack[t - 0x100]);
    }
    else
    {
        /* globals reference */
        for(uint8_t i = 0; i < n; ++i)
            (void)push(interp, interp->globals[t - 0x200]);
    }
    return ABC_RESULT_NORMAL;
}

static abc_result_t setrn(abc_interp_t* interp, uint8_t n)
{
    uint16_t t = pop16(interp);
    if(t < 0x100 || t >= 0x600) return ABC_RESULT_ERROR;
    if(t < 0x200)
    {
        /* stack reference */
        for(uint8_t i = 0; i < n; ++i)
            interp->stack[t + n - 1 - 0x100] = pop8(interp);
    }
    else
    {
        /* globals reference */
        for(uint8_t i = 0; i < n; ++i)
            interp->globals[t + n - 1 - 0x200] = pop8(interp);
    }
    return ABC_RESULT_NORMAL;
}

static abc_result_t cult(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    uint8_t a = pop8(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cult2(abc_interp_t* interp)
{
    uint16_t b = pop16(interp);
    uint16_t a = pop16(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cult3(abc_interp_t* interp)
{
    uint32_t b = pop24(interp);
    uint32_t a = pop24(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cult4(abc_interp_t* interp)
{
    uint32_t b = pop32(interp);
    uint32_t a = pop32(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cslt(abc_interp_t* interp)
{
    int8_t b = (int8_t)pop8(interp);
    int8_t a = (int8_t)pop8(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cslt2(abc_interp_t* interp)
{
    int16_t b = (int16_t)pop16(interp);
    int16_t a = (int16_t)pop16(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cslt3(abc_interp_t* interp)
{
    int32_t b = pop24signed(interp);
    int32_t a = pop24signed(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cslt4(abc_interp_t* interp)
{
    int32_t b = (int32_t)pop32(interp);
    int32_t a = (int32_t)pop32(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cflt(abc_interp_t* interp)
{
    float b = popf(interp);
    float a = popf(interp);
    return push(interp, a < b ? 1 : 0);
}

static abc_result_t cfeq(abc_interp_t* interp)
{
    float b = popf(interp);
    float a = popf(interp);
    return push(interp, a == b ? 1 : 0);
}

static abc_result_t fadd(abc_interp_t* interp)
{
    float b = popf(interp);
    float a = popf(interp);
    return pushf(interp, a + b);
}

static abc_result_t fsub(abc_interp_t* interp)
{
    float b = popf(interp);
    float a = popf(interp);
    return pushf(interp, a - b);
}

static abc_result_t fmul(abc_interp_t* interp)
{
    float b = popf(interp);
    float a = popf(interp);
    return pushf(interp, a * b);
}

static abc_result_t fdiv(abc_interp_t* interp)
{
    float b = popf(interp);
    float a = popf(interp);
    return pushf(interp, a / b);
}

static abc_result_t f2i(abc_interp_t* interp)
{
    float a = popf(interp);
    return push32(interp, (uint32_t)(int32_t)a);
}

static abc_result_t f2u(abc_interp_t* interp)
{
    float a = popf(interp);
    return push32(interp, (uint32_t)a);
}

static abc_result_t i2f(abc_interp_t* interp)
{
    int32_t a = (int32_t)pop32(interp);
    return pushf(interp, (float)a);
}

static abc_result_t u2f(abc_interp_t* interp)
{
    uint32_t a = pop32(interp);
    return pushf(interp, (float)a);
}

abc_result_t run(abc_interp_t* interp, abc_host_t const* h)
{
    if(!interp || !h || !h->prog || !h->millis)
        return ABC_RESULT_ERROR;

    if(interp->pc == 0)
    {
        /* starting point of execution */
        interp->pc = 20;
    }
    
    uint8_t instr = h->prog(h->user, interp->pc++);
    
    switch(instr)
    {
    case I_NOP:   return ABC_RESULT_NORMAL;
    case I_PUSH:  return push(interp, imm8(interp, h));
    case I_P0:    return push(interp, 0);
    case I_P1:    return push(interp, 1);
    case I_P2:    return push(interp, 2);
    case I_P3:    return push(interp, 3);
    case I_P4:    return push(interp, 4);
    case I_P5:    return push(interp, 5);
    case I_P6:    return push(interp, 6);
    case I_P7:    return push(interp, 7);
    case I_P8:    return push(interp, 8);
    case I_P16:   return push(interp, 16);
    case I_P32:   return push(interp, 32);
    case I_P64:   return push(interp, 64);
    case I_P128:  return push(interp, 128);
    case I_P00:   return push_zn(interp, 2);
    case I_P000:  return push_zn(interp, 3);
    case I_P0000: return push_zn(interp, 4);
    case I_PZ8:   return push_zn(interp, 8);
    case I_PZ16:  return push_zn(interp, 16);
    case I_PUSHG: return pushn(interp, h, 2);
    case I_PUSHL: return pushn(interp, h, 3);
    case I_PUSH4: return pushn(interp, h, 4);
    case I_SEXT:  return sextn(interp, 1);
    case I_SEXT2: return sextn(interp, 2);
    case I_SEXT3: return sextn(interp, 3);
    case I_DUP:   return push(interp, head(interp));
    case I_DUP2:  return push(interp, headn(interp, 2));
    case I_DUP3:  return push(interp, headn(interp, 3));
    case I_DUP4:  return push(interp, headn(interp, 4));
    case I_DUP5:  return push(interp, headn(interp, 5));
    case I_DUP6:  return push(interp, headn(interp, 6));
    case I_DUP7:  return push(interp, headn(interp, 7));
    case I_DUP8:  return push(interp, headn(interp, 8));
    case I_DUPW:  return dupw(interp, 1);
    case I_DUPW2: return dupw(interp, 2);
    case I_DUPW3: return dupw(interp, 3);
    case I_DUPW4: return dupw(interp, 4);
    case I_DUPW5: return dupw(interp, 5);
    case I_DUPW6: return dupw(interp, 6);
    case I_DUPW7: return dupw(interp, 7);
    case I_DUPW8: return dupw(interp, 8);
    case I_GETL:  return getln(interp, h, 1);
    case I_GETL2: return getln(interp, h, 2);
    case I_GETL4: return getln(interp, h, 4);
    case I_GETLN: return getln(interp, h, pop8(interp));
    case I_SETL:  return setln(interp, h, 1);
    case I_SETL2: return setln(interp, h, 2);
    case I_SETL4: return setln(interp, h, 4);
    case I_SETLN: return setln(interp, h, pop8(interp));
    case I_GETG:  return getgn(interp, h, 1);
    case I_GETG2: return getgn(interp, h, 2);
    case I_GETG4: return getgn(interp, h, 4);
    case I_GETGN: return getgn(interp, h, pop8(interp));
    case I_GTGB:  return gtgbn(interp, h, 1);
    case I_GTGB2: return gtgbn(interp, h, 2);
    case I_GTGB4: return gtgbn(interp, h, 4);
    case I_SETG:  return setgn(interp, h, 1);
    case I_SETG2: return setgn(interp, h, 2);
    case I_SETG4: return setgn(interp, h, 4);
    case I_SETGN: return setgn(interp, h, pop8(interp));
    case I_GETP:  return getpn(interp, h, 1);
    case I_GETPN: return getpn(interp, h, imm8(interp, h));
    case I_GETR:  return getrn(interp, 1);
    case I_GETR2: return getrn(interp, 2);
    case I_GETRN: return getrn(interp, imm8(interp, h));
    case I_SETR:  return setrn(interp, 1);
    case I_SETR2: return setrn(interp, 2);
    case I_SETRN: return setrn(interp, imm8(interp, h));
    case I_POP:   interp->sp -= 1; return ABC_RESULT_NORMAL;
    case I_POP2:  interp->sp -= 2; return ABC_RESULT_NORMAL;
    case I_POP3:  interp->sp -= 3; return ABC_RESULT_NORMAL;
    case I_POP4:  interp->sp -= 4; return ABC_RESULT_NORMAL;
    case I_POPN:  interp->sp -= imm8(interp, h); return ABC_RESULT_NORMAL;
    case I_AIXB1: goto unknown_instruction;
    case I_AIDXB: goto unknown_instruction;
    case I_AIDX:  goto unknown_instruction;
    case I_PIDXB: goto unknown_instruction;
    case I_UAIDX: goto unknown_instruction;
    case I_UPIDX: goto unknown_instruction;
    case I_ASLC:  goto unknown_instruction;
    case I_PSLC:  goto unknown_instruction;
    case I_REFL:  return push16(interp, 0x100 + interp->sp - imm8(interp, h));
    case I_REFGB: return push16(interp, 0x200 + imm8(interp, h));
    case I_INC:   interp->stack[(uint8_t)(interp->sp - 1)] += 1; return ABC_RESULT_NORMAL;
    case I_DEC:   interp->stack[(uint8_t)(interp->sp - 1)] -= 1; return ABC_RESULT_NORMAL;
    case I_LINC:  goto unknown_instruction;
    case I_PINC:  goto unknown_instruction;
    case I_PINC2: goto unknown_instruction;
    case I_PINC3: goto unknown_instruction;
    case I_PINC4: goto unknown_instruction;
    case I_PDEC:  goto unknown_instruction;
    case I_PDEC2: goto unknown_instruction;
    case I_PDEC3: goto unknown_instruction;
    case I_PDEC4: goto unknown_instruction;
    case I_PINCF: goto unknown_instruction;
    case I_PDECF: goto unknown_instruction;
    case I_ADD:   return add(interp);
    case I_ADD2:  return add2(interp);
    case I_ADD3:  return add3(interp);
    case I_ADD4:  return add4(interp);
    case I_SUB:   return sub(interp);
    case I_SUB2:  return sub2(interp);
    case I_SUB3:  return sub3(interp);
    case I_SUB4:  return sub4(interp);
    case I_ADD2B: return add2b(interp);
    case I_ADD3B: return add3b(interp);
    case I_SUB2B: return sub2b(interp);
    case I_MUL2B: return mul2b(interp);
    case I_MUL:   return mul(interp);
    case I_MUL2:  return mul2(interp);
    case I_MUL3:  return mul3(interp);
    case I_MUL4:  return mul4(interp);
    case I_UDIV2: return udiv2(interp);
    case I_UDIV4: return udiv4(interp);
    case I_DIV2:  return div2(interp);
    case I_DIV4:  return div4(interp);
    case I_UMOD2: return umod2(interp);
    case I_UMOD4: return umod4(interp);
    case I_MOD2:  return mod2(interp);
    case I_MOD4:  return mod4(interp);
    case I_LSL:   return lsl(interp);
    case I_LSL2:  return lsl2(interp);
    case I_LSL4:  return lsl4(interp);
    case I_LSR:   return lsr(interp);
    case I_LSR2:  return lsr2(interp);
    case I_LSR4:  return lsr4(interp);
    case I_ASR:   return asr(interp);
    case I_ASR2:  return asr2(interp);
    case I_ASR4:  return asr4(interp);
    case I_AND:   return bw_and(interp);
    case I_AND2:  return bw_and2(interp);
    case I_AND4:  return bw_and4(interp);
    case I_OR:    return bw_or(interp);
    case I_OR2:   return bw_or2(interp);
    case I_OR4:   return bw_or4(interp);
    case I_XOR:   return bw_xor(interp);
    case I_XOR2:  return bw_xor2(interp);
    case I_XOR4:  return bw_xor4(interp);
    case I_COMP:  return bw_comp(interp);
    case I_COMP2: return bw_comp2(interp);
    case I_COMP4: return bw_comp4(interp);
    case I_BOOL:  return logical_bool(interp);
    case I_BOOL2: return logical_bool2(interp);
    case I_BOOL3: return logical_bool3(interp);
    case I_BOOL4: return logical_bool4(interp);
    case I_CULT:  return cult(interp);
    case I_CULT2: return cult2(interp);
    case I_CULT3: return cult3(interp);
    case I_CULT4: return cult4(interp);
    case I_CSLT:  return cslt(interp);
    case I_CSLT2: return cslt2(interp);
    case I_CSLT3: return cslt3(interp);
    case I_CSLT4: return cslt4(interp);
    case I_CFEQ:  return cfeq(interp);
    case I_CFLT:  return cflt(interp);
    case I_NOT:   return logical_not(interp);
    case I_FADD:  return fadd(interp);
    case I_FSUB:  return fsub(interp);
    case I_FMUL:  return fmul(interp);
    case I_FDIV:  return fdiv(interp);
    case I_F2I:   return f2i(interp);
    case I_F2U:   return f2u(interp);
    case I_I2F:   return i2f(interp);
    case I_U2F:   return u2f(interp);
    case I_BZ:    goto unknown_instruction;
    case I_BZ1:   goto unknown_instruction;
    case I_BNZ:   goto unknown_instruction;
    case I_BNZ1:  goto unknown_instruction;
    case I_BZP:   goto unknown_instruction;
    case I_BZP1:  goto unknown_instruction;
    case I_BNZP:  goto unknown_instruction;
    case I_BNZP1: goto unknown_instruction;
    case I_JMP:   goto unknown_instruction;
    case I_JMP1:  goto unknown_instruction;
    case I_CALL:  goto unknown_instruction;
    case I_CALL1: goto unknown_instruction;
    case I_RET:   goto unknown_instruction;
    case I_SYS:   goto unknown_instruction;
    default:
    unknown_instruction:
        assert(0);
        return ABC_RESULT_ERROR;
    }
}
