#pragma once

namespace ards
{

enum sysfunc_t : uint16_t
{
    SYS_DISPLAY,
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
    SYS_SET_FRAME_RATE,
    SYS_NEXT_FRAME,
    SYS_IDLE,
    SYS_DEBUG_BREAK,
    SYS_ASSERT,
    SYS_POLL_BUTTONS,
    SYS_JUST_PRESSED,
    SYS_JUST_RELEASED,
    SYS_PRESSED,
    SYS_ANY_PRESSED,
    SYS_NOT_PRESSED,
    SYS_MILLIS,
    SYS_STRLEN,
    SYS_STRLEN_P,
    SYS_STRCMP,
    SYS_STRCMP_P,
    //SYS_STRCPY,
    //SYS_STRCPY_P,
    SYS_FORMAT,
    SYS_TONES_PLAY,
    SYS_TONES_PLAYING,
    SYS_TONES_STOP,
    SYS_AUDIO_ENABLED,
    SYS_AUDIO_TOGGLE,
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

    SYS_NUM
};

enum instr_t : uint8_t
{
    I_NOP,

    I_PUSH,  // push imm
    I_P0,    // push 0
    I_P1,    // push 1
    I_P2,    // push 2
    I_P3,    // push 3
    I_P4,    // push 4
    I_P5,    // push 5
    I_P6,    // push 6
    I_P7,    // push 7
    I_P8,    // push 8
    I_P16,   // push 16
    I_P32,   // push 32
    I_P64,   // push 64
    I_P128,  // push 128
    I_P00,   // push 0; push 0
    I_PZN,   // push 0 (<imm8> times, imm > 2)
    I_PUSHG, // push imm16
    I_PUSHL, // push imm24
    I_PUSH4, // push imm32

    I_SEXT,  // push 0x00 or 0xff to sign extend TOS
    I_SEXT2, // same as sext but do it twice
    I_SEXT3, // same as sext but do it three times

    I_DUP,   // a | a a
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

    I_GETL,  // push stack[top - imm] (imm=1 is TOS)
    I_GETL2,
    I_GETL4,
    I_GETLN, // pop N then same as GETL but push N bytes
    I_SETL,  // pop, then store to stack[top - imm]
    I_SETL2, // pop, then store to stack[top - imm] (2 bytes)
    I_SETL4, // pop, then store to stack[top - imm] (4 bytes)
    I_SETLN, // pop N then same as SETL but pop/store N bytes
    I_GETG,  // push globals[imm]
    I_GETG2, // push globals[imm]
    I_GETGN, // pop N then same as GETG but push N bytes
    I_SETG,  // pop, then store to globals[imm]
    I_SETG2, // pop, then store to globals[imm] (2 bytes)
    I_SETGN, // pop N then same as SETG but pop/store N bytes

    I_GETP,  // pop addr24 then push 1 byte from prog address
    I_GETPN, // pop addr24 then push imm8 bytes from prog address

    I_GETR,  // ref | (1 byte *ref)
    I_GETR2, // ref | (2 bytes *ref)
    I_GETRN, // ref | (imm8 bytes *ref)
    I_SETR,  // (1 byte data) ref |
    I_SETR2, // (2 bytes data) ref |
    I_SETRN, // (imm8 bytes data) ref |

    I_POP,   // a |
    I_POP2,  // a b |
    I_POP3,  // a b c |
    I_POP4,  // a b c d |
    I_POPN,  // pop imm8 bytes

    // array reference (RAM)
    I_AIXB1, // ref i | (ref+i*1) with bounds checking with imm (8-bit)
    I_AIDXB, // ref i | (ref+i*imm) with bounds checking with imm2 (8-bit)
    I_AIDX,  // ref i | (ref+i*imm) with bounds checking with imm2 (16-bit)

    // array reference (prog)
    I_PIDXB, // pref i | (pref+i*imm) with bounds checking with imm2 (8-bit)
    I_PIDX,  // pref i | (pref+i*imm) with bounds checking with imm2
             //          imm:    16-bit
             //          i/imm2: 24-bit

