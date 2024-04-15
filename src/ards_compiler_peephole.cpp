#include "ards_compiler.hpp"

#include <algorithm>

namespace ards
{

void compiler_t::clear_removed_instrs(std::vector<compiler_instr_t>& instrs)
{
    auto end = std::remove_if(
        instrs.begin(), instrs.end(),
        [](compiler_instr_t const& i) { return i.instr == I_REMOVE; }
    );
    instrs.erase(end, instrs.end());
}

void compiler_t::peephole(compiler_func_t& f)
{
    while(peephole_bake_offsets(f))
        ;
    while(peephole_bake_getpn(f))
        ;
    while(peephole_remove_pop(f))
        ;
    while(peephole_simplify_derefs(f))
        ;
    while(peephole_arithmetic(f))
        ;
    while(peephole_dup_setln(f))
        ;
    while(peephole_pre_push_compress(f))
        ;
    while(peephole_ref(f))
        ;
    while(peephole_linc(f))
        ;
    while(peephole_dup_sext(f))
        ;
    while(peephole_bzp(f))
        ;
    while(peephole_redundant_bzp(f))
        ;
    while(peephole_compress_pop(f))
        ;
    while(peephole_compress_push(f))
        ;
    while(peephole_compress_pushes_pushn(f))
        ;
    while(peephole_compress_duplicate_pushes(f))
        ;
}

bool compiler_t::peephole_bake_getpn(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];

        // replace PUSHL <LABEL>; GETPN <N> with a bunch of pushes
        if(i0.instr == I_PUSHL && i1.instr == I_GETPN && i1.imm <= 16)
        {
            // locate label
            auto offset = i0.imm;
            auto n = i1.imm;
            auto it = progdata.find(i0.label);
            if(it == progdata.end())
                continue;
            auto const& d = it->second.data;
            if(offset + n > d.size())
                continue;
            bool valid = true;
            for(auto const& p : it->second.relocs_glob)
                if(p.first >= offset && p.first < offset + n)
                    valid = false;
            for(auto const& p : it->second.relocs_prog)
                if(p.first >= offset && p.first < offset + n)
                    valid = false;
            if(!valid)
                continue;

            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            
            f.instrs.insert(f.instrs.begin() + i, n, { I_PUSH, i0.line });

            // i0, i1 invalidated now

            for(size_t j = 0; j < n; ++j)
                f.instrs[i + j].imm = d[offset + j];
            t = true;
            continue;
        }
    }

    return t;
}

bool compiler_t::peephole_remove_pop(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];

        // replace GETPN <N>; POP with GETPN <N-1>
        if(i0.instr == I_GETPN && i0.imm > 1 && i1.instr == I_POP)
        {
            i0.imm -= 1;
            i1.instr = I_REMOVE;
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
            i1.instr = I_PUSH;
            i1.imm = (i0.imm < 128 ? 0 : 255);
            t = true;
            continue;
        }

        if(i + 2 >= f.instrs.size()) continue;
        auto& i2 = f.instrs[i + 2];

        // replace PUSH <M>; GETLN <N>; POP with PUSH <M-1>; GETLN <N>
        if(i0.instr == I_PUSH && i1.instr == I_GETLN && i1.imm > 1 && i2.instr == I_POP)
        {
            i0.imm -= 1;
            i2.instr = I_REMOVE;
            t = true;
            continue;
        }
    }

    return t;
}

bool compiler_t::peephole_linc(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 2 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];

        if(i0.instr == I_GETL && i1.instr == I_INC && i2.instr == I_SETL &&
            i0.imm == i2.imm)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_LINC;
            if(i2.imm == 1)
                i2.instr = I_INC;
            t = true;
            continue;
        }
    }

    return t;
}

bool compiler_t::peephole_ref(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 3 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];
        auto& i3 = f.instrs[i + 3];

        // replace REFG N; PUSH A0; PUSH A1; ADD2 with REFG N+A
        if( i0.instr == I_PUSHG &&
            i1.instr == I_PUSH && i2.instr == I_PUSH &&
            i3.instr == I_ADD2)
        {
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            i0.imm = i1.imm + i2.imm * 256;
            t = true;
            continue;
        }

        // remove REFG N; POP; POP
        if(i0.instr == I_PUSHG &&
            i1.instr == I_POP &&
            i2.instr == I_POP)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            t = true;
            continue;
        }
    }

    return t;
}

