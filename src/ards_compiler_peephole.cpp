#include "ards_compiler.hpp"

#include <algorithm>

namespace ards
{

static void clear_removed_instrs(std::vector<compiler_instr_t>& instrs)
{
    auto end = std::remove_if(
        instrs.begin(), instrs.end(),
        [](compiler_instr_t const& i) { return i.instr == I_REMOVE; }
    );
    instrs.erase(end, instrs.end());
}

void compiler_t::peephole(compiler_func_t& f)
{
    while(peephole_pre_push_compress(f))
        ;
    while(peephole_compress_push_pop(f))
        ;
    while(peephole_compress_duplicate_pushes(f))
        ;
}

bool compiler_t::peephole_pre_push_compress(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];

        // replace GETRN 1 with GETR
        // replace GETRN 2 with GETR2
        if(i0.instr == I_GETRN)
        {
            if(i0.imm == 1)
            {
                i0.instr = I_GETR;
                t = true;
                continue;
            }
            if(i0.imm == 2)
            {
                i0.instr = I_GETR2;
                t = true;
                continue;
            }
        }
        
        // replace SETRN 1 with SETR
        // replace SETRN 2 with SETR2
        if(i0.instr == I_SETRN)
        {
            if(i0.imm == 1)
            {
                i0.instr = I_SETR;
                t = true;
                continue;
            }
            if(i0.imm == 2)
            {
                i0.instr = I_SETR2;
                t = true;
                continue;
            }
        }

        // replace GETL 1 with DUP
        if(i0.instr == I_GETL && i0.imm == 1)
        {
            i0.instr = I_DUP;
            t = true;
            continue;
        }

        // replace REFG N with REFGB N (N < 256)
        if(i0.instr == I_REFG && i0.imm < 256)
        {
            i0.instr = I_REFGB;
            t = true;
            continue;
        }

        if(i + 1 >= f.instrs.size()) continue;
        auto& i1 = f.instrs[i + 1];

        // replace NOT; NOT with BOOL
        if(i0.instr == I_NOT && i1.instr == I_NOT)
        {
            i1.instr = I_REMOVE;
            i0.instr = I_BOOL;
            t = true;
            continue;
        }

        // replace BOOL; NOT with NOT
        if(i0.instr == I_BOOL && i1.instr == I_NOT)
        {
            i0.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace BOOL; BZ|BNZ with BZ|BNZ
        if(i0.instr == I_BOOL && (i1.instr == I_BZ || i1.instr == I_BNZ))
        {
            i0.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace NOT; BZ with BNZ
        // replace NOT; BNZ with BZ
        if(i0.instr == I_NOT && (i1.instr == I_BZ || i1.instr == I_BNZ))
        {
            i0.instr = I_REMOVE;
            i1.instr = (i1.instr == I_BZ ? I_BNZ : I_BZ);
            t = true;
            continue;
        }

        // remove PUSH <N>; BZ (N != 0)
        if(i0.instr == I_PUSH && i0.imm != 0 && i1.instr == I_BZ)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // remove PUSH 0; BNZ
        if(i0.instr == I_PUSH && i0.imm == 0 && i1.instr == I_BNZ)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH <N>; BNZ with JMP (N != 0)
        if(i0.instr == I_PUSH && i0.imm != 0 && i1.instr == I_BNZ)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_JMP;
            t = true;
            continue;
        }

        // replace PUSH 0; BZ with JMP
        if(i0.instr == I_PUSH && i0.imm == 0 && i1.instr == I_BZ)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_JMP;
            t = true;
            continue;
        }

