#pragma once

namespace ards
{

enum instr_t : uint8_t
{
    I_NOP,

    I_PUSH,  // push imm
    I_GETL,  // push stack[top - imm] (imm=1 is TOS)
    I_GETL2, // same as GETL but push 2 bytes
    I_SETL,  // pop, then store to stack[top - imm]
    I_SETL2, // same as SETL but pop/store 2 bytes
    I_POP,   // pop one element
    I_ADD,   // a b | a+b
    I_ADD2,  // a0 a1 b0 b1 | (a+b)0 (a+b)1
    I_SUB,   // a b | a-b
    I_SUB2,  // a0 a1 b0 b1 | (a-b)0 (a-b)1
    I_BZ,    // pop, branch if zero to imm3
    I_BNZ,   // pop, branch if nonzero to imm3
    I_BNEG,  // pop, branch if negative
    I_JMP,   // jmp imm3
    I_CALL,
    I_RET,
    I_SYS,   // call sys2
};

}