bool compiler_t::peephole_dup_sext(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];

        // replace GETL <N> with DUP<N>
        if(i0.instr == I_GETL && i0.imm >= 1 && i0.imm <= 8)
        {
            i0.instr = instr_t(I_DUP + i0.imm - 1);
            t = true;
            continue;
        }

        // replace GETL2 <N> with DUPW<N-1>
        if(i0.instr == I_GETL2 && i0.imm >= 2 && i0.imm <= 9)
        {
            i0.instr = instr_t(I_DUPW + i0.imm - 2);
            t = true;
            continue;
        }

        if(i + 1 >= f.instrs.size()) continue;
        auto& i1 = f.instrs[i + 1];

        // replace DUP<N>; DUP<N> with DUPW<N-1>
        if(i0.instr >= I_DUP2 && i0.instr <= I_DUP8 && i0.instr == i1.instr)
        {
            i0.instr = instr_t(i0.instr + I_DUPW - I_DUP - 1);
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        if(i + 2 >= f.instrs.size()) continue;
        auto& i2 = f.instrs[i + 2];

        // replace SEXT; SEXT; SEXT with SEXT3
        // replace SEXT; SEXT;      with SEXT2
        if(i0.instr == I_SEXT && i1.instr == I_SEXT)
        {
            i0.instr = I_SEXT2;
            i1.instr = I_REMOVE;
            if(i2.instr == I_SEXT)
            {
                i0.instr = I_SEXT3;
                i2.instr = I_REMOVE;
            }
            t = true;
            continue;
        }
    }

    return t;
}

bool compiler_t::peephole_simplify_derefs(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];

        // replace local derefs with GETLN
        if(i0.instr == I_REFL && i1.instr == I_GETRN)
        {
            i0.instr = I_PUSH;
            i1.instr = I_GETLN;
            std::swap(i0.imm, i1.imm);
            t = true;
            continue;
        }

        // replace local derefs with SETLN
        if(i0.instr == I_REFL && i1.instr == I_SETRN)
        {
            i0.instr = I_PUSH;
            i1.instr = I_SETLN;
            std::swap(i0.imm, i1.imm);
            i1.imm -= i0.imm;
            t = true;
            continue;
        }

        // replace global derefs with GETGN
        if(i0.instr == I_PUSHG && i1.instr == I_GETRN)
        {
            i0.instr = I_PUSH;
            i1.instr = I_GETGN;
            i1.label = std::move(i0.label);
            i0.label.clear();
            std::swap(i0.imm, i1.imm);
            t = true;
            continue;
        }

        // replace global derefs with SETGN
        if(i0.instr == I_PUSHG && i1.instr == I_SETRN)
        {
            i0.instr = I_PUSH;
            i1.instr = I_SETGN;
            i1.label = std::move(i0.label);
            i0.label.clear();
            std::swap(i0.imm, i1.imm);
            t = true;
            continue;
        }

        // replace prog derefs with GETPN
        //if(i0.instr == I_REFL && i1.instr == I_GETRN)
        //{
        //    i0.instr = I_PUSH;
        //    i1.instr = I_GETPN;
        //    i1.label = std::move(i0.label);
        //    i0.label.clear();
        //    std::swap(i0.imm, i1.imm);
        //    t = true;
        //    continue;
        //}

        // replace GETPN <N>; POP with GETPN <N-1>
        if(i0.instr == I_GETPN && i1.instr == I_POP)
        {
            assert(i0.imm > 0);
            if(i0.imm == 1)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_REMOVE;
            }
            else
            {
                i0.imm -= 1;
                i1.instr = I_REMOVE;
            }
            t = true;
            continue;
        }

        if(i + 2 >= f.instrs.size()) continue;
        auto& i2 = f.instrs[i + 2];

        // replace PUSH <N>; GETLN <M>; POP with PUSH <N-1>; GETLN <M>
        // replace PUSH <N>; GETGN <M>; POP with PUSH <N-1>; GETGN <M>
        if(i0.instr == I_PUSH && i2.instr == I_POP &&
            (i1.instr == I_GETLN || i1.instr == I_GETGN))
        {
            assert(i0.imm > 0);
            if(i0.imm == 1)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
            }
            else
            {
                i0.imm -= 1;
                i2.instr = I_REMOVE;
            }
            t = true;
            continue;
        }

        if(i + 3 >= f.instrs.size()) continue;
        auto& i3 = f.instrs[i + 3];

        // combine consecutive GETLNs to adjacant stack locations
        if( i0.instr == I_PUSH && i1.instr == I_GETLN &&
            i2.instr == I_PUSH && i3.instr == I_GETLN &&
            i0.imm + i1.imm == i2.imm + i3.imm)
        {
            i0.imm += i2.imm;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            t = true;
            continue;
        }

    }
    return t;
}