        // replace PUSH N; BOOL with PUSH N (N == 0 or 1)
        if(i0.instr == I_PUSH && i0.imm <= 1 && i1.instr == I_BOOL)
        {
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH N; NOT with PUSH !N
        if(i0.instr == I_PUSH && i1.instr == I_NOT)
        {
            i1.instr = I_REMOVE;
            i0.imm = (i0.imm == 0 ? 1 : 0);
            t = true;
            continue;
        }

        // remove PUSH N; POP
        if(i0.instr == I_PUSH && i1.instr == I_POP)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH N; SEXT with PUSH N; PUSH <0 or 255>
        if(i0.instr == I_PUSH && i1.instr == I_SEXT)
        {
            if(i0.imm < 128)
                i1 = { I_PUSH, 0 };
            else
                i1 = { I_PUSH, 255 };
            t = true;
            continue;
        }

        // replace PUSH 1; ADD with INC
        if(i0.instr == I_PUSH && i0.imm == 1 && i1.instr == I_ADD)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_INC;
            t = true;
            continue;
        }

        // replace PUSH 1; GETLN <N> with GETL <N>
        // replace PUSH 1; SETLN <N> with SETL <N>
        // replace PUSH 1; GETGN <N> with GETG <N>
        // replace PUSH 1; SETGN <N> with SETG <N>
        if(i0.instr == I_PUSH && i0.imm == 1)
        {
            if(i1.instr == I_GETLN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_GETL;
                t = true;
                continue;
            }
            if(i1.instr == I_SETLN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_SETL;
                t = true;
                continue;
            }
            if(i1.instr == I_GETGN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_GETG;
                t = true;
                continue;
            }
            if(i1.instr == I_SETGN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_SETG;
                t = true;
                continue;
            }
        }

        // replace PUSH 2; GETLN <N> with GETL2 <N>
        if(i0.instr == I_PUSH && i0.imm == 2)
        {
            if(i1.instr == I_GETLN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_GETL2;
                t = true;
                continue;
            }
        }

        // replace PUSH 0; AIDX M N (M,N < 256) with AIDXB M N
        if(i0.instr == I_PUSH && i0.imm == 0 &&
            i1.instr == I_AIDX && i1.imm < 256 && i1.imm2 < 256)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_AIDXB;
            t = true;
            continue;
        }
    }
    return t;
}

bool compiler_t::peephole_compress_push_pop(compiler_func_t & f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        if(i + 1 >= f.instrs.size()) continue;
        auto& i1 = f.instrs[i + 1];

        // replace PUSH 0 with P0
        if(i0.instr == I_PUSH)
        {
            if(i0.imm == 0 && i1.instr == I_PUSH && i1.imm == 0)
            {
                i0.instr = I_P00;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            static std::unordered_map<uint32_t, instr_t> const push_instrs =
            {
                { 0, I_P0 },
                { 1, I_P1 },
                { 2, I_P2 },
                { 3, I_P3 },
                { 4, I_P4 },
            };
            auto it = push_instrs.find(i0.imm);
            if(it != push_instrs.end())
            {
                i0.instr = it->second;
                t = true;
                continue;
            }
        }

        // replace POP; POP with POP2
        if(i0.instr == I_POP && i1.instr == I_POP)
        {
            i0.instr = I_POP2;
            i1.instr = I_REMOVE;
            if(i + 2 < f.instrs.size() && f.instrs[i + 2].instr == I_POP)
            {
                f.instrs[i + 2].instr = I_REMOVE;
                i0.instr = I_POP3;
                if(i + 3 < f.instrs.size() && f.instrs[i + 3].instr == I_POP)
                {
                    f.instrs[i + 3].instr = I_REMOVE;
                    i0.instr = I_POP4;
                }
            }
            t = true;
            continue;
        }
    }
    return t;
}

bool compiler_t::peephole_compress_duplicate_pushes(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];

        // replace PUSH N; PUSH N ... with PUSH N; dup ...
        if(i0.instr == I_PUSH)
        {
            auto imm = i0.imm;
            for(size_t j = i + 1; j < f.instrs.size(); ++j)
            {
                auto& ti = f.instrs[j];
                if(ti.instr != I_PUSH || ti.imm != imm) break;
                ti.instr = I_DUP;
                t = true;
            }
            if(t) continue;
        }
    }
    return t;
}

}
