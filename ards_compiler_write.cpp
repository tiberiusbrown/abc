#include "ards_compiler.hpp"

#include <assert.h>

namespace ards
{

static void write_instr(std::ostream& f, compiler_instr_t const& instr)
{
    if(instr.is_label)
    {
        f << instr.label << ":\n";
        return;
    }
    f << "  ";
    switch(instr.instr)
    {
    case I_NOP:   f << "nop"; break;
    case I_PUSH:  f << "push  " << instr.imm; break;
    case I_SEXT:  f << "sext"; break;
    case I_GETL:  f << "getl  " << instr.imm; break;
    case I_GETLN: f << "getln " << instr.imm; break;
    case I_SETL:  f << "setl  " << instr.imm; break;
    case I_SETLN: f << "setln " << instr.imm; break;
    case I_GETG:  f << "getg  " << instr.label; break;
    case I_GETGN: f << "getgn " << instr.label; break;
    case I_SETG:  f << "setg  " << instr.label; break;
    case I_SETGN: f << "setgn " << instr.label; break;
    case I_POP:   f << "pop"; break;
    case I_ADD:   f << "add"; break;
    case I_ADD2:  f << "add2"; break;
    case I_ADD3:  f << "add3"; break;
    case I_ADD4:  f << "add4"; break;
    case I_SUB:   f << "sub"; break;
    case I_SUB2:  f << "sub2"; break;
    case I_SUB3:  f << "sub3"; break;
    case I_SUB4:  f << "sub4"; break;
    case I_CPNE:  f << "cpne"; break;
    case I_CPNE2: f << "cpne2"; break;
    case I_CPNE3: f << "cpne3"; break;
    case I_CPNE4: f << "cpne4"; break;
    case I_NOT:   f << "not"; break;
    case I_BZ:    f << "bz    " << instr.label; break;
    case I_BNZ:   f << "bnz   " << instr.label; break;
    case I_JMP:   f << "jmp   " << instr.label; break;
    case I_CALL:  f << "call  " << instr.label; break;
    case I_RET:   f << "ret"; break;
    case I_SYS:
    {
        // TODO: make this faster?
        f << "sys   ";
        std::string_view sys = "???";
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
        f << ".global " << name << " " << global.type.prim_size << "\n";
    }

    f << "\n";

    for(auto const& [name, func] : funcs)
    {
        f << name << ":\n";
        for(auto const& instr : func.instrs)
            write_instr(f, instr);
        f << "\n";
    }
}

}