    // unsized array references
    I_UAIDX, // aref i  | (aref+i*imm) with bounds checking
    I_UPIDX, // apref i | (apref+i*imm) with bounds checking
             //           imm: 16-bit
             //           i:   24-bit

    I_REFL,  // imm8 -> pointer to local var
    I_REFG,  // imm16 -> pointer to global var
    I_REFGB, // imm8 -> pointer to global var
    
    I_INC,
    I_DEC,
    I_LINC,  // increment 1 byte local at imm8

    I_PINC,  // ref | x   post-increment ref
    I_PINC2, // ref | x   post-increment ref
    I_PINC3, // ref | x   post-increment ref
    I_PINC4, // ref | x   post-increment ref
    I_PDEC,  // ref | x   post-decrement ref
    I_PDEC2, // ref | x   post-decrement ref
    I_PDEC3, // ref | x   post-decrement ref
    I_PDEC4, // ref | x   post-decrement ref
    I_PINCF,
    I_PDECF,

    I_ADD,   // a b | a+b
    I_ADD2,  // a0 a1 b0 b1 | (a+b)0 (a+b)1
    I_ADD3,  //
    I_ADD4,  //
    I_SUB,   // a b | a-b
    I_SUB2,  // a0 a1 b0 b1 | (a-b)0 (a-b)1
    I_SUB3,  //
    I_SUB4,  //

    I_ADD2B, // a0 a1 b0 | (a+b)0 (a+b)1

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
    I_LSL3,
    I_LSL4,
    I_LSR,
    I_LSR2,
    I_LSR3,
    I_LSR4,
    I_ASR,
    I_ASR2,
    I_ASR3,
    I_ASR4,

    I_AND,
    I_AND2,
    I_AND3,
    I_AND4,
    I_OR,
    I_OR2,
    I_OR3,
    I_OR4,
    I_XOR,
    I_XOR2,
    I_XOR3,
    I_XOR4,
    I_COMP,
    I_COMP2,
    I_COMP3,
    I_COMP4,

    I_BOOL,  // a | (a!=0)
    I_BOOL2, // a b | (a!=0 && b!=0)
    I_BOOL3, // a b c | (a!=0 && b!=0 && c!=0)
    I_BOOL4, // a b c d | (a!=0 && b!=0 && c!=0 && d!=0)
    I_CULT,  // a b | a < b (unsigned)
    I_CULT2,
    I_CULT3,
    I_CULT4,
    I_CSLT,  // a b | a < b (signed)
    I_CSLT2,
    I_CSLT3,
    I_CSLT4,
    I_CULE,  // a b | a <= b (unsigned)
    I_CULE2,
    I_CULE3,
    I_CULE4,
    I_CSLE,  // a b | a <= b (signed)
    I_CSLE2,
    I_CSLE3,
    I_CSLE4,

    I_CFEQ,
    I_CFLT,
    I_CFLE,

    I_NOT,   // a | !a

    I_FADD,
    I_FSUB,
    I_FMUL,
    I_FDIV,
    I_F2I,
    I_F2U,
    I_I2F,
    I_U2F,

    I_BZ,     // pop, branch if zero to imm3
    I_BZ1,    // imm8 rel offset
    //I_BZ2,  // imm16 rel offset
    I_BNZ,    // pop, branch if nonzero to imm3
    I_BNZ1,   // imm8 rel offset
    //I_BNZ2, // imm16 rel offset
    I_BZP,    // 
    I_BZP1,   // 
    //I_BZP2, // (same as dup; b[n]z; pop)
    I_BNZP,   // 
    I_BNZP1,  // 
    //I_BNZP2,// 
    I_JMP,    // jmp imm3
    I_JMP1,   // imm8 rel offset
    //I_JMP2, // imm16 rel offset
    I_CALL,
    I_CALL1,  // imm8 rel offset
    //I_CALL2,// imm16 rel offset
    I_RET,

    I_SYS,   // call sysfunc (imm16)
    I_SYSB,  // call sysfunc (imm8)

    //
    // pseudo-instructions just for compiler optimizations
    //

    I_REMOVE, // dummy value used to indicate an optimized-out instruction

    I_PUSH2,
    I_PUSH3,

};

}
