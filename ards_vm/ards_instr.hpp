#pragma once

namespace ards
{

enum sysfunc_t : uint16_t
{
    SYS_DISPLAY,
    SYS_DRAW_PIXEL,
    SYS_DRAW_FILLED_RECT,
    SYS_SET_FRAME_RATE,
    SYS_NEXT_FRAME,
    SYS_IDLE,
    SYS_DEBUG_BREAK,

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
    I_P00,   // push 0; push 0

    I_SEXT,  // push 0x00 or 0xff to sign extend TOS

    I_DUP,   // a | a a

    I_GETL,  // push stack[top - imm] (imm=1 is TOS)
    I_GETL2,
    I_GETLN, // pop N then same as GETL but push N bytes
    I_SETL,  // pop, then store to stack[top - imm]
    I_SETLN, // pop N then same as SETL but pop/store N bytes
    I_GETG,  // push globals[imm]
    I_GETGN, // pop N then same as GETG but push N bytes
    I_SETG,  // pop, then store to globals[imm]
    I_SETGN, // pop N then same as SETG but pop/store N bytes

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

    I_AIDXB, // ref i | (ref+i*imm) with bounds checking with imm2 (8-bit)
    I_AIDX,  // ref i | (ref+i*imm) with bounds checking with imm2 (16-bit)

    I_REFL,  // imm8 -> pointer to local var
    I_REFG,  // imm16 -> pointer to global var
    I_REFGB, // imm8 -> pointer to global var
    
    I_INC,

    I_ADD,   // a b | a+b
    I_ADD2,  // a0 a1 b0 b1 | (a+b)0 (a+b)1
    I_ADD3,  //
    I_ADD4,  //
    I_SUB,   // a b | a-b
    I_SUB2,  // a0 a1 b0 b1 | (a-b)0 (a-b)1
    I_SUB3,  //
    I_SUB4,  //

    I_MUL,
    I_MUL2,
    I_MUL3,
    I_MUL4,

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
    I_NOT,   // a | !a

    I_BZ,    // pop, branch if zero to imm3
    I_BZ1,   // imm8 rel offset
    //I_BZ2,   // imm16 rel offset
    I_BNZ,   // pop, branch if nonzero to imm3
    I_BNZ1,  // imm8 rel offset
    //I_BNZ2,  // imm16 rel offset
    I_JMP,   // jmp imm3
    I_JMP1,  // imm8 rel offset
    //I_JMP2,  // imm16 rel offset
    I_CALL,
    I_CALL1, // imm8 rel offset
    //I_CALL2, // imm16 rel offset
    I_RET,

    I_SYS,   // call sysfunc (imm16)
    I_SYSB,  // call sysfunc (imm8)

    I_REMOVE, // dummy value used to indicate an optimized-out instruction
};

}
