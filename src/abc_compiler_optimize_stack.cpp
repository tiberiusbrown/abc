#include "abc_compiler.hpp"

namespace abc
{

bool compiler_t::optimize_stack_func(std::vector<compiler_instr_t>& instrs)
{
    bool t = false;
    clear_removed_instrs(instrs);
    int32_t n = 0;
    for(size_t i = 0; i < instrs.size(); ++i)
    {
        int32_t pn = n;
        n += instr_stack_mod(instrs[i]);
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