bool compiler_t::peephole_bake_offsets(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 3 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];
        auto& i3 = f.instrs[i + 3];

        // convert REFL N; PUSH ..; PUSH ..; ADD2 to REFL N-..
        // convert REFG N; PUSH ..; PUSH ..; ADD2 to REFG N+..
        if(i1.instr == I_PUSH && i2.instr == I_PUSH && i3.instr == I_ADD2)
        {
            if(i0.instr == I_REFL)
            {
                assert(i2.imm == 0);
                i0.imm -= i1.imm;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                i3.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i0.instr == I_PUSHG)
            {
                i0.imm += i1.imm;
                i0.imm += i2.imm * 256;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                i3.instr = I_REMOVE;
                t = true;
                continue;
            }
        }

        if(i + 4 >= f.instrs.size()) continue;
        auto& i4 = f.instrs[i + 4];

        // convert PUSHL <LABEL>; PUSH ..; PUSH ..; PUSH ...; ADD3 to PUSHL LABEL+..
        if( i0.instr == I_PUSHL &&
            i1.instr == I_PUSH &&
            i2.instr == I_PUSH &&
            i3.instr == I_PUSH &&
            i4.instr == I_ADD3)
        {
            i0.imm += i1.imm;
            i0.imm += i2.imm * (1 << 8);
            i0.imm += i3.imm * (1 << 16);
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            i4.instr = I_REMOVE;
            t = true;
            continue;
        }
    }
    return t;
}

