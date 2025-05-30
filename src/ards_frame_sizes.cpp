#include "ards_compiler.hpp"

namespace ards
{

// TODO: SETGN, SETLN, GETGN, GETLN should use immediates

void compiler_t::update_frame_sizes(std::vector<compiler_instr_t>& instrs)
{
    int32_t n = 0;
    for(size_t i = 0; i < instrs.size(); ++i)
    {
        instrs[i].frame_size = uint16_t(n);
        switch(instrs[i].instr)
        {
        case I_SETGN:
            assert(i > 0);
            n -= 1;
            n -= instrs[i - 1].imm;
            break;
        case I_POP4:
        case I_SETR2:
        case I_SETG4:
            n -= 4;
            break;
        case I_POP3:
        case I_SETR:
            n -= 3;
            break;
        case I_POP2:
        case I_SETG2:
            n -= 2;
            break;
        case I_POP:
        case I_SETG:
            n -= 1;
            break;
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
            break;
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
            n += 1;
            break;
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
            n += 2;
            break;
        case I_P000:
        case I_PUSHL:
        case I_PUSH3:
        case I_SEXT3:
            n += 3;
            break;
        case I_P0000:
        case I_PUSH4:
        case I_GETL4:
        case I_GETG4:
        case I_GTGB4:
            n += 4;
            break;
        case I_PZ8:
            n += 8;
            break;
        case I_PZ16:
            n += 16;
            break;
        case I_ALLOC:
            n += instrs[i].imm;
            break;
        case I_GETLN:
            assert(i > 0);
            n -= 1;
            n += instrs[i - 1].imm;
            break;
        case I_GETPN:
            n -= 3;
            n += instrs[i].imm;
            break;
        case I_GETRN:
            n -= 2;
            n += instrs[i].imm;
            break;
        case I_POPN:
            n -= instrs[i].imm;
            break;
        default:
            assert(0);
            break;
        }
        assert(n >= 0);
        instrs[i].frame_size_after = uint16_t(n);
    }
}
    
}
