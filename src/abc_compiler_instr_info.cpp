#include "abc_compiler.hpp"

namespace abc
{

static uint32_t sys_stack_access(compiler_instr_t const& i)
{
    auto sys = (sysfunc_t)i.imm;

    if(sysfunc_is_format(sys))
        return i.imm2;

    uint32_t n = 0;

    auto it = sysfunc_decls.find(sys);
    if(it == sysfunc_decls.end())
    {
        assert(0);
        return 256u;
    }

    auto const& decl = it->second.decl;
    for(auto const& arg : decl.arg_types)
        n += (uint32_t)arg.prim_size;
    return n;
}

std::pair<bool, uint32_t> compiler_t::instr_accesses_stack(
    compiler_instr_t const& i, uint32_t off)
{
    switch(i.instr)
    {
    case I_SYS:
    {
        auto n = sys_stack_access(i);
        return { off <= n, n };
    }

    case I_GETL:
    case I_LINC:
        return { off == i.imm, i.imm };
    case I_GETL2:
        return { off >= i.imm && off <= i.imm + 1, i.imm + 1 };
    case I_GETL4:
        return { off >= i.imm && off <= i.imm + 3, i.imm + 3 };
    case I_GETLN:
        return { off >= i.imm2 && off <= i.imm2 + i.imm - 1, i.imm2 + i.imm - 1 };

    case I_SETL:
        return { off <= 1 || off == i.imm + 1, i.imm + 1 };
    case I_SETL2:
        return { off <= 2 || off >= i.imm + 1 && off <= i.imm + 2, i.imm + 2 };
    case I_SETL4:
        return { off <= 4 || off >= i.imm + 1 && off <= i.imm + 4, i.imm + 4 };
    case I_SETLN:
        return {
            off <= i.imm || off >= i.imm2 + 1 && off <= i.imm2 + i.imm,
            i.imm2 + i.imm };

    case I_POP:
    case I_BZ1:
    case I_BNZ1:
    case I_BZP1:
    case I_BNZP1:
    case I_BZ2:
    case I_BNZ2:
    case I_BZ:
    case I_BNZ:
    case I_BZP:
    case I_BNZP:
    case I_DUP:
    case I_SEXT:
    case I_BOOL:
    case I_COMP:
    case I_NOT:
    case I_INC:
    case I_DEC:
    case I_SETG:
        return { off <= 1, 1 };
    case I_ADD:
    case I_SUB:
    case I_MUL:
    case I_POP2:
    case I_DUPW:
    case I_SEXT2:
    case I_BOOL2:
    case I_COMP2:
    case I_AND:
    case I_OR:
    case I_XOR:
    case I_CULT:
    case I_CSLT:
    case I_LSL:
    case I_LSR:
    case I_ASR:
    case I_PINC:
    case I_PINC2:
    case I_PINC3:
    case I_PINC4:
    case I_PDEC:
    case I_PDEC2:
    case I_PDEC3:
    case I_PDEC4:
    case I_PINCF:
    case I_PDECF:
    case I_SETG2:
        return { off <= 2, 2 };
    case I_POP3:
    case I_IJMP:
    case I_ICALL:
    case I_SEXT3:
    case I_ADD2B:
    case I_SUB2B:
    case I_MUL2B:
    case I_BOOL3:
    case I_LSL2:
    case I_LSR2:
    case I_ASR2:
    case I_GETP:
    case I_GETPN:
    case I_AIXB1:
    case I_AIDXB:
        return { off <= 3, 3 };
    case I_POP4:
    case I_ADD2:
    case I_SUB2:
    case I_MUL2:
    case I_DIV2:
    case I_MOD2:
    case I_UDIV2:
    case I_UMOD2:
    case I_ADD3B:
    case I_BOOL4:
    case I_COMP4:
    case I_AND2:
    case I_OR2:
    case I_XOR2:
    case I_F2I:
    case I_F2U:
    case I_I2F:
    case I_U2F:
    case I_CULT2:
    case I_CSLT2:
    case I_AIDX:
    case I_PIDXB:
    case I_SETG4:
        return { off <= 4, 4 };
    case I_LSL4:
    case I_LSR4:
    case I_ASR4:
        return { off <= 5, 5 };
    case I_ADD3:
    case I_SUB3:
    case I_MUL3:
    case I_CULT3:
    case I_CSLT3:
    case I_UAIDX:
    case I_PIDX:
        return { off <= 6, 6 };
    case I_ADD4:
    case I_SUB4:
    case I_MUL4:
    case I_DIV4:
    case I_MOD4:
    case I_UDIV4:
    case I_UMOD4:
    case I_FADD:
    case I_FSUB:
    case I_FMUL:
    case I_FDIV:
    case I_AND4:
    case I_OR4:
    case I_XOR4:
    case I_CFEQ:
    case I_CFLT:
    case I_CULT4:
    case I_CSLT4:
    case I_ASLC:
        return { off <= 8, 8 };
    case I_UPIDX:
        return { off <= 9, 9 };
    case I_PSLC:
        return { off <= 12, 12 };
    case I_POPN:
    case I_SETGN:
        return { off <= i.imm, i.imm };
    case I_DUP2:
        return { off == 2, 2 };
    case I_DUP3:
        return { off == 3, 3 };
    case I_DUP4:
        return { off == 4, 4 };
    case I_DUP5:
        return { off == 5, 5 };
    case I_DUP6:
        return { off == 6, 6 };
    case I_DUP7:
        return { off == 7, 7 };
    case I_DUP8:
        return { off == 8, 8 };
    case I_DUPW2:
        return { off >= 2 && off <= 3, 3 };
    case I_DUPW3:
        return { off >= 3 && off <= 4, 4 };
    case I_DUPW4:
        return { off >= 4 && off <= 5, 5 };
    case I_DUPW5:
        return { off >= 5 && off <= 6, 6 };
    case I_DUPW6:
        return { off >= 6 && off <= 7, 7 };
    case I_DUPW7:
        return { off >= 7 && off <= 8, 8 };
    case I_DUPW8:
        return { off >= 8 && off <= 9, 9 };
    case I_NOP:
    case I_GETG:
    case I_GETG2:
    case I_GETG4:
    case I_GETGN:
    case I_GTGB:
    case I_GTGB2:
    case I_GTGB4:
    case I_REFGB:
    case I_RET:
    case I_JMP:
    case I_JMP1:
    case I_JMP2:
    case I_CALL:
    case I_CALL1:
    case I_CALL2:
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
    case I_P00:
    case I_P000:
    case I_P0000:
    case I_PZ8:
    case I_PZ16:
    case I_ALLOC:
    case I_PUSH2:
    case I_PUSH3:
    case I_PUSH4:
    case I_PUSHG:
    case I_PUSHL:
        return { false, 0 };
    case I_REFL:
    case I_GETR:
    case I_GETR2:
    case I_GETRN:
    case I_SETR:
    case I_SETR2:
    case I_SETRN:
        return { true, 256 };
    default:
        assert(0);
        break;
    }
    return { true, 256 };
}

static int sys_stack_mod(compiler_instr_t const& i)
{
    auto sys = (sysfunc_t)i.imm;
    int n = 0;

    auto it = sysfunc_decls.find(sys);
    if(it == sysfunc_decls.end())
    {
        assert(0);
        return 0;
    }

    auto const& decl = it->second.decl;
    n += (int)decl.return_type.prim_size;
    if(sysfunc_is_format(sys))
        n -= (int)i.imm2;
    else
    {
        for(auto const& arg : decl.arg_types)
            n -= (int)arg.prim_size;
    }
    return n;
}

int compiler_t::instr_stack_mod(compiler_instr_t const& i)
{
    switch(i.instr)
    {
    case I_SYS:
        return sys_stack_mod(i);

    case I_SETRN:
        return -(int)i.imm - 2;
    case I_SETGN:
    case I_SETLN:
    case I_POPN:
        return -(int)i.imm;

    case I_GETRN:
        return i.imm - 2;
    case I_GETPN:
        return i.imm - 3;
    case I_ALLOC:
    case I_GETGN:
    case I_GETLN:
        return i.imm;

    case I_CULT4:
    case I_CSLT4:
    case I_CFLT:
    case I_CFEQ:
        return -7;
    case I_UPIDX:
    case I_PSLC:
        return -6;
    case I_CULT3:
    case I_CSLT3:
        return -5;
    case I_POP4:
    case I_SETR2:
    case I_SETG4:
    case I_ADD4:
    case I_SUB4:
    case I_MUL4:
    case I_DIV4:
    case I_UDIV4:
    case I_MOD4:
    case I_UMOD4:
    case I_AND4:
    case I_OR4:
    case I_XOR4:
    case I_UAIDX:
    case I_SETL4:
    case I_FADD:
    case I_FSUB:
    case I_FMUL:
    case I_FDIV:
    case I_ASLC:
        return -4;
    case I_POP3:
    case I_SETR:
    case I_BOOL4:
    case I_ADD3:
    case I_SUB3:
    case I_MUL3:
    case I_IJMP:
    case I_ICALL:
    case I_CULT2:
    case I_CSLT2:
    case I_PIDX:
        return -3;
    case I_POP2:
    case I_SETG2:
    case I_ADD2:
    case I_SUB2:
    case I_MUL2:
    case I_DIV2:
    case I_UDIV2:
    case I_MOD2:
    case I_UMOD2:
    case I_BOOL3:
    case I_AND2:
    case I_OR2:
    case I_XOR2:
    case I_AIDX:
    case I_SETL2:
        return -2;
    case I_POP:
    case I_SETG:
    case I_BOOL2:
    case I_ADD:
    case I_SUB:
    case I_MUL:
    case I_ADD2B:
    case I_ADD3B:
    case I_SUB2B:
    case I_MUL2B:
    case I_LSL:
    case I_LSL2:
    case I_LSL4:
    case I_LSR:
    case I_LSR2:
    case I_LSR4:
    case I_ASR:
    case I_ASR2:
    case I_ASR4:
    case I_AND:
    case I_OR:
    case I_XOR:
    case I_BZ1:
    case I_BNZ1:
    case I_BZP1:
    case I_BNZP1:
    case I_BZ2:
    case I_BNZ2:
    case I_BZ:
    case I_BNZ:
    case I_BZP:
    case I_BNZP:
    case I_CULT:
    case I_CSLT:
    case I_GETR:
    case I_PINC:
    case I_PDEC:
    case I_AIDXB:
    case I_AIXB1:
    case I_PIDXB:
    case I_SETL:
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
    case I_COMP2:
    case I_COMP4:
    case I_JMP:
    case I_JMP1:
    case I_JMP2:
    case I_CALL:
    case I_CALL1:
    case I_CALL2:
    case I_RET:
    case I_GETR2:
    case I_PINC2:
    case I_PDEC2:
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
    case I_PINC3:
    case I_PDEC3:
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
    case I_REFL:
    case I_REFGB:
    case I_PINC4:
    case I_PDEC4:
    case I_PINCF:
    case I_PDECF:
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