bool compiler_t::peephole_pre_push_compress(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];

        // replace GETPN 1 with GETP
        if(i0.instr == I_GETPN)
        {
            if(i0.imm == 1)
            {
                i0.instr = I_GETP;
                t = true;
                continue;
            }
        }

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

        if(i + 1 >= f.instrs.size()) continue;
        auto& i1 = f.instrs[i + 1];

        if(i0.instr == I_REFL)
        {
            if(i1.instr == I_GETR)
            {
                i0.instr = I_GETL;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i1.instr == I_GETR2)
            {
                i0.instr = I_GETL2;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i1.instr == I_GETRN && i1.imm == 4)
            {
                i0.instr = I_GETL4;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i1.instr == I_GETRN && i1.imm != 4)
            {
                auto offset = i0.imm;
                auto size = i1.imm;
                i0.instr = I_PUSH;
                i0.imm = size;
                i1.instr = I_GETLN;
                i1.imm = offset;
                t = true;
                continue;
            }
        }

        if(i0.instr == I_PUSHG)
        {
            if(i1.instr == I_GETR)
            {
                i0.instr = I_GETG;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i1.instr == I_GETR2)
            {
                i0.instr = I_GETG2;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i1.instr == I_GETRN && i1.imm == 4)
            {
                i0.instr = I_GETG4;
                i1.instr = I_REMOVE;
                t = true;
                continue;
            }
            if(i1.instr == I_GETRN && i1.imm != 4)
            {
                auto label = i0.label;
                auto offset = i0.imm;
                auto size = i1.imm;
                i0.instr = I_PUSH;
                i0.imm = size;
                i1.instr = I_GETGN;
                i1.label = label;
                i1.imm = offset;
                t = true;
                continue;
            }
        }

        // replace NOT; NOT with BOOL
        if(i0.instr == I_NOT && i1.instr == I_NOT)
        {
            i1.instr = I_REMOVE;
            i0.instr = I_BOOL;
            t = true;
            continue;
        }

        // replace BOOL; BOOL with BOOL
        if(i0.instr == I_BOOL && i1.instr == I_BOOL)
        {
            i1.instr = I_REMOVE;
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

        // replace NOT; BOOL with NOT
        if(i0.instr == I_NOT && i1.instr == I_BOOL)
        {
            i1.instr = I_REMOVE;
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

        // replace PUSH 1; ADD with INC
        if(i0.instr == I_PUSH && i0.imm == 1 && i1.instr == I_ADD)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_INC;
            t = true;
            continue;
        }

        // replace PUSH 1; SUB with DEC
        if(i0.instr == I_PUSH && i0.imm == 1 && i1.instr == I_SUB)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_DEC;
            t = true;
            continue;
        }

        // replace PUSH 0; ADD2 with ADD2B
        if(i0.instr == I_PUSH && i0.imm == 0 && i1.instr == I_ADD2)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_ADD2B;
            t = true;
            continue;
        }

        // replace PUSH 0; SUB2 with SUB2B
        if(i0.instr == I_PUSH && i0.imm == 0 && i1.instr == I_SUB2)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_SUB2B;
            t = true;
            continue;
        }

        // replace PUSH 0; MUL2 with MUL2B
        if(i0.instr == I_PUSH && i0.imm == 0 && i1.instr == I_MUL2)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_MUL2B;
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
            if(i1.instr == I_SETLN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_SETL2;
                t = true;
                continue;
            }
            if(i1.instr == I_GETGN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_GETG2;
                t = true;
                continue;
            }
            if(i1.instr == I_SETGN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_SETG2;
                t = true;
                continue;
            }
        }

        // replace PUSH 4; GETLN <N> with GETL4 <N>
        if(i0.instr == I_PUSH && i0.imm == 4)
        {
            if(i1.instr == I_GETLN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_GETL4;
                t = true;
                continue;
            }
            if(i1.instr == I_SETLN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_SETL4;
                t = true;
                continue;
            }
            if(i1.instr == I_GETGN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_GETG4;
                t = true;
                continue;
            }
            if(i1.instr == I_SETGN)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_SETG4;
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
            if(i1.imm == 1)
            {
                i1.imm = i1.imm2;
                i1.instr = I_AIXB1;
            }
            t = true;
            continue;
        }

        // remove JMP <LABEL>; LABEL:
        if(i0.instr == I_JMP && i1.is_label && i0.label == i1.label)
        {
            i0.instr = I_REMOVE;
            t = true;
            continue;
        }

        if(i + 2 >= f.instrs.size()) continue;
        auto& i2 = f.instrs[i + 2];

        // replace PUSH 1; AND; BOOL with PUSH 1; AND
        if(i0.instr == I_PUSH && i0.imm == 1 &&
            i1.instr == I_AND &&
            i2.instr == I_BOOL)
        {
            i2.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH 0; PUSH 0; ADD3 with ADD3B
        if( i0.instr == I_PUSH && i0.imm == 0 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            i2.instr == I_ADD3)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_ADD3B;
            t = true;
            continue;
        }

        // replace PUSH N (> 0); PUSH 255; ADD2 with PUSH -N; SUB2B
        if(i0.instr == I_PUSH && i0.imm != 0 &&
            i1.instr == I_PUSH && i1.imm == 255 &&
            (i2.instr == I_ADD2 || i2.instr == I_SUB2))
        {
            i0.imm = uint8_t(-(int32_t)i0.imm);
            i1.instr = I_REMOVE;
            i2.instr = (i2.instr == I_ADD2 ? I_SUB2B : I_ADD2B);
            t = true;
            continue;
        }

        // replace PUSH 0; PUSH 0; PIDX M N (M,N < 256) with PIDXB M N
        if( i0.instr == I_PUSH && i0.imm == 0 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            i2.instr == I_PIDX && i2.imm < 256 && i2.imm2 < 256)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_PIDXB;
            t = true;
            continue;
        }

        // replace BZ <L1>; JMP <L2>; <L1>: with BNZ <L2>
        // replace BNZ <L1>; JMP <L2>; <L1>: with BZ <L2>
        if(i0.label == i2.label && i1.instr == I_JMP && i2.is_label)
        {
            if(i0.instr == I_BZ)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_BNZ;
                t = true;
                continue;
            }
            if(i0.instr == I_BNZ)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_BZ;
                t = true;
                continue;
            }
        }
    }
    return t;
}

