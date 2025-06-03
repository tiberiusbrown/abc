#include "abc_compiler.hpp"

namespace abc
{

static bool is_pop(compiler_instr_t const& i)
{
    switch(i.instr)
    {
    case I_POP:
    case I_POP2:
    case I_POP3:
    case I_POP4:
    case I_POPN:
        return true;
    default:
        return false;
    }
}

static bool is_branch_jmp_call_ret(compiler_instr_t const& i)
{
    switch(i.instr)
    {
    case I_JMP:
    case I_JMP1:
    case I_CALL:
    case I_CALL1:
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
        return true;
    default:
        return false;
    }
}

static bool is_stack_eliminatable(compiler_instr_t const& i)
{
    switch(i.instr)
    {
    case I_DUP:
    case I_DUP2:
    case I_DUP3:
    case I_DUP4:
    case I_DUP5:
    case I_DUP6:
    case I_DUP7:
    case I_DUP8:
    case I_DUPW:
    case I_DUPW2:
    case I_DUPW3:
    case I_DUPW4:
    case I_DUPW5:
    case I_DUPW6:
    case I_DUPW7:
    case I_DUPW8:
    case I_SEXT:
    case I_SEXT2:
    case I_SEXT3:
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
    case I_REFGB:
    case I_REFL:
    //case I_GTGB:
    //case I_GTGB2:
    //case I_GTGB4:
    //case I_GETG:
    //case I_GETG2:
    //case I_GETG4:
    //case I_GETGN:
    //case I_GETL:
    //case I_GETL2:
    //case I_GETL4:
    //case I_GETLN:
    //case I_GETP:
    //case I_GETPN:
        return true;
    default:
        return false;
    }
}

bool compiler_t::optimize_stack_func(std::vector<compiler_instr_t>& instrs)
{
    bool t = false;
    clear_removed_instrs(instrs);
    for(size_t i = 0; i < instrs.size(); ++i)
    {
        auto& i0 = instrs[i];

        // eliminate PUSH ... POP that's unused in between
        if(is_stack_eliminatable(i0)) 
        {
            bool elim = false;
            int n = instr_stack_mod(i0);
            int size = n;
            if(size > 1) // TODO: support multibyte stuff
                continue;
            size_t j;
            for(j = i + 1; j < instrs.size(); ++j)
            {
                auto const& ij = instrs[j];
                if(is_pop(ij))
                {
                    bool accessed_some = false;
                    bool accessed_all = true;
                    for(int k = 0; k < size; ++k)
                    {
                        auto p = instr_accesses_stack(ij, n - k);
                        if(p.first)
                            accessed_some = true;
                        else
                            accessed_all = false;
                    }
                    if(accessed_all)
                    {
                        elim = true;
                        break;
                    }
                    if(accessed_some)
                        break;
                }
                if(ij.is_label)
                    break;
                if(is_branch_jmp_call_ret(ij))
                    break;
                bool accessed = false;
                for(int k = 0; k < size; ++k)
                {
                    auto p = instr_accesses_stack(ij, n - k);
                    if(p.first)
                        accessed = true;
                    if((int)p.second > n)
                        accessed = true;
                }
                if(accessed)
                    break;
                n += instr_stack_mod(ij);
            }
            if(elim)
            {
                i0.instr = I_REMOVE;
                auto& ipop = instrs[j];
                switch(ipop.instr)
                {
                case I_POP:  ipop.instr = I_REMOVE; break;
                case I_POP2: ipop.instr = I_POP; break;
                case I_POP3: ipop.instr = I_POP2; break;
                case I_POP4: ipop.instr = I_POP3; break;
                case I_POPN: if(--ipop.imm == 0) ipop.instr = I_REMOVE; break;
                default: assert(0); break;
                }
                t = true;
                continue;
            }
        }
    }
    return t;
}

bool compiler_t::optimize_stack()
{
    bool t = false;

    for(auto& [n, f] : funcs)
    {
        while(optimize_stack_func(f.instrs))
            t = true;
    }

    return t;
}

}
