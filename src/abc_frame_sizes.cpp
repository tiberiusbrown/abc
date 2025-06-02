#include "abc_compiler.hpp"

namespace abc
{

int compiler_t::instr_stack_mod(compiler_instr_t const& i)
{
    switch(i.instr)
    {
    case I_SETGN:
    case I_POPN:
        return -(int)i.imm;

    case I_GETLN:
        return i.imm - 1;
    case I_GETRN:
        return i.imm - 2;
    case I_GETPN:
        return i.imm - 3;
    case I_ALLOC:
        return i.imm;

    case I_POP4:
    case I_SETR2:
    case I_SETG4:
        return -4;
    case I_POP3:
    case I_SETR:
        return -3;
    case I_POP2:
    case I_SETG2:
        return -2;
    case I_POP:
    case I_SETG:
        return -1;
    case I_NOP:
    case I_INC:
    case I_DEC:
    case I_LINC:
    case I_F2I:
    case I_F2U:
    case I_I2F:
    case I_U2F:
    case I_NOT:
    case I_BOOL:
    case I_COMP:
    case I_JMP:
    case I_JMP1:
    case I_CALL:
    case I_CALL1:
    case I_ICALL:
    case I_IJMP:
    case I_RET:
    case I_SYS:
    case I_GETR2:
        return 0;
    case I_PUSH:
    case I_P0:
    case I_P1:
    case I_P2:
    case I_P3:
    case I_P4:
    case I_P5:
    case I_P6:
    case I_P7:
    case I_P8:
    case I_P16:
    case I_P32:
    case I_P64:
    case I_P128:
    case I_SEXT:
    case I_DUP:
    case I_DUP2:
    case I_DUP3:
    case I_DUP4:
    case I_DUP5:
    case I_DUP6:
    case I_DUP7:
    case I_DUP8:
    case I_GETL:
    case I_GETP:
    case I_GETG:
    case I_GTGB:
    case I_REFL:
    case I_REFGB:
        return 1;
    case I_P00:
    case I_PUSHG:
    case I_PUSH2:
    case I_SEXT2:
    case I_DUPW:
    case I_DUPW2:
    case I_DUPW3:
    case I_DUPW4:
    case I_DUPW5:
    case I_DUPW6:
    case I_DUPW7:
    case I_DUPW8:
    case I_GETL2:
    case I_GETG2:
    case I_GTGB2:
        return 2;
    case I_P000:
    case I_PUSHL:
    case I_PUSH3:
    case I_SEXT3:
        return 3;
    case I_P0000:
    case I_PUSH4:
    case I_GETL4:
    case I_GETG4:
    case I_GTGB4:
        return 4;
    case I_PZ8:
        return 8;
    case I_PZ16:
        return 16;
    default:
        assert(0);
        return 0;
    }
}
    
}