bool compiler_t::peephole_arithmetic(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];

        // remove PUSH 0; ADD/SUB
        if(i0.instr == I_PUSH && i0.imm == 0 &&
            (i1.instr == I_ADD || i1.instr == I_SUB))
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // remove PUSH 1; MUL
        if(i0.instr == I_PUSH && i0.imm == 1 && i1.instr == I_MUL)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        if(i + 2 >= f.instrs.size()) continue;
        auto& i2 = f.instrs[i + 2];

        // remove PUSH 0; PUSH 0; ADD2/SUB2
        if(i0.instr == I_PUSH && i0.imm == 0 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            (i2.instr == I_ADD2 || i2.instr == I_SUB2))
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            t = true;
            continue;
        }

        // remove PUSH 1; PUSH 0; MUL2
        if(i0.instr == I_PUSH && i0.imm == 1 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            i2.instr == I_MUL2)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            t = true;
            continue;
        }

        if(i + 3 >= f.instrs.size()) continue;
        auto& i3 = f.instrs[i + 3];

        // remove PUSH 0; PUSH 0; PUSH 0; ADD3/SUB3
        if(i0.instr == I_PUSH && i0.imm == 0 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            i2.instr == I_PUSH && i2.imm == 0 &&
            (i3.instr == I_ADD3 || i3.instr == I_SUB3))
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            t = true;
            continue;
        }

        if(i + 4 >= f.instrs.size()) continue;
        auto& i4 = f.instrs[i + 4];

        // remove PUSH 0; PUSH 0; PUSH 0; PUSH 0; ADD4/SUB4
        if(i0.instr == I_PUSH && i0.imm == 0 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            i2.instr == I_PUSH && i2.imm == 0 &&
            i3.instr == I_PUSH && i3.imm == 0 &&
            (i4.instr == I_ADD4 || i4.instr == I_SUB4))
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            i4.instr = I_REMOVE;
            t = true;
            continue;
        }

        // remove PUSH 1; PUSH 0; PUSH 0; PUSH 0; MUL4
        if(i0.instr == I_PUSH && i0.imm == 1 &&
            i1.instr == I_PUSH && i1.imm == 0 &&
            i2.instr == I_PUSH && i2.imm == 0 &&
            i3.instr == I_PUSH && i3.imm == 0 &&
            i4.instr == I_MUL2)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            i4.instr = I_REMOVE;
            t = true;
            continue;
        }

    }
    return t;
}

bool compiler_t::peephole_dup_setln(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 4 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];
        auto& i3 = f.instrs[i + 3];

        // replace:
        //    PUSH <N>
        //    GETLN <N>
        //    PUSH <N>
        //    SETLN <M>
        //    POP (N times)
        // with:
        //    PUSH <N>
        //    SETLN <M-N>

        if( i0.instr == I_PUSH && i1.instr == I_GETLN &&
            i2.instr == I_PUSH && i3.instr == I_SETLN &&
            i0.imm == i1.imm && i0.imm == i2.imm && i3.imm > i0.imm)
        {
            auto n = i0.imm;

            if(i + 4 + n >= f.instrs.size()) continue;

            bool valid = true;
            for(size_t j = i + 4; j < i + 4 + n; ++j)
            {
                if(f.instrs[j].instr != I_POP)
                {
                    valid = false;
                    break;
                }
            }
            if(!valid)
                continue;

            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i3.imm -= n;

            for(size_t j = i + 4; j < i + 4 + n; ++j)
                f.instrs[j].instr = I_REMOVE;
            
            t = true;
            continue;
        }
    }
    return t;
}

bool compiler_t::peephole_bzp(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 2 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];

        // replace DUP; B[N]Z; POP with B[N]ZP
        if(i0.instr == I_DUP && i2.instr == I_POP && (
            i1.instr == I_BZ || i1.instr == I_BNZ))
        {
            i0.instr = I_REMOVE;
            i1.instr = (i1.instr == I_BZ ? I_BZP : I_BNZP);
            i2.instr = I_REMOVE;
            t = true;
            continue;
        }
    }
    return t;
}

bool compiler_t::peephole_redundant_bzp(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i];

        // replace BZP/BNZP to a BZ/BNZ
        if(i0.instr == I_BZP || i0.instr == I_BNZP)
        {
            size_t j;
            for(j = 0; j < f.instrs.size(); ++j)
                if(f.instrs[j].is_label && f.instrs[j].label == i0.label)
                    break;
            if(j >= f.instrs.size())
                continue;
            for(; j < f.instrs.size(); ++j)
                if(!f.instrs[j].is_label)
                    break;
            if(j >= f.instrs.size())
                continue;
            if(i0.instr == I_BZP && f.instrs[j].instr == I_BZ)
            {
                i0.instr = I_BZ;
                i0.label = f.instrs[j].label;
                t = true;
            }
            else if(i0.instr == I_BNZP && f.instrs[j].instr == I_BNZ)
            {
                i0.instr = I_BNZ;
                i0.label = f.instrs[j].label;
                t = true;
            }
            else if(
                i0.instr == I_BZP && f.instrs[j].instr == I_BNZ ||
                i0.instr == I_BNZP && f.instrs[j].instr == I_BZ)
            {
                auto label = new_label(f);
                f.instrs.insert(f.instrs.begin() + j + 1, compiler_instr_t{
                    I_NOP, f.instrs[j].line, 0, 0, label, true });
                i0.instr = (i0.instr == I_BZP ? I_BZ : I_BNZ);
                i0.label = label;
                t = true;
            }
        }
    }
    return t;
}

