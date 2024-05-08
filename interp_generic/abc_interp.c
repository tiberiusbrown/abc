#include "abc_interp.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define FONT_HEADER_PER_CHAR 7
#define FONT_HEADER_OFFSET 1
#define FONT_HEADER_BYTES (FONT_HEADER_PER_CHAR * 256 + FONT_HEADER_OFFSET)

enum
{
    SYS_DISPLAY,
    SYS_DISPLAY_NOCLEAR,
    SYS_GET_PIXEL,
    SYS_DRAW_PIXEL,
    SYS_DRAW_HLINE,
    SYS_DRAW_VLINE,
    SYS_DRAW_LINE,
    SYS_DRAW_RECT,
    SYS_DRAW_FILLED_RECT,
    SYS_DRAW_CIRCLE,
    SYS_DRAW_FILLED_CIRCLE,
    SYS_DRAW_SPRITE,
    SYS_DRAW_SPRITE_SELFMASK,
    SYS_DRAW_TEXT,
    SYS_DRAW_TEXT_P,
    SYS_DRAW_TEXTF,
    SYS_TEXT_WIDTH,
    SYS_TEXT_WIDTH_P,
    SYS_WRAP_TEXT,
    SYS_SET_TEXT_FONT,
    SYS_SET_TEXT_COLOR,
    SYS_SET_FRAME_RATE,
    SYS_NEXT_FRAME,
    SYS_IDLE,
    SYS_DEBUG_BREAK,
    SYS_DEBUG_PRINTF,
    SYS_ASSERT,
    SYS_POLL_BUTTONS,
    SYS_JUST_PRESSED,
    SYS_JUST_RELEASED,
    SYS_PRESSED,
    SYS_ANY_PRESSED,
    SYS_NOT_PRESSED,
    SYS_MILLIS,
    SYS_MEMCPY,
    SYS_MEMCPY_P,
    SYS_STRLEN,
    SYS_STRLEN_P,
    SYS_STRCMP,
    SYS_STRCMP_P,
    SYS_STRCMP_PP,
    SYS_STRCPY,
    SYS_STRCPY_P,
    SYS_FORMAT,
    SYS_MUSIC_PLAY,
    SYS_MUSIC_PLAYING,
    SYS_MUSIC_STOP,
    SYS_TONES_PLAY,
    SYS_TONES_PLAY_PRIMARY,
    SYS_TONES_PLAY_AUTO,
    SYS_TONES_PLAYING,
    SYS_TONES_STOP,
    SYS_AUDIO_ENABLED,
    SYS_AUDIO_TOGGLE,
    SYS_AUDIO_PLAYING,
    SYS_AUDIO_STOP,
    SYS_SAVE_EXISTS,
    SYS_SAVE,
    SYS_LOAD,
    SYS_SIN,
    SYS_COS,
    SYS_TAN,
    SYS_ATAN2,
    SYS_FLOOR,
    SYS_CEIL,
    SYS_ROUND,
    SYS_MOD,
    SYS_POW,
    SYS_SQRT,
    SYS_GENERATE_RANDOM_SEED,
    SYS_INIT_RANDOM_SEED,
    SYS_RANDOM,
    SYS_RANDOM_RANGE,
};

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

static uint8_t prog8(abc_host_t const* h, uint32_t addr)
{
    return h->prog(h->user, addr);
}

static uint16_t prog16(abc_host_t const* h, uint32_t addr)
{
    uint16_t t = 0;
    t += (h->prog(h->user, addr + 0) << 0);
    t += (h->prog(h->user, addr + 1) << 8);
    return t;
}

static uint32_t prog24(abc_host_t const* h, uint32_t addr)
{
    uint32_t t = 0;
    t += (h->prog(h->user, addr + 0) << 0);
    t += (h->prog(h->user, addr + 1) << 8);
    t += (h->prog(h->user, addr + 2) << 16);
    return t;
}

static uint32_t prog32(abc_host_t const* h, uint32_t addr)
{
    uint32_t t = 0;
    t += (h->prog(h->user, addr + 0) << 0);
    t += (h->prog(h->user, addr + 1) << 8);
    t += (h->prog(h->user, addr + 2) << 16);
    t += (h->prog(h->user, addr + 3) << 24);
    return t;
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
        (void)push(interp, interp->globals[(t + i - 0x200) & 1023]);
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
        interp->globals[(t - i - 0x200) & 1023] = interp->stack[--interp->sp];
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

static uint8_t* refptr(abc_interp_t* interp, uint16_t t)
{
    if(t < 0x100 || t >= 0x600) return NULL;
    if(t < 0x200)
    {
        /* stack reference */
        return &interp->stack[t - 0x100];
    }
    else
    {
        /* globals reference */
        return &interp->globals[t - 0x200];
    }
}

static abc_result_t getrn(abc_interp_t* interp, uint8_t n)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr || !space(interp, n)) return ABC_RESULT_ERROR;
    for(uint8_t i = 0; i < n; ++i)
        (void)push(interp, ptr[i]);
    return ABC_RESULT_NORMAL;
}

static abc_result_t setrn(abc_interp_t* interp, uint8_t n)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr) return ABC_RESULT_ERROR;
    for(uint8_t i = 0; i < n; ++i)
        ptr[n - 1 - i] = pop8(interp);
    return ABC_RESULT_NORMAL;
}

static abc_result_t pinc(abc_interp_t* interp, int8_t offset)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr || !space(interp, 3)) return ABC_RESULT_ERROR;
    uint8_t x = 0;
    x = (x << 8) + ptr[0];
    uint8_t y = x + offset;
    ptr[0] = (uint8_t)(y >> 0);
    push(interp, x);
    return ABC_RESULT_NORMAL;
}

static abc_result_t pinc2(abc_interp_t* interp, int8_t offset)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr || !space(interp, 4)) return ABC_RESULT_ERROR;
    uint16_t x = 0;
    x = (x << 8) + ptr[1];
    x = (x << 8) + ptr[0];
    uint16_t y = x + offset;
    ptr[0] = (uint8_t)(y >> 0);
    ptr[1] = (uint8_t)(y >> 8);
    push16(interp, x);
    return ABC_RESULT_NORMAL;
}

static abc_result_t pinc3(abc_interp_t* interp, int8_t offset)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr || !space(interp, 5)) return ABC_RESULT_ERROR;
    uint32_t x = 0;
    x = (x << 8) + ptr[2];
    x = (x << 8) + ptr[1];
    x = (x << 8) + ptr[0];
    uint32_t y = x + offset;
    ptr[0] = (uint8_t)(y >> 0);
    ptr[1] = (uint8_t)(y >> 8);
    ptr[2] = (uint8_t)(y >> 16);
    push24(interp, x);
    return ABC_RESULT_NORMAL;
}

static abc_result_t pinc4(abc_interp_t* interp, int8_t offset)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr || !space(interp, 6)) return ABC_RESULT_ERROR;
    uint32_t x = 0;
    x = (x << 8) + ptr[3];
    x = (x << 8) + ptr[2];
    x = (x << 8) + ptr[1];
    x = (x << 8) + ptr[0];
    uint32_t y = x + offset;
    ptr[0] = (uint8_t)(y >> 0);
    ptr[1] = (uint8_t)(y >> 8);
    ptr[2] = (uint8_t)(y >> 16);
    ptr[3] = (uint8_t)(y >> 24);
    push32(interp, x);
    return ABC_RESULT_NORMAL;
}

