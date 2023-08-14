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

    SYS_NUM
};

enum instr_t : uint8_t
{
    I_NOP,

    I_PUSH,  // push imm
    I_GETL,  // push stack[top - imm] (imm=1 is TOS)
    I_GETLN, // pop N then same as GETL but push N bytes
    I_SETL,  // pop, then store to stack[top - imm]
    I_SETLN, // pop N then same as SETL but pop/store N bytes
    I_GETG,  // push globals[imm]
    I_GETGN, // pop N then same as GETG but push N bytes
    I_SETG,  // pop, then store to globals[imm]
    I_SETGN, // pop N then same as SETG but pop/store N bytes
    I_POP,   // a |
    I_ADD,   // a b | a+b
    I_ADD2,  // a0 a1 b0 b1 | (a+b)0 (a+b)1
    I_SUB,   // a b | a-b
    I_SUB2,  // a0 a1 b0 b1 | (a-b)0 (a-b)1
    I_NOT,   // a | !a
    I_BZ,    // pop, branch if zero to imm3
    I_BNZ,   // pop, branch if nonzero to imm3
    I_JMP,   // jmp imm3
    I_CALL,
    I_RET,
    I_SYS,   // call sysfunc
};

}
