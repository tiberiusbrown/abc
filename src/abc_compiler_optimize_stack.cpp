#include "abc_compiler.hpp"

namespace abc
{

bool compiler_t::optimize_stack_func(std::vector<compiler_instr_t>& instrs)
{
    bool t = false;
    clear_removed_instrs(instrs);
    for(size_t i = 0; i < instrs.size(); ++i)
    {
        (void)instr_stack_mod(instrs[i]);
        (void)instr_accesses_stack(instrs[i], 1);
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
