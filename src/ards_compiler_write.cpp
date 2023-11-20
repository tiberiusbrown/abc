#include "ards_compiler.hpp"

#include <assert.h>

namespace ards
{

static void write_instr(std::ostream& f, compiler_instr_t const& instr, uint16_t& line)
{
    if(instr.instr == I_REMOVE)
        return;
    if(instr.is_label)
    {
        f << instr.label << ":\n";
        return;
    }
    if(line != instr.line && instr.line != 0)
    {
        line = instr.line;
        f << "  .line " << instr.line << "\n";
    }
    f << "  ";
    switch(instr.instr)
    {
    case I_NOP:   f << "nop"; break;
    case I_PUSH:  f << "push  " << instr.imm; break;
    case I_P0:    f << "p0"; break;
    case I_P1:    f << "p1"; break;
    case I_P2:    f << "p2"; break;
    case I_P3:    f << "p3"; break;
    case I_P4:    f << "p4"; break;
    case I_P5:    f << "p5"; break;
    case I_P6:    f << "p6"; break;
    case I_P7:    f << "p7"; break;
    case I_P8:    f << "p8"; break;
    case I_P00:   f << "p00"; break;
    case I_PUSHG: f << "pushg " << instr.label; break;
    case I_PUSHL: f << "pushl " << instr.label; break;
    case I_SEXT:  f << "sext"; break;
    case I_DUP:   f << "dup"; break;
    case I_DUP2:  f << "dup2"; break;
    case I_DUP3:  f << "dup3"; break;
    case I_DUP4:  f << "dup4"; break;
    case I_DUP5:  f << "dup5"; break;
    case I_DUP6:  f << "dup6"; break;
    case I_DUP7:  f << "dup7"; break;
    case I_DUP8:  f << "dup8"; break;
    case I_DUPW:  f << "dupw"; break;
    case I_DUPW2: f << "dupw2"; break;
    case I_DUPW3: f << "dupw3"; break;
    case I_DUPW4: f << "dupw4"; break;
    case I_DUPW5: f << "dupw5"; break;
    case I_DUPW6: f << "dupw6"; break;
    case I_DUPW7: f << "dupw7"; break;
    case I_DUPW8: f << "dupw8"; break;
    case I_GETL:  f << "getl  " << instr.imm; break;
    case I_GETL2: f << "getl2 " << instr.imm; break;
    case I_GETLN: f << "getln " << instr.imm; break;
    case I_SETL:  f << "setl  " << instr.imm; break;
    case I_SETLN: f << "setln " << instr.imm; break;
    case I_GETG:  f << "getg  " << instr.label; break;
    case I_GETGN: f << "getgn " << instr.label; break;
    case I_SETG:  f << "setg  " << instr.label; break;
    case I_SETGN: f << "setgn " << instr.label; break;
    case I_GETP:  f << "getp"; break;
    case I_GETPN: f << "getpn " << instr.imm; break;
    case I_GETR:  f << "getr"; break;
    case I_GETR2: f << "getr2"; break;
    case I_GETRN: f << "getrn " << instr.imm; break;
    case I_SETR:  f << "setr"; break;
    case I_SETR2: f << "setr2"; break;
    case I_SETRN: f << "setrn " << instr.imm; break;
    case I_POP:   f << "pop"; break;
    case I_POP2:  f << "pop2"; break;
    case I_POP3:  f << "pop3"; break;
    case I_POP4:  f << "pop4"; break;

    case I_AIDXB: f << "aidxb " << instr.imm << " " << instr.imm2; break;
    case I_AIDX:  f << "aidx  " << instr.imm << " " << instr.imm2; break;

    case I_PIDXB: f << "pidxb " << instr.imm << " " << instr.imm2; break;
    case I_PIDX:  f << "pidx  " << instr.imm << " " << instr.imm2; break;

    case I_UAIDX: f << "uaidx " << instr.imm; break;
    case I_UPIDX: f << "upidx " << instr.imm; break;

    case I_REFL:  f << "refl  " << instr.imm; break;
    case I_REFG:  f << "refg  " << instr.label << " " << instr.imm; break;
    case I_REFGB: assert(false); break;

    case I_INC:   f << "inc"; break;
    case I_LINC:  f << "linc  " << instr.imm; break;

    case I_ADD:   f << "add"; break;
    case I_ADD2:  f << "add2"; break;
    case I_ADD3:  f << "add3"; break;
    case I_ADD4:  f << "add4"; break;
    case I_SUB:   f << "sub"; break;
    case I_SUB2:  f << "sub2"; break;
    case I_SUB3:  f << "sub3"; break;
    case I_SUB4:  f << "sub4"; break;

    case I_MUL:   f << "mul"; break;
    case I_MUL2:  f << "mul2"; break;
    case I_MUL3:  f << "mul3"; break;
    case I_MUL4:  f << "mul4"; break;

    case I_UDIV2: f << "udiv2"; break;
    case I_UDIV4: f << "udiv4"; break;
    case I_DIV2:  f << "div2"; break;
    case I_DIV4:  f << "div4"; break;

    case I_UMOD2: f << "umod2"; break;
    case I_UMOD4: f << "umod4"; break;
    case I_MOD2:  f << "mod2"; break;
    case I_MOD4:  f << "mod4"; break;

    case I_LSL:   f << "lsl"; break;
    case I_LSL2:  f << "lsl2"; break;
    case I_LSL3:  f << "lsl3"; break;
    case I_LSL4:  f << "lsl4"; break;
    case I_LSR:   f << "lsr"; break;
    case I_LSR2:  f << "lsr2"; break;
    case I_LSR3:  f << "lsr3"; break;
    case I_LSR4:  f << "lsr4"; break;
    case I_ASR:   f << "asr"; break;
    case I_ASR2:  f << "asr2"; break;
    case I_ASR3:  f << "asr3"; break;
    case I_ASR4:  f << "asr4"; break;

    case I_AND:   f << "and"; break;
    case I_AND2:  f << "and2"; break;
    case I_AND3:  f << "and3"; break;
    case I_AND4:  f << "and4"; break;
    case I_OR:    f << "or"; break;
    case I_OR2:   f << "or2"; break;
    case I_OR3:   f << "or3"; break;
    case I_OR4:   f << "or4"; break;
    case I_XOR:   f << "xor"; break;
    case I_XOR2:  f << "xor2"; break;
    case I_XOR3:  f << "xor3"; break;
    case I_XOR4:  f << "xor4"; break;
    case I_COMP:  f << "comp"; break;
    case I_COMP2: f << "comp2"; break;
    case I_COMP3: f << "comp3"; break;
    case I_COMP4: f << "comp4"; break;

    case I_BOOL:  f << "bool"; break;
    case I_BOOL2: f << "bool2"; break;
    case I_BOOL3: f << "bool3"; break;
    case I_BOOL4: f << "bool4"; break;
    case I_CULT:  f << "cult"; break;
    case I_CULT2: f << "cult2"; break;
    case I_CULT3: f << "cult3"; break;
    case I_CULT4: f << "cult4"; break;
    case I_CULE:  f << "cule"; break;
    case I_CULE2: f << "cule2"; break;
    case I_CULE3: f << "cule3"; break;
    case I_CULE4: f << "cule4"; break;
    case I_CSLT:  f << "cslt"; break;
    case I_CSLT2: f << "cslt2"; break;
    case I_CSLT3: f << "cslt3"; break;
    case I_CSLT4: f << "cslt4"; break;
    case I_CSLE:  f << "csle"; break;
    case I_CSLE2: f << "csle2"; break;
    case I_CSLE3: f << "csle3"; break;
    case I_CSLE4: f << "csle4"; break;
    case I_NOT:   f << "not"; break;
    case I_BZ:    f << "bz    " << instr.label; break;
    case I_BNZ:   f << "bnz   " << instr.label; break;
    case I_JMP:   f << "jmp   " << instr.label; break;
    case I_CALL:  f << "call  " << instr.label; break;
    case I_RET:   f << "ret"; break;
    case I_SYS:
    case I_SYSB:
    {
        f << (instr.instr == I_SYS ? "sys   " : "sysb  ");
        std::string_view sys = "???";
        // TODO: make this faster?
        for(auto const& [k, v] : sys_names)
            if(instr.imm == v)
                sys = k;
        assert(sys != "???");
        f << sys;
        break;
    }
    default:
        assert(false);
        f << "???"; break;
    }
    f << "\n";
}

void compiler_t::write(std::ostream& f)
{
    for(auto const& [name, global] : globals)
    {
        if(global.is_constexpr_ref() ||
            global.var.is_constexpr ||
            global.var.type.is_prog) continue;
        f << ".global " << name << " " << global.var.type.prim_size << "\n";
    }

    f << "\n";

    for(auto const& [label, pd] : progdata)
    {
        f << label << ":\n";
        size_t rp = 0;
        size_t rg = 0;
        for(size_t i = 0; i < pd.data.size(); ++i)
        {
            if(rp < pd.relocs_prog.size() && pd.relocs_prog[rp].first == i)
            {
                f << "  .rp " << pd.relocs_prog[rp].second << "\n";
                rp += 1;
                i += 2;
                continue;
            }
            if(rg < pd.relocs_glob.size() && pd.relocs_glob[rg].first == i)
            {
                f << "  .rg " << pd.relocs_glob[rg].second << "\n";
                rg += 1;
                i += 1;
                continue;
            }
            f << "  .b " << (int)pd.data[i] << "\n";
        }
    }

    f << "\n";

    for(auto const& [name, func] : funcs)
    {
        uint16_t line = 0;
        f << name << ":\n";
        f << "  .file " << func.filename << "\n";
        for(auto const& instr : func.instrs)
            write_instr(f, instr, line);
        f << "\n";
    }
}

}