static abc_result_t pincf(abc_interp_t* interp, float offset)
{
    uint16_t t = pop16(interp);
    uint8_t* ptr = refptr(interp, t);
    if(!ptr || !space(interp, 6)) return ABC_RESULT_ERROR;
    union { float f; uint32_t x; } u, uinc;
    u.x = 0;
    u.x = (u.x << 8) + ptr[3];
    u.x = (u.x << 8) + ptr[2];
    u.x = (u.x << 8) + ptr[1];
    u.x = (u.x << 8) + ptr[0];
    uinc.x = u.x;
    uinc.f += offset;
    ptr[0] = (uint8_t)(uinc.x >> 0);
    ptr[1] = (uint8_t)(uinc.x >> 8);
    ptr[2] = (uint8_t)(uinc.x >> 16);
    ptr[3] = (uint8_t)(uinc.x >> 24);
    push32(interp, u.x);
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

static abc_result_t aixb1(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t n = imm8(interp, h);
    uint8_t i = pop8(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint16_t p = pop16(interp);
    return push16(interp, p + i);
}

static abc_result_t aidxb(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t b = imm8(interp, h);
    uint8_t n = imm8(interp, h);
    uint8_t i = pop8(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint16_t p = pop16(interp);
    return push16(interp, p + i * b);
}

static abc_result_t aidx(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t b = imm16(interp, h);
    uint16_t n = imm16(interp, h);
    uint16_t i = pop16(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint16_t p = pop16(interp);
    return push16(interp, p + i * b);
}

static abc_result_t pidxb(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t b = imm8(interp, h);
    uint8_t n = imm8(interp, h);
    uint8_t i = pop8(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint32_t p = pop24(interp);
    return push24(interp, p + i * b);
}

static abc_result_t pidx(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t b = imm16(interp, h);
    uint32_t n = imm24(interp, h);
    uint32_t i = pop24(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint32_t p = pop24(interp);
    return push24(interp, p + i * b);
}

static abc_result_t uaidx(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t b = imm16(interp, h);
    uint16_t i = pop16(interp);
    uint16_t n = pop16(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint16_t p = pop16(interp);
    return push16(interp, p + i * b);
}

static abc_result_t upidx(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t b = imm16(interp, h);
    uint32_t i = pop24(interp);
    uint32_t n = pop24(interp);
    if(i >= n) return ABC_RESULT_ERROR;
    uint32_t p = pop24(interp);
    return push24(interp, p + i * b);
}

static abc_result_t aslc(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t stop = pop16(interp);
    uint16_t start = pop16(interp);
    uint16_t n = pop16(interp);
    uint16_t p = pop16(interp);
    uint16_t b = imm16(interp, h);
    if(start >= n || stop > n) return ABC_RESULT_ERROR;
    push16(interp, p + start * b);
    return push16(interp, stop - start);
}

static abc_result_t pslc(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t stop = pop24(interp);
    uint32_t start = pop24(interp);
    uint32_t n = pop24(interp);
    uint32_t p = pop24(interp);
    uint16_t b = imm16(interp, h);
    if(start >= n || stop > n) return ABC_RESULT_ERROR;
    push24(interp, p + start * b);
    return push24(interp, stop - start);
}

static abc_result_t bz(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t addr = imm24(interp, h);
    if(pop8(interp) == 0)
        interp->pc = addr;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bz1(abc_interp_t* interp, abc_host_t const* h)
{
    int8_t offset = (int8_t)imm8(interp, h);
    if(pop8(interp) == 0)
        interp->pc += offset;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bnz(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t addr = imm24(interp, h);
    if(pop8(interp) != 0)
        interp->pc = addr;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bnz1(abc_interp_t* interp, abc_host_t const* h)
{
    int8_t offset = (int8_t)imm8(interp, h);
    if(pop8(interp) != 0)
        interp->pc += offset;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bzp(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t addr = imm24(interp, h);
    if(pop8(interp) == 0)
        interp->pc = addr, interp->sp += 1;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bzp1(abc_interp_t* interp, abc_host_t const* h)
{
    int8_t offset = (int8_t)imm8(interp, h);
    if(pop8(interp) == 0)
        interp->pc += offset, interp->sp += 1;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bnzp(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t addr = imm24(interp, h);
    if(pop8(interp) != 0)
        interp->pc = addr, interp->sp += 1;
    return ABC_RESULT_NORMAL;
}

static abc_result_t bnzp1(abc_interp_t* interp, abc_host_t const* h)
{
    int8_t offset = (int8_t)imm8(interp, h);
    if(pop8(interp) != 0)
        interp->pc += offset, interp->sp += 1;
    return ABC_RESULT_NORMAL;
}

static abc_result_t linc(abc_interp_t* interp, uint8_t n, int8_t x)
{
    interp->stack[(uint8_t)(interp->sp - n)] += x;
    return ABC_RESULT_NORMAL;
}

static abc_result_t call(abc_interp_t* interp, uint32_t addr)
{
    if(interp->csp >= sizeof(interp->call_stack) / sizeof(interp->call_stack[0]))
        return ABC_RESULT_ERROR;
    interp->call_stack[interp->csp++] = interp->pc;
    interp->pc = addr;
    return ABC_RESULT_NORMAL;
}

static abc_result_t ret(abc_interp_t* interp)
{
    if(interp->csp == 0) return ABC_RESULT_ERROR;
    interp->pc = interp->call_stack[--interp->csp];
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_assert(abc_interp_t* interp)
{
    if(pop8(interp) == 0) return ABC_RESULT_ERROR;
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_memcpy(abc_interp_t* interp)
{
    uint16_t n0 = pop16(interp);
    uint16_t b0 = pop16(interp);
    uint16_t n1 = pop16(interp);
    uint16_t b1 = pop16(interp);
    uint8_t* p0 = refptr(interp, b0);
    uint8_t* p1 = refptr(interp, b1);
    if(n0 != n1 || !p0 || !p1) return ABC_RESULT_ERROR;
    if(!refptr(interp, b0 + n0 - 1)) return ABC_RESULT_ERROR;
    if(!refptr(interp, b1 + n1 - 1)) return ABC_RESULT_ERROR;
    memcpy(p0, p1, n0);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_memcpy_P(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t n0 = pop16(interp);
    uint16_t b0 = pop16(interp);
    uint32_t n1 = pop24(interp);
    uint32_t b1 = pop24(interp);
    uint8_t* p0 = refptr(interp, b0);
    if(n0 != n1 || !p0) return ABC_RESULT_ERROR;
    if(!refptr(interp, b0 + n0 - 1)) return ABC_RESULT_ERROR;
    for(uint16_t n = 0; n < n0; ++n)
        p0[n] = h->prog(h->user, b1 + n);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_strcmp(abc_interp_t* interp)
{
    uint16_t n0 = pop16(interp);
    uint16_t b0 = pop16(interp);
    uint16_t n1 = pop16(interp);
    uint16_t b1 = pop16(interp);
    uint8_t* p0 = refptr(interp, b0);
    uint8_t* p1 = refptr(interp, b1);
    if(!p0 || !p1) return ABC_RESULT_ERROR;
    if(!refptr(interp, b0 + n0 - 1)) return ABC_RESULT_ERROR;
    if(!refptr(interp, b1 + n1 - 1)) return ABC_RESULT_ERROR;
    uint8_t c0, c1;
    for(;;)
    {
        c0 = *p0++;
        c1 = *p1++;
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    return push(interp, c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
}

static abc_result_t sys_strcmp_P(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t n0 = pop16(interp);
    uint16_t b0 = pop16(interp);
    uint32_t n1 = pop24(interp);
    uint32_t b1 = pop24(interp);
    uint8_t* p0 = refptr(interp, b0);
    if(!p0) return ABC_RESULT_ERROR;
    if(!refptr(interp, b0 + n0 - 1)) return ABC_RESULT_ERROR;
    uint8_t c0, c1;
    for(;;)
    {
        c0 = *p0++;
        c1 = h->prog(h->user, b1++);
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    return push(interp, c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
}

static abc_result_t sys_strcmp_PP(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t n0 = pop24(interp);
    uint32_t b0 = pop24(interp);
    uint32_t n1 = pop24(interp);
    uint32_t b1 = pop24(interp);
    uint8_t c0, c1;
    for(;;)
    {
        c0 = h->prog(h->user, b0++);
        c1 = h->prog(h->user, b1++);
        if(n0 == 0) c0 = '\0'; else --n0;
        if(n1 == 0) c1 = '\0'; else --n1;
        if(c1 == '\0') break;
        if(c0 != c1) break;
    }
    return push(interp, c0 < c1 ? -1 : c1 < c0 ? 1 : 0);
}

static abc_result_t sys_strcpy(abc_interp_t* interp)
{
    uint16_t n0 = pop16(interp);
    uint16_t b0 = pop16(interp);
    uint16_t n1 = pop16(interp);
    uint16_t b1 = pop16(interp);
    uint8_t* p0 = refptr(interp, b0);
    uint8_t* p1 = refptr(interp, b1);
    uint16_t nr = n0;
    uint16_t br = b0;
    if(!p0 || !p1) return ABC_RESULT_ERROR;
    if(!refptr(interp, b0 + n0 - 1)) return ABC_RESULT_ERROR;
    if(!refptr(interp, b1 + n1 - 1)) return ABC_RESULT_ERROR;
    if(n0 != 0)
    {
        for(;;)
        {
            uint8_t c = *p1++;
            *p0++ = c;
            if(c == 0) break;
            if(--n0 == 0) break;
            if(--n1 == 0) { *p0++ = 0; break; }
        }
    }
    push16(interp, br);
    push16(interp, nr);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_strcpy_P(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t n0 = pop16(interp);
    uint16_t b0 = pop16(interp);
    uint32_t n1 = pop24(interp);
    uint32_t b1 = pop24(interp);
    uint8_t* p0 = refptr(interp, b0);
    uint16_t nr = n0;
    uint16_t br = b0;
    if(!p0) return ABC_RESULT_ERROR;
    if(!refptr(interp, b0 + n0 - 1)) return ABC_RESULT_ERROR;
    if(n0 != 0)
    {
        for(;;)
        {
            uint8_t c = h->prog(h->user, b1++);
            *p0++ = c;
            if(c == 0) break;
            if(--n0 == 0) break;
            if(--n1 == 0) { *p0++ = 0; break; }
        }
    }
    push16(interp, br);
    push16(interp, nr);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_strlen(abc_interp_t* interp)
{
    uint16_t n = pop16(interp);
    uint16_t b = pop16(interp);
    uint8_t* p = refptr(interp, b);
    if(!p) return ABC_RESULT_ERROR;
    if(!refptr(interp, b + n - 1)) return ABC_RESULT_ERROR;
    uint16_t t = 0;
    if(n != 0)
    {
        while(*p++ != '\0')
        {
            ++t;
            if(--n == 0) break;
        }
    }
    return push16(interp, t);
}

static abc_result_t sys_strlen_P(abc_interp_t* interp, abc_host_t const* h)
{
    uint32_t n = pop24(interp);
    uint32_t b = pop24(interp);
    uint32_t t = 0;
    if(n != 0)
    {
        while(h->prog(h->user, b++) != '\0')
        {
            ++t;
            if(--n == 0) break;
        }
    }
    return push24(interp, t);
}

static uint16_t save_size(abc_host_t const* h)
{
    uint16_t n = h->prog(h->user, 10) + 256 * h->prog(h->user, 11);
    return n > 1024 ? 0 : n;
}

static abc_result_t sys_save_exists(abc_interp_t* interp, abc_host_t const* h)
{
    return push(interp, interp->has_save && save_size(h) != 0 ? 1 : 0);
}

static abc_result_t sys_save(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t n = save_size(h);
    if(n != 0)
    {
        memcpy(interp->saved, interp->globals, n);
        interp->has_save = 1;
    }
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_load(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t n = save_size(h);
    uint8_t has_save = interp->has_save && n != 0;
    if(has_save)
        memcpy(interp->globals, interp->saved, n);
    return push(interp, has_save ? 1 : 0);
}

static void format_add_int(
    void(*f)(void* u, char c), void* u,
    uint32_t x, bool sign, uint8_t base, int8_t w)
{
    char buf[10];
    char* tp = buf;

    if(sign && (int32_t)x < 0)
    {
        f(u, '-');
        x = (uint32_t)(-(int32_t)x);
        --w;
    }

    if(x == 0)
    {
        *tp++ = '0';
        --w;
    }
    while(x != 0)
    {
        uint8_t r = x % base;
        x /= base;
        *tp++ = (char)(r + (r < 10 ? '0' : 'a' - 10));
        --w;
    }

    while(w > 0)
    {
        *tp++ = '0';
        --w;
    }

    while(tp != buf)
        f(u, *--tp);
}

static void format_add_float(
    void(*f)(void* u, char c), void* u, float x, uint8_t prec)
{
    if(isnan(x))
    {
        f(u, 'n');
        f(u, 'a');
        f(u, 'n');
        return;
    }
    if(isinf(x))
    {
        f(u, 'i');
        f(u, 'n');
        f(u, 'f');
        return;
    }
    if(x > 4294967040.f || x < -4294967040.f)
    {
        f(u, 'o');
        f(u, 'v');
        f(u, 'f');
        return;
    }

    if(x < 0.0f)
    {
        f(u, '-');
        x = -x;
    }

    {
        float r = 0.5f;
        for(uint8_t i = 0; i < prec; ++i)
            r *= 0.1f;
        x += r;
    }

    uint32_t n = (uint32_t)x;
    float r = x - (float)n;
    format_add_int(f, u, n, false, 10, 0);

    if(prec > 0)
        f(u, '.');
    while((int8_t)(--prec) >= 0)
    {
        r *= 10.f;
        uint8_t t = (uint8_t)r;
        f(u, (char)('0' + t));
        r -= t;
    }
}

static void format_exec(
    abc_interp_t* interp, abc_host_t const* h,
    void(*f)(void* u, char c), void* u)
{
    uint32_t fn = pop24(interp);
    uint32_t fb = pop24(interp);

    while(fn != 0)
    {
        char c = (char)h->prog(h->user, fb++);
        --fn;
        if(c != '%')
        {
            f(u, c);
            continue;
        }
        c = (char)h->prog(h->user, fb++);
        --fn;
        switch(c)
        {
        case 'c':
            f(u, (char)pop8(interp));
            break;
        case '%':
            f(u, c);
            break;
        case 's':
        {
            uint16_t tn = pop16(interp);
            uint16_t tb = pop16(interp);
            uint8_t* p = refptr(interp, tb);
            if(!p) break;
            while(tn != 0)
            {
                uint8_t tc = *p++;
                if(tc == '\0') break;
                f(u, (char)tc);
                --tn;
            }
            break;
        }
        case 'S':
        {
            uint32_t tn = pop24(interp);
            uint32_t tb = pop24(interp);
            while(tn != 0)
            {
                uint8_t tc = h->prog(h->user, tb++);
                if(tc == '\0') break;
                f(u, (char)tc);
                --tn;
            }
            break;
        }
        case 'd':
        case 'u':
        case 'x':
        {
            uint32_t x = pop32(interp);
            int8_t w = (int8_t)(h->prog(h->user, fb++) - '0');
            --fn;
            format_add_int(f, u, x, c == 'd', c == 'x' ? 16 : 10, w);
            break;
        }
        case 'f':
        {
            float x = popf(interp);
            uint8_t prec = h->prog(h->user, fb++) - '0';
            --fn;
            format_add_float(f, u, x, prec);
            break;
        }
        default:
            break;
        }
    }

}

typedef struct
{
    char* p;
    uint16_t n;
} format_user_buffer;

static void format_exec_to_buffer(void* user, char c)
{
    format_user_buffer* u = (format_user_buffer*)user;
    uint16_t n = u->n;
    if(n == 0) return;
    u->n = --n;
    *(u->p)++ = (uint8_t)c;
}

static abc_result_t sys_format(abc_interp_t* interp, abc_host_t const* h)
{
    uint16_t n = pop16(interp);
    uint16_t b = pop16(interp);
    format_user_buffer u;
    u.p = (char*)refptr(interp, b);
    if(!u.p || !refptr(interp, b + n - 1)) return ABC_RESULT_ERROR;
    u.n = n;
    format_exec(interp, h, format_exec_to_buffer, &u);
    if(u.n != 0)
        *u.p = '\0';
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_display(abc_interp_t* interp)
{
    memcpy(interp->display, interp->display_buffer, sizeof(interp->display));
    memset(interp->display_buffer, 0, sizeof(interp->display_buffer));
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_display_noclear(abc_interp_t* interp)
{
    memcpy(interp->display, interp->display_buffer, sizeof(interp->display));
    return ABC_RESULT_NORMAL;
}

static uint8_t get_pixel_helper(abc_interp_t* interp, uint8_t x, uint8_t y)
{
    uint8_t row = y >> 3;
    uint8_t bit = y & 7;
    if(x < 128 && y < 64)
        return (interp->display_buffer[row * 128 + x] & (1 << bit)) ? 1 : 0;
    return 0;
}

static abc_result_t sys_get_pixel(abc_interp_t* interp)
{
    uint8_t y = pop8(interp);
    uint8_t x = pop8(interp);
    return push(interp, get_pixel_helper(interp, x, y));
}

static abc_result_t sys_set_frame_rate(abc_interp_t* interp)
{
    uint8_t fr = pop8(interp);
    interp->frame_dur = fr >= 4 ? (uint8_t)(1000u / fr) : 255u;
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_next_frame(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t now = (uint8_t)h->millis(h->user);
    uint8_t frame_duration = now - interp->frame_start;
    if(frame_duration < interp->frame_dur)
    {
        if(ABC_RESULT_ERROR == push(interp, 0))
            return ABC_RESULT_ERROR;
        return ABC_RESULT_IDLE;
    }

    interp->frame_start += interp->frame_dur;
    return push(interp, 1);
}

static abc_result_t sys_audio_enabled(abc_interp_t* interp)
{
    return push(interp, interp->audio_enabled != 0 ? 1 : 0);
}

static abc_result_t sys_audio_toggle(abc_interp_t* interp)
{
    interp->audio_enabled = !interp->audio_enabled;
    return ABC_RESULT_NORMAL;
}

static uint8_t host_buttons(abc_host_t const* h)
{
    return h->buttons ? h->buttons(h->user) : 0;
}

static abc_result_t sys_poll_buttons(abc_interp_t* interp, abc_host_t const* h)
{
    interp->buttons_prev = interp->buttons_curr;
    interp->buttons_curr = host_buttons(h);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_just_pressed(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    return push(interp,
        (b & interp->buttons_curr) && !(b & interp->buttons_prev) ? 1 : 0);
}

static abc_result_t sys_just_released(abc_interp_t* interp)
{
    uint8_t b = pop8(interp);
    return push(interp,
        !(b & interp->buttons_curr) && (b & interp->buttons_prev) ? 1 : 0);
}

static abc_result_t sys_pressed(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t b = pop8(interp);
    return push(interp, (host_buttons(h) & b) == b ? 1 : 0);
}

static abc_result_t sys_any_pressed(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t b = pop8(interp);
    return push(interp, (host_buttons(h) & b) != 0 ? 1 : 0);
}

static abc_result_t sys_not_pressed(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t b = pop8(interp);
    return push(interp, (host_buttons(h) & b) == 0 ? 1 : 0);
}

static abc_result_t sys_millis(abc_interp_t* interp, abc_host_t const* h)
{
    return push32(interp, h->millis ? h->millis(h->user) : 0u);
}

static void draw_pixel_helper(abc_interp_t* interp, int16_t x, int16_t y, uint8_t c)
{
    if((uint16_t)x < 128 && (uint16_t)y < 64)
    {
        uint8_t* p = &interp->display_buffer[((uint16_t)y >> 3) * 128 + (uint16_t)x];
        uint8_t m = 1 << ((uint16_t)y & 7);
        if(c != 0)
            *p |= m;
        else
            *p &= ~m;
    }
}

static abc_result_t sys_draw_pixel(abc_interp_t* interp)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint8_t c = pop8(interp);
    draw_pixel_helper(interp, x, y, c);
    return ABC_RESULT_NORMAL;
}

static void draw_filled_rect_helper(abc_interp_t* interp,
    int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c)
{
    int16_t tx = x + w;
    int16_t ty = y + h;
    if(x >= 128 || y >= 64 || tx <= 0 || ty <= 0)
        return;
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    if(tx > 128) tx = 128;
    if(ty > 64) ty = 64;

    // TODO: optimize
    for(int16_t iy = y; iy < ty; ++iy)
        for(int16_t ix = x; ix < tx; ++ix)
            draw_pixel_helper(interp, ix, iy, c);
}

static abc_result_t sys_draw_filled_rect(abc_interp_t* interp)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint8_t w = pop8(interp);
    uint8_t h = pop8(interp);
    uint8_t c = pop8(interp);
    draw_filled_rect_helper(interp, x, y, w, h, c);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_draw_rect(abc_interp_t* interp)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint8_t w = pop8(interp);
    uint8_t h = pop8(interp);
    uint8_t c = pop8(interp);
    draw_filled_rect_helper(interp, x, y, w, 1, c);
    draw_filled_rect_helper(interp, x, y, 1, h, c);
    draw_filled_rect_helper(interp, x, y + h - 1, w, 1, c);
    draw_filled_rect_helper(interp, x + w - 1, y, 1, h, c);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_draw_hline(abc_interp_t* interp)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint8_t w = pop8(interp);
    uint8_t c = pop8(interp);
    draw_filled_rect_helper(interp, x, y, w, 1, c);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_draw_vline(abc_interp_t* interp)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint8_t h = pop8(interp);
    uint8_t c = pop8(interp);
    draw_filled_rect_helper(interp, x, y, 1, h, c);
    return ABC_RESULT_NORMAL;
}

/*
    Modes
    ==================
    0  Overwrite
    1  Masked
    2  Self Masked
    3  Self Masked Erase
*/
static void draw_sprite_helper(
    abc_interp_t* interp, abc_host_t const* host,
    uint32_t image, int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t mode)
{
    // TODO: optimize
    (void)host;
    (void)image;
    (void)mode;
    uint32_t stride = w;
    if(mode == 1) stride *= 2;
    int16_t tx = x + w;
    int16_t ty = y + h;
    if(x >= 128 || y >= 64 || tx <= 0 || ty <= 0) return;
    uint16_t ibx = 0;
    uint16_t iy = 0;
    if(x < 0)
    {
        ibx = (uint16_t)(-x);
        x = 0;
    }
    if(y < 0)
    {
        iy = (uint16_t)(-y);
        y = 0;
    }
    if(tx > 128) tx = 128;
    if(ty > 64) ty = 64;
    for(int16_t r = y; r < ty; ++r, ++iy)
    {
        uint16_t ix = ibx;
        for(int16_t c = x; c < tx; ++c, ++ix)
        {
            uint32_t offset = w * (iy >> 3) + ix;
            if(mode == 1) offset *= 2;
            uint32_t addr = image + offset;
            uint8_t ip = (prog8(host, addr + 0) & (1 << (iy & 7))) ? 1 : 0;
            uint8_t im = (prog8(host, addr + 1) & (1 << (iy & 7))) ? 1 : 0;
            uint8_t* bptr = &interp->display_buffer[((uint16_t)r >> 3) * 128 + c];
            uint8_t m = 1 << ((uint16_t)r & 7);
            switch(mode)
            {
            case 0:
                if(ip) *bptr |= m;
                else   *bptr &= ~m;
                break;
            case 1:
                if(!im) break;
                if(ip) *bptr |= m;
                else   *bptr &= ~m;
                break;
            case 2:
                if(ip) *bptr |= m;
                break;
            case 3:
                if(ip) *bptr &= ~m;
                break;
            default:
                break;
            }
        }
    }
}

static abc_result_t sys_draw_sprite_selfmask(abc_interp_t* interp, abc_host_t const* h)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint32_t image = pop24(interp);
    uint16_t frame = pop16(interp);

    uint8_t iw = prog8(h, image + 0);
    uint8_t ih = prog8(h, image + 1);
    uint8_t masked = prog8(h, image + 2);
    uint16_t num = prog16(h, image + 3);

    if(frame >= num) return ABC_RESULT_ERROR;

    image += 5;
    uint8_t pages = (ih + 7) >> 3;
    uint32_t offset = (uint32_t)pages * iw;
    if(masked) offset *= 2;
    image += offset * frame;

    draw_sprite_helper(interp, h, image, x, y, iw, ih, masked ? 1 : 2);

    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_draw_sprite(abc_interp_t* interp, abc_host_t const* h)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint32_t image = pop24(interp);
    uint16_t frame = pop16(interp);

    uint8_t iw = prog8(h, image + 0);
    uint8_t ih = prog8(h, image + 1);
    uint8_t masked = prog8(h, image + 2);
    uint16_t num = prog16(h, image + 3);

    if(frame >= num) return ABC_RESULT_ERROR;

    image += 5;
    uint8_t pages = (ih + 7) >> 3;
    uint32_t offset = (uint32_t)pages * iw;
    if(masked) offset *= 2;
    image += offset * frame;

    draw_sprite_helper(interp, h, image, x, y, iw, ih, masked ? 1 : 0);

    return ABC_RESULT_NORMAL;
}

static void draw_fast_vline(
    abc_interp_t* interp,
    int16_t x, int16_t y, uint16_t h, uint8_t color)
{
    draw_filled_rect_helper(interp, x, y, 1, (uint8_t)h, color);
}

/* adapted from Arduboy2 library (BSD 3 - clause) */
static void fill_circle_helper(
    abc_interp_t* interp,
    int16_t x0, int16_t y0, uint8_t r,
    uint8_t sides, int16_t delta, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while(x < y)
    {
        if(f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        if(sides & 0x1) /* right side */
        {
            draw_fast_vline(interp, x0 + x, y0 - y, 2 * y + 1 + delta, color);
            draw_fast_vline(interp, x0 + y, y0 - x, 2 * x + 1 + delta, color);
        }

        if(sides & 0x2) /* left side */
        {
            draw_fast_vline(interp, x0 - x, y0 - y, 2 * y + 1 + delta, color);
            draw_fast_vline(interp, x0 - y, y0 - x, 2 * x + 1 + delta, color);
        }
    }
}

static abc_result_t sys_draw_filled_circle(abc_interp_t* interp)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint8_t r = pop8(interp);
    uint8_t c = pop8(interp);
    draw_fast_vline(interp, x, y - r, 2 * r + 1, c);
    fill_circle_helper(interp, x, y, r, 3, 0, c);
    return ABC_RESULT_NORMAL;
}

/* adapted from Arduboy2 library (BSD 3 - clause) */
static abc_result_t sys_draw_circle(abc_interp_t* interp)
{
    int16_t x0    = (int16_t)pop16(interp);
    int16_t y0    = (int16_t)pop16(interp);
    uint8_t r     = pop8(interp);
    uint8_t color = pop8(interp);

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    draw_pixel_helper(interp, x0, y0 + r, color);
    draw_pixel_helper(interp, x0, y0 - r, color);
    draw_pixel_helper(interp, x0 + r, y0, color);
    draw_pixel_helper(interp, x0 - r, y0, color);

    while(x < y)
    {
        if(f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }

        x++;
        ddF_x += 2;
        f += ddF_x;

        draw_pixel_helper(interp, x0 + x, y0 + y, color);
        draw_pixel_helper(interp, x0 - x, y0 + y, color);
        draw_pixel_helper(interp, x0 + x, y0 - y, color);
        draw_pixel_helper(interp, x0 - x, y0 - y, color);
        draw_pixel_helper(interp, x0 + y, y0 + x, color);
        draw_pixel_helper(interp, x0 - y, y0 + x, color);
        draw_pixel_helper(interp, x0 + y, y0 - x, color);
        draw_pixel_helper(interp, x0 - y, y0 - x, color);
    }

    return ABC_RESULT_NORMAL;
}

/* adapted from Arduboy2 library (BSD 3 - clause) */
static void draw_line_helper(
    abc_interp_t* interp,
    int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if(steep) {
        int16_t t;
        t = x0;
        x0 = y0;
        y0 = t;
        t = x1;
        x1 = y1;
        y1 = t;
    }

    if(x0 > x1) {
        int16_t t;
        t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = (int16_t)abs(y1 - y0);

    int16_t err = dx / 2;
    int8_t ystep;

    if(y0 < y1)
    {
        ystep = 1;
    }
    else
    {
        ystep = -1;
    }

    for(; x0 <= x1; x0++)
    {
        if(steep)
        {
            draw_pixel_helper(interp, y0, x0, color);
        }
        else
        {
            draw_pixel_helper(interp, x0, y0, color);
        }

        err -= dy;
        if(err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

static abc_result_t sys_draw_line(abc_interp_t* interp)
{
    int16_t x0 = (int16_t)pop16(interp);
    int16_t y0 = (int16_t)pop16(interp);
    int16_t x1 = (int16_t)pop16(interp);
    int16_t y1 = (int16_t)pop16(interp);
    uint8_t c = pop8(interp);
    draw_line_helper(interp, x0, y0, x1, y1, c);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_set_text_font(abc_interp_t* interp)
{
    interp->text_font = pop24(interp);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_set_text_color(abc_interp_t* interp)
{
    interp->text_color = pop8(interp) != 0 ? 1 : 0;
    return ABC_RESULT_NORMAL;
}

static uint8_t font_get_line_height(abc_interp_t* interp, abc_host_t const* host)
{
    return prog8(host, interp->text_font + FONT_HEADER_PER_CHAR * 256);
}

static uint8_t font_get_x_advance(abc_interp_t* interp, abc_host_t const* host, char c)
{
    return prog8(host, interp->text_font + FONT_HEADER_PER_CHAR * (uint8_t)c + 0);
}

static uint8_t draw_char_helper(
    abc_interp_t* interp, abc_host_t const* host,
    int16_t x, int16_t y, char c)
{
    uint32_t font = interp->text_font;
    uint32_t glyph = font + (uint8_t)c * FONT_HEADER_PER_CHAR;
    uint8_t xadv = prog8(host, glyph + 0);
    int8_t xoff = (int8_t)prog8(host, glyph + 1);
    int8_t yoff = (int8_t)prog8(host, glyph + 2);
    uint16_t offset = prog16(host, glyph + 3);
    uint8_t w = prog8(host, glyph + 5);
    uint8_t h = prog8(host, glyph + 6);
    uint32_t addr = font + FONT_HEADER_BYTES + offset;
    draw_sprite_helper(
        interp, host, addr,
        x + xoff, y + yoff, w, h,
        interp->text_color != 0 ? 2 : 3);
    return xadv;
}

static abc_result_t sys_draw_text(abc_interp_t* interp, abc_host_t const* host)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint16_t tn = pop16(interp);
    uint16_t tb = pop16(interp);
    if(interp->text_font >= 0xff000000) return ABC_RESULT_ERROR;
    uint8_t line_height = font_get_line_height(interp, host);
    int16_t bx = x;

    char c;
    while(tn != 0)
    {
        uint8_t* ptr = refptr(interp, tb++);
        if(!ptr) return ABC_RESULT_ERROR;
        c = *ptr;
        if(c == '\0') break;
        --tn;
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }

        x += draw_char_helper(interp, host, x, y, c);
    }

    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_draw_text_P(abc_interp_t* interp, abc_host_t const* host)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);
    uint32_t tn = pop24(interp);
    uint32_t tb = pop24(interp);
    if(interp->text_font >= 0xff000000) return ABC_RESULT_ERROR;
    uint8_t line_height = font_get_line_height(interp, host);
    int16_t bx = x;

    char c;
    while(tn != 0)
    {
        c = (char)prog8(host, tb++);
        if(c == '\0') break;
        --tn;
        if(c == '\n')
        {
            x = bx;
            y += line_height;
            continue;
        }

        x += draw_char_helper(interp, host, x, y, c);
    }

    return ABC_RESULT_NORMAL;
}

typedef struct
{
    abc_interp_t* interp;
    abc_host_t const* host;
    int16_t bx;
    int16_t x;
    int16_t y;
    uint8_t line_height;
} format_user_draw;

static void format_exec_draw(void* user, char c)
{
    format_user_draw* u = (format_user_draw*)user;
    u->x += draw_char_helper(u->interp, u->host, u->x, u->y, c);
}

static abc_result_t sys_draw_textf(abc_interp_t* interp, abc_host_t const* host)
{
    int16_t x = (int16_t)pop16(interp);
    int16_t y = (int16_t)pop16(interp);

    format_user_draw u;
    u.interp = interp;
    u.host = host;
    u.bx = x;
    u.x = x;
    u.y = y;
    u.line_height = font_get_line_height(interp, host);

    format_exec(interp, host, format_exec_draw, &u);

    return ABC_RESULT_NORMAL;
}

typedef struct
{
    abc_host_t const* host;
} format_user_debug;

static void format_exec_debug(void* user, char c)
{
    format_user_debug* u = (format_user_debug*)user;
    abc_host_t const* host = u->host;
    if(host->debug_putc)
        host->debug_putc(host->user, c);
}

static abc_result_t sys_debug_printf(abc_interp_t* interp, abc_host_t const* host)
{
    format_user_debug u;
    u.host = host;
    format_exec(interp, host, format_exec_debug, &u);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_text_width(abc_interp_t* interp, abc_host_t const* host)
{
    uint16_t tn = pop16(interp);
    uint16_t tb = pop16(interp);
    if(interp->text_font >= 0xff000000) return ABC_RESULT_ERROR;
    char c;
    uint16_t wmax = 0;
    uint16_t w = 0;
    while(tn != 0)
    {
        uint8_t* ptr = refptr(interp, tb++);
        if(!ptr) return ABC_RESULT_ERROR;
        c = *ptr;
        if(c == '\0') break;
        --tn;
        if(c == '\n')
        {
            if(w > wmax) wmax = w;
            w = 0;
            continue;
        }
        w += font_get_x_advance(interp, host, c);
    }
    if(w > wmax) wmax = w;
    return push16(interp, wmax);
}

static abc_result_t sys_text_width_P(abc_interp_t* interp, abc_host_t const* host)
{
    uint32_t tn = pop24(interp);
    uint32_t tb = pop24(interp);
    if(interp->text_font >= 0xff000000) return ABC_RESULT_ERROR;
    char c;
    uint16_t wmax = 0;
    uint16_t w = 0;
    while(tn != 0)
    {
        c = (char)prog8(host, tb++);
        if(c == '\0') break;
        --tn;
        if(c == '\n')
        {
            if(w > wmax) wmax = w;
            w = 0;
            continue;
        }
        w += font_get_x_advance(interp, host, c);
    }
    if(w > wmax) wmax = w;
    return push16(interp, wmax);
}

static abc_result_t sys_text_wrap(abc_interp_t* interp, abc_host_t const* host)
{
    uint16_t tn = pop16(interp);
    uint16_t tb = pop16(interp);
    uint8_t w = pop8(interp);
    if(interp->text_font >= 0xff000000) return ABC_RESULT_ERROR;
    char c;
    uint8_t cw = 0; // current width
    char* p = (char*)refptr(interp, tb);
    if(!p) return ABC_RESULT_ERROR;
    char* tp = p;   // pointer after last word break
    uint8_t tw = 0; // width at last word break
    uint16_t ttn = tn;   // tn at last word break
    while((c = *p) != '\0' && tn != 0)
    {
        p = (char*)refptr(interp, ++tb);
        if(!p) return ABC_RESULT_ERROR;
        --tn;
        cw += font_get_x_advance(interp, host, c);
        if(c == '\n')
        {
            cw = 0;
            tw = 0;
            tp = p;
            continue;
        }
        if(c == ' ')
        {
            tp = p;
            tw = cw;
            ttn = tn;
        }
        if(cw <= w) continue;
        if(tw == 0) continue;
        p = tp;
        *(tp - 1) = '\n';
        cw = 0;
        tw = 0;
        tn = ttn;
    }

    return ABC_RESULT_NORMAL;
}

static void advance_audio_channel(abc_interp_t* interp, abc_host_t const* host, uint8_t i)
{
    uint32_t addr = interp->audio_addrs[i];
    if(addr == 0)
        return;
    uint8_t tone = prog8(host, addr++);
    uint8_t tick = prog8(host, addr++);
    if(tone > 128)
        addr = 0;
    interp->audio_ticks[i] = tick;
    interp->audio_tones[i] = tone;
    interp->audio_addrs[i] = addr;
    if(interp->audio_addrs[0] == 0 && interp->audio_addrs[1] == 0)
        interp->music_active = false;
}

static abc_result_t sys_music_play(abc_interp_t* interp, abc_host_t const* host)
{
    uint32_t song = pop24(interp);
    if(!interp->audio_enabled)
        return ABC_RESULT_NORMAL;
    uint32_t offset = prog24(host, song);
    song += 3;
    interp->audio_addrs[0] = song;
    interp->audio_addrs[1] = song + offset;
    advance_audio_channel(interp, host, 0);
    advance_audio_channel(interp, host, 1);
    interp->music_active = 1;
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_music_stop(abc_interp_t* interp)
{
    if(interp->music_active)
    {
        interp->audio_addrs[0] = 0;
        interp->audio_addrs[1] = 0;
    }
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_music_playing(abc_interp_t* interp)
{
    return push(interp, interp->music_active != 0 ? 1 : 0);
}

static abc_result_t sys_tones_play(abc_interp_t* interp, abc_host_t const* host)
{
    uint32_t t = pop24(interp);
    interp->audio_addrs[2] = t;
    advance_audio_channel(interp, host, 2);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_tones_play_primary(abc_interp_t* interp, abc_host_t const* host)
{
    uint32_t t = pop24(interp);
    if(interp->music_active)
    {
        interp->music_active = false;
        interp->audio_addrs[1] = 0;
    }
    interp->audio_addrs[0] = t;
    advance_audio_channel(interp, host, 0);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_tones_play_auto(abc_interp_t* interp, abc_host_t const* host)
{
    uint32_t t = pop24(interp);
    uint8_t i = interp->audio_addrs[2] == 0 || interp->audio_addrs[0] != 0 ? 2 : 0;
    interp->audio_addrs[i] = t;
    advance_audio_channel(interp, host, i);
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_tones_stop(abc_interp_t* interp)
{
    interp->audio_tones[2] = 0;
    if(!interp->music_active)
        interp->audio_addrs[0] = 0;
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_tones_playing(abc_interp_t* interp)
{
    uint32_t t =
        interp->audio_addrs[0] |
        interp->audio_addrs[1] |
        interp->audio_addrs[2];
    return push(interp,
        t != 0 && (!interp->music_active || interp->audio_addrs[2] != 0) ? 1 : 0);
}

static abc_result_t sys_audio_stop(abc_interp_t* interp)
{
    interp->audio_addrs[0] = 0;
    interp->audio_addrs[1] = 0;
    interp->audio_addrs[2] = 0;
    interp->music_active = 0;
    return ABC_RESULT_NORMAL;
}

static abc_result_t sys_audio_playing(abc_interp_t* interp)
{
    uint32_t t =
        interp->audio_addrs[0] |
        interp->audio_addrs[1] |
        interp->audio_addrs[2];
    return push(interp, t != 0 ? 1 : 0);
}

static abc_result_t sys_sin(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, sinf(a));
}

static abc_result_t sys_cos(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, cosf(a));
}

static abc_result_t sys_tan(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, tanf(a));
}

static abc_result_t sys_atan2(abc_interp_t* interp)
{
    float a = popf(interp);
    float b = popf(interp);
    return pushf(interp, atan2f(a, b));
}

static abc_result_t sys_floor(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, floorf(a));
}

static abc_result_t sys_ceil(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, ceilf(a));
}

static abc_result_t sys_round(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, roundf(a));
}

static abc_result_t sys_mod(abc_interp_t* interp)
{
    float a = popf(interp);
    float b = popf(interp);
    return pushf(interp, fmodf(a, b));
}

static abc_result_t sys_pow(abc_interp_t* interp)
{
    float a = popf(interp);
    float b = popf(interp);
    return pushf(interp, powf(a, b));
}

static abc_result_t sys_sqrt(abc_interp_t* interp)
{
    float a = popf(interp);
    return pushf(interp, sqrtf(a));
}

static abc_result_t sys(abc_interp_t* interp, abc_host_t const* h)
{
    uint8_t sysnum = imm8(interp, h) >> 1;

    switch(sysnum)
    {
    case SYS_DISPLAY:              return sys_display(interp);
    case SYS_DISPLAY_NOCLEAR:      return sys_display_noclear(interp);
    case SYS_GET_PIXEL:            return sys_get_pixel(interp);
    case SYS_DRAW_PIXEL:           return sys_draw_pixel(interp);
    case SYS_DRAW_HLINE:           return sys_draw_hline(interp);
    case SYS_DRAW_VLINE:           return sys_draw_vline(interp);
    case SYS_DRAW_LINE:            return sys_draw_line(interp);
    case SYS_DRAW_RECT:            return sys_draw_rect(interp);
    case SYS_DRAW_FILLED_RECT:     return sys_draw_filled_rect(interp);
    case SYS_DRAW_CIRCLE:          return sys_draw_circle(interp);
    case SYS_DRAW_FILLED_CIRCLE:   return sys_draw_filled_circle(interp);
    case SYS_DRAW_SPRITE:          return sys_draw_sprite(interp, h);
    case SYS_DRAW_SPRITE_SELFMASK: return sys_draw_sprite_selfmask(interp, h);
    case SYS_DRAW_TEXT:            return sys_draw_text(interp, h);
    case SYS_DRAW_TEXT_P:          return sys_draw_text_P(interp, h);
    case SYS_DRAW_TEXTF:           return sys_draw_textf(interp, h);
    case SYS_TEXT_WIDTH:           return sys_text_width(interp, h);
    case SYS_TEXT_WIDTH_P:         return sys_text_width_P(interp, h);
    case SYS_WRAP_TEXT:            return sys_text_wrap(interp, h);
    case SYS_SET_TEXT_FONT:        return sys_set_text_font(interp);
    case SYS_SET_TEXT_COLOR:       return sys_set_text_color(interp);
    case SYS_SET_FRAME_RATE:       return sys_set_frame_rate(interp);
    case SYS_NEXT_FRAME:           return sys_next_frame(interp, h);
    case SYS_IDLE:                 return ABC_RESULT_IDLE;
    case SYS_DEBUG_BREAK:          return ABC_RESULT_BREAK;
    case SYS_DEBUG_PRINTF:         return sys_debug_printf(interp, h);
    case SYS_ASSERT:               return sys_assert(interp);
    case SYS_POLL_BUTTONS:         return sys_poll_buttons(interp, h);
    case SYS_JUST_PRESSED:         return sys_just_pressed(interp);
    case SYS_JUST_RELEASED:        return sys_just_released(interp);
    case SYS_PRESSED:              return sys_pressed(interp, h);
    case SYS_ANY_PRESSED:          return sys_any_pressed(interp, h);
    case SYS_NOT_PRESSED:          return sys_not_pressed(interp, h);
    case SYS_MILLIS:               return sys_millis(interp, h);
    case SYS_MEMCPY:               return sys_memcpy(interp);
    case SYS_MEMCPY_P:             return sys_memcpy_P(interp, h);
    case SYS_STRLEN:               return sys_strlen(interp);
    case SYS_STRLEN_P:             return sys_strlen_P(interp, h);
    case SYS_STRCMP:               return sys_strcmp(interp);
    case SYS_STRCMP_P:             return sys_strcmp_P(interp, h);
    case SYS_STRCMP_PP:            return sys_strcmp_PP(interp, h);
    case SYS_STRCPY:               return sys_strcpy(interp);
    case SYS_STRCPY_P:             return sys_strcpy_P(interp, h);
    case SYS_FORMAT:               return sys_format(interp, h);
    case SYS_MUSIC_PLAY:           return sys_music_play(interp, h);
    case SYS_MUSIC_PLAYING:        return sys_music_playing(interp);
    case SYS_MUSIC_STOP:           return sys_music_stop(interp);
    case SYS_TONES_PLAY:           return sys_tones_play(interp, h);
    case SYS_TONES_PLAY_PRIMARY:   return sys_tones_play_primary(interp, h);
    case SYS_TONES_PLAY_AUTO:      return sys_tones_play_auto(interp, h);
    case SYS_TONES_PLAYING:        return sys_tones_playing(interp);
    case SYS_TONES_STOP:           return sys_tones_stop(interp);
    case SYS_AUDIO_ENABLED:        return sys_audio_enabled(interp);
    case SYS_AUDIO_TOGGLE:         return sys_audio_toggle(interp);
    case SYS_AUDIO_PLAYING:        return sys_audio_playing(interp);
    case SYS_AUDIO_STOP:           return sys_audio_stop(interp);
    case SYS_SAVE_EXISTS:          return sys_save_exists(interp, h);
    case SYS_SAVE:                 return sys_save(interp, h);
    case SYS_LOAD:                 return sys_load(interp, h);
    case SYS_SIN:                  return sys_sin(interp);
    case SYS_COS:                  return sys_cos(interp);
    case SYS_TAN:                  return sys_tan(interp);
    case SYS_ATAN2:                return sys_atan2(interp);
    case SYS_FLOOR:                return sys_floor(interp);
    case SYS_CEIL:                 return sys_ceil(interp);
    case SYS_ROUND:                return sys_round(interp);
    case SYS_MOD:                  return sys_mod(interp);
    case SYS_POW:                  return sys_pow(interp);
    case SYS_SQRT:                 return sys_sqrt(interp);
    case SYS_GENERATE_RANDOM_SEED: goto unknown_sysfunc;
    case SYS_INIT_RANDOM_SEED:     goto unknown_sysfunc;
    case SYS_RANDOM:               goto unknown_sysfunc;
    case SYS_RANDOM_RANGE:         goto unknown_sysfunc;
    default:
    unknown_sysfunc:
        assert(0);
        return ABC_RESULT_ERROR;
    }
}

abc_result_t abc_run(abc_interp_t* interp, abc_host_t const* h)
{
    if(!interp || !h || !h->prog)
        return ABC_RESULT_ERROR;

    if(interp->pc == 0)
    {
        /* interpreter was reset: initialize state */
        memset(interp, 0, sizeof(abc_interp_t));
        interp->pc = 20;
        interp->audio_enabled = 1;
        interp->text_font = 0xffffffff;
        interp->text_color = 1;
        interp->frame_dur = 50;
    }
    
    uint8_t instr = imm8(interp, h);
    
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
    case I_AIXB1: return aixb1(interp, h);
    case I_AIDXB: return aidxb(interp, h);
    case I_AIDX:  return aidx(interp, h);
    case I_PIDXB: return pidxb(interp, h);
    case I_PIDX:  return pidx(interp, h);
    case I_UAIDX: return uaidx(interp, h);
    case I_UPIDX: return upidx(interp, h);
    case I_ASLC:  return aslc(interp, h);
    case I_PSLC:  return pslc(interp, h);
    case I_REFL:  return push16(interp, 0x100 + interp->sp - imm8(interp, h));
    case I_REFGB: return push16(interp, 0x200 + imm8(interp, h));
    case I_INC:   return linc(interp, 1, +1);
    case I_DEC:   return linc(interp, 1, -1);
    case I_LINC:  return linc(interp, imm8(interp, h), +1);
    case I_PINC:  return pinc(interp, +1);
    case I_PINC2: return pinc2(interp, +1);
    case I_PINC3: return pinc3(interp, +1);
    case I_PINC4: return pinc4(interp, +1);
    case I_PDEC:  return pinc(interp, -1);
    case I_PDEC2: return pinc2(interp, -1);
    case I_PDEC3: return pinc3(interp, -1);
    case I_PDEC4: return pinc4(interp, -1);
    case I_PINCF: return pincf(interp, +1.f);
    case I_PDECF: return pincf(interp, -1.f);
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
    case I_BZ:    return bz(interp, h);
    case I_BZ1:   return bz1(interp, h);
    case I_BNZ:   return bnz(interp, h);
    case I_BNZ1:  return bnz1(interp, h);
    case I_BZP:   return bzp(interp, h);
    case I_BZP1:  return bzp1(interp, h);
    case I_BNZP:  return bnzp(interp, h);
    case I_BNZP1: return bnzp1(interp, h);
    case I_JMP:   interp->pc = imm24(interp, h); return ABC_RESULT_NORMAL;
    case I_JMP1:  interp->pc += (int8_t)imm8(interp, h); return ABC_RESULT_NORMAL;
    case I_CALL:  return call(interp, imm24(interp, h));
    case I_CALL1: return call(interp, interp->pc + (int8_t)imm8(interp, h));
    case I_RET:   return ret(interp);
    case I_SYS:   return sys(interp, h);
    default:
        assert(0);
        return ABC_RESULT_ERROR;
    }
}

static uint32_t audio_phase_adv(uint8_t tone, uint32_t sample_rate)
{
    if(tone == 0 || tone > 128)
        return 0;

    /* Compute frequency of tone in Hz */
    /* TODO: use LUT for this */
    float f = 440.f * powf(2.f, (1.f / 12.f) * (float)(tone - 69));

    /* Divide by sample rate to get phase advance per sample */
    float adv = f / (float)sample_rate;

    return (uint32_t)(adv * 4294967296.f);
}

void abc_audio(
    abc_interp_t* interp,
    abc_host_t const* host,
    int16_t* samples,     /* Audio sample buffer to fill */
    uint32_t num_samples, /* Number of requested samples */
    uint32_t sample_rate  /* Sample rate in Hz */
)
{
    if(!samples)
        return;

    if(interp == 0 || num_samples == 0)
    {
        memset(samples, 0, sizeof(int16_t) * num_samples);
        return;
    }

    uint32_t ns = (uint32_t)(1e9f * (float)num_samples / (float)sample_rate);
    ns += interp->audio_ns_rem;

    /* Audio ticks are 4 milliseconds */
    uint32_t ticks = ns / 4000000;
    interp->audio_ns_rem = ns - ticks * 4000000;

    /* Convert sample rate from Hz to samples/tick */
    float samples_per_tick = (float)sample_rate * 0.004f;
    float sample_count = 0.f;
    uint32_t index = 0;

    /* Update channel states */
    while(index < num_samples)
    {
        if(ticks != 0)
        {
            --ticks;
            for(uint8_t i = 0; i < 3; ++i)
            {
                if(--interp->audio_ticks[i] == 0)
                    advance_audio_channel(interp, host, i);
            }
        }

        /* Compute phase advances */
        uint32_t phase_adv[3];
        for(uint8_t i = 0; i < 3; ++i)
        {
            phase_adv[i] = interp->audio_addrs[i] == 0 ? 0 :
                audio_phase_adv(interp->audio_tones[i], sample_rate);
        }

        /* Figure out how many samples to produce for this tick */
        sample_count += samples_per_tick;
        uint32_t n = (uint32_t)sample_count;
        sample_count -= (float)n;
        n += index;
        if(n > num_samples)
            n = num_samples;

        while(index < n)
        {
            /* Compute sample */
            int16_t s = 0;
            for(uint8_t i = 0; i < 3; ++i)
            {
                if(interp->audio_addrs[i] == 0) continue;

                /* Square wave audio */
                if(interp->audio_phase[i] < 0x80000000u)
                    s += 2048;
                else
                    s -= 2048;

                interp->audio_phase[i] += phase_adv[i];
            }

            /* Write sample to buffer */
            samples[index++] = s;
        }
    }

    /* Fill out any remainder of the buffer */
    for(; index < num_samples; ++index)
        samples[index] = samples[index - 1];

    return;
}