bool compiler_t::peephole_compress_pop(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        if(i + 1 >= f.instrs.size()) continue;
        auto& i1 = f.instrs[i + 1];

        // replace POP; POP with POP2
        if(i0.instr == I_POP && i1.instr == I_POP)
        {
            size_t j;
            for(j = i + 2; j < f.instrs.size(); ++j)
            {
                if(f.instrs[j].instr != I_POP)
                    break;
            }
            j -= i;
            if(j > 4)
            {
                if(j > 255) j = 255;
                i0.instr = I_POPN;
                i0.imm = (uint8_t)j;
                for(size_t k = i + 1; k < i + j; ++k)
                    f.instrs[k].instr = I_REMOVE;
                t = true;
                continue;
            }
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

bool compiler_t::peephole_compress_push(compiler_func_t& f)
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
                size_t j;
                for(j = i + 2; j < f.instrs.size(); ++j)
                {
                    if(f.instrs[j].instr != I_PUSH || f.instrs[j].imm != 0)
                        break;
                }
                j -= i;
                if(j == 16)
                {
                    i0.instr = I_PZ16;
                    for(size_t k = i + 1; k < i + j; ++k)
                        f.instrs[k].instr = I_REMOVE;
                    t = true;
                    continue;
                }
                if(j == 8)
                {
                    i0.instr = I_PZ8;
                    for(size_t k = i + 1; k < i + j; ++k)
                        f.instrs[k].instr = I_REMOVE;
                    t = true;
                    continue;
                }
                if(j == 4)
                {
                    i0.instr = I_P0000;
                    i1.instr = I_REMOVE;
                    f.instrs[i + 2].instr = I_REMOVE;
                    f.instrs[i + 3].instr = I_REMOVE;
                    t = true;
                    continue;
                }
                if(j == 3)
                {
                    i0.instr = I_P000;
                    i1.instr = I_REMOVE;
                    f.instrs[i + 2].instr = I_REMOVE;
                    t = true;
                    continue;
                }
                if(j == 2)
                {
                    i0.instr = I_P00;
                    i1.instr = I_REMOVE;
                    t = true;
                    continue;
                }
            }
            else
            {
                static std::unordered_map<uint32_t, instr_t> const push_instrs =
                {
                    { 0, I_P0 },
                    { 1, I_P1 },
                    { 2, I_P2 },
                    { 3, I_P3 },
                    { 4, I_P4 },
                    { 5, I_P5 },
                    { 6, I_P6 },
                    { 7, I_P7 },
                    { 8, I_P8 },
                    { 16, I_P16 },
                    { 32, I_P32 },
                    { 64, I_P64 },
                    { 128, I_P128 },
                };
                auto it = push_instrs.find(i0.imm);
                if(it != push_instrs.end())
                {
                    i0.instr = it->second;
                    t = true;
                    continue;
                }
            }
        }
    }
    return t;
}

bool compiler_t::peephole_compress_pushes_pushn(compiler_func_t& f)
{
    bool t = false;
    clear_removed_instrs(f.instrs);

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        if(i0.instr != I_PUSH) continue;

        auto& i1 = f.instrs[i + 1];
        if(i1.instr != I_PUSH) continue;

        i1.instr = I_REMOVE;
        i0.instr = I_PUSH2;
        i0.imm |= (i1.imm << 8);

        if(i + 2 < f.instrs.size() && f.instrs[i + 2].instr == I_PUSH)
        {
            auto& i2 = f.instrs[i + 2];
            i2.instr = I_REMOVE;
            i0.instr = I_PUSH3;
            i0.imm |= (i2.imm << 16);
            if(i + 3 < f.instrs.size() && f.instrs[i + 3].instr == I_PUSH)
            {
                auto& i3 = f.instrs[i + 3];
                i3.instr = I_REMOVE;
                i0.instr = I_PUSH4;
                i0.imm |= (i3.imm << 24);
            }
        }

        t = true;
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
