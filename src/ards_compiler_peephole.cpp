#include "ards_compiler.hpp"

#include <algorithm>

namespace ards
{

void compiler_t::optimize()
{
    // peephole optimizations


    for(bool repeat = true; repeat;)
    {
        repeat = false;
        if(enable_inlining)
            repeat |= inline_or_remove_functions();

        // compute progdata offsets
        {
            uint32_t offset = 256;
            for(auto& [label, pd] : progdata)
            {
                pd.offset = offset;
                offset += (uint32_t)pd.data.size();
            }
        }
        // compute progdata(interlabel) offsets
        for(auto& [label, pd] : progdata)
        {
            for(auto const& i : pd.inter_labels)
            {
                if(auto it = progdata.find(i.second); it != progdata.end())
                {
                    it->second.offset = pd.offset + (uint32_t)i.first;
                }
            }
        }

        for(auto& [n, f] : funcs)
        {
            if(!errs.empty())
                return;
            while(peephole_reduce(f))
                ;
            while(peephole_reduce_bake_pushl(f))
                ;
        }
        repeat |= remove_unreferenced_labels();
        repeat |= merge_adjacent_labels();
    }

    for(bool repeat = true; repeat;)
    {
        repeat = false;
        for(auto& [n, f] : funcs)
        {
            if(!errs.empty())
                return;
            while(peephole_reduce(f))
                ;
            while(peephole(f))
                ;
        }
        repeat |= remove_unreferenced_labels();
        repeat |= merge_adjacent_labels();
    }
}

bool compiler_t::peephole(compiler_func_t& f)
{
    bool t = false;

    while(peephole_pre_push_compress(f))
        t = true;
    while(peephole_linc(f))
        t = true;
    while(peephole_dup_sext(f))
        t = true;
    while(peephole_bzp(f))
        t = true;
    while(peephole_redundant_bzp(f))
        t = true;
    while(peephole_combine_pop(f))
        t = true;
    while(peephole_compress_pop(f))
        t = true;
    while(peephole_push_setl_popn(f))
        t = true;
    if(!t)
    {
        while(peephole_compress_push_sequence(f))
            t = true;
    }

    t |= peephole_remove_inaccessible_code(f);

    if(enable_jmp_to_ret)
        while(peephole_jmp_to_ret(f))
            t = true;

    if(t) tail_call_optimization();

    return t;
}

void compiler_t::clear_removed_instrs(std::vector<compiler_instr_t>& instrs)
{
    auto end = std::remove_if(
        instrs.begin(), instrs.end(),
        [](compiler_instr_t const& i) { return i.instr == I_REMOVE; }
    );
    instrs.erase(end, instrs.end());
}

void compiler_t::tail_call_optimization()
{
    for(auto& [n, f] : funcs)
    {
        for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
        {
            auto& i0 = f.instrs[i + 0];
            auto& i1 = f.instrs[i + 1];
            if(i0.instr == I_CALL && i1.instr == I_RET)
            {
                i0.instr = I_JMP;
                i1.instr = I_REMOVE;
                clear_removed_instrs(f.instrs);
            }
        }
    }
}

bool compiler_t::remove_unreferenced_labels()
{
    std::unordered_map<std::string, int> label_counts;
    for(auto const& [n, f] : funcs)
        for(auto const& i : f.instrs)
            if(!i.is_label && !i.label.empty())
                label_counts[i.label] += 1;
    for(auto const& [k, d] : progdata)
        for(auto const& p : d.relocs_prog)
            label_counts[p.second] += 1;
    bool t = false;
    for(auto& [n, f] : funcs)
    {
        for(auto& i : f.instrs)
            if(i.is_label && label_counts[i.label] == 0)
                i.instr = I_REMOVE, t = true;
        clear_removed_instrs(f.instrs);
    }
    return t;
}

bool compiler_t::merge_adjacent_labels()
{
    bool t = false;
    for(auto& [n, f] : funcs)
    {
        for(size_t i = 0; i < f.instrs.size(); ++i)
        {
            if(!f.instrs[i].is_label) continue;
            for(size_t j = i + 1; j < f.instrs.size(); ++j)
            {
                auto& j0 = f.instrs[j];
                if(!j0.is_label)
                    break;

                for(auto& ti : f.instrs)
                    if(!ti.is_label && ti.label == j0.label)
                        ti.label = f.instrs[i].label;

                j0.instr = I_REMOVE;
                t = true;
            }
        }
        clear_removed_instrs(f.instrs);
    }
    return t;
}

bool compiler_t::is_inlinable(
    std::string const& func, std::unordered_set<std::string>& refs)
{
    if(refs.count(func)) return false;
    auto it = funcs.find(func);
    if(it == funcs.end()) return true;
    auto const& f = it->second;
    refs.insert(func);
    for(auto const& i : f.instrs)
    {
        if(!i.is_label && !i.label.empty() && !is_inlinable(i.label, refs))
            return false;
    }
    return true;
}

bool compiler_t::is_inlinable(std::string const& func)
{
    std::unordered_set<std::string> refs;
    return is_inlinable(func, refs);
}

bool compiler_t::should_inline(std::string const& func, int ref_count)
{
    if(!is_inlinable(func)) return false;
    if(!funcs.count(func)) return false;
    if(ref_count == 1) return true;
#if 0
    return false;
#else
    auto const& instrs = funcs[func].instrs;
    if(instrs.size() <= 8) return true;
    if((size_t)(ref_count - 1) * instrs.size() <= inlining_max_add_instrs)
        return true;
    return false;
#endif
}

bool compiler_t::inline_function(std::string const& func)
{
    auto it = funcs.find(func);
    assert(it != funcs.end());
    if(it == funcs.end()) return false;
    auto const& f = it->second;
    bool t = false;
    for(auto& [tn, tf] : funcs)
    {
        if(tn == func) continue;
        for(size_t i = 0; i < tf.instrs.size(); ++i)
        {
            auto& ti = tf.instrs[i];
            if(ti.is_label) continue;
            if(ti.label != func) continue;
            switch(ti.instr)
            {
            case I_CALL:
            case I_CALL1:
                break;
            default:
                continue;
            }

            auto func_instrs = f.instrs;
            auto ret_label = new_label(tf);
            func_instrs.push_back({ I_NOP, 0, 0, 0, ret_label, true });
            for(auto& fi : func_instrs)
            {
                if(fi.is_label)
                {
                    // need to rename label in case of multiple inlining
                    auto replacement_label = new_label(tf);
                    for(auto& tfi : func_instrs)
                    {
                        if(!tfi.is_label && tfi.label == fi.label)
                            tfi.label = replacement_label;
                    }
                    fi.label = replacement_label;
                    continue;
                }
                if(fi.instr == I_RET)
                {
                    fi.instr = I_JMP;
                    fi.label = ret_label;
                }
            }

            tf.instrs.erase(tf.instrs.begin() + i);
            tf.instrs.insert(tf.instrs.begin() + i, func_instrs.begin(), func_instrs.end());
            t = true;
        }
    }
    if(t)
        funcs.erase(func);
    return t;
}

bool compiler_t::inline_or_remove_functions()
{
    std::unordered_map<std::string, int> func_refs;
    for(auto const& [n, f] : funcs)
    {
        func_refs[n] = 0;
        for(auto const& [tn, tf] : funcs)
        {
            for(auto const& i : tf.instrs)
                if(!i.is_label && i.label == n)
                    func_refs[n] += 1;
        }
    }

    bool t = false;
    for(auto const& [n, c] : func_refs)
    {
        if(n == "main") continue;
        if(n == "$globinit") continue;
        if(c == 0)
        {
            funcs.erase(n);
            t = true;
        }
        else if(should_inline(n, c))
        {
            inline_function(n);
            t = true;
        }
    }
    return t;
}


bool compiler_t::peephole_reduce_bake_pushl(compiler_func_t& f)
{
    bool t = false;
    if(!enable_bake_pushl)
        return false;
    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i];

        // bake progdata labels as PUSH's
        if(i0.instr == I_PUSHL)
        {
            if(auto it = progdata.find(i0.label); it != progdata.end())
            {
                auto offset = it->second.offset + i0.imm;
                i0.instr = I_PUSH;
                i0.imm = uint8_t(offset >> 0);
                std::string empty;
                i0.label.swap(empty);
                f.instrs.insert(f.instrs.begin() + i, 2, i0);
                f.instrs[i + 1].imm = uint8_t(offset >> 8);
                f.instrs[i + 2].imm = uint8_t(offset >> 16);
                t = true;
                continue;
            }
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_reduce(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {

        // replace:
        //     PUSH <M times>
        //     PUSH <N times>
        //     PUSH N
        //     SETLN M
        //     POPN <M-N times>
        // with:
        //     PUSH <N times>
        {
            size_t num_pushes = 0;
            for(size_t j = i; j + 2 < f.instrs.size(); ++j, ++num_pushes)
                if(f.instrs[j].instr != I_PUSH) break;
            if(num_pushes >= 3)
            {
                auto& isetln = f.instrs[i + num_pushes];
                size_t m = isetln.imm;
                auto& in = f.instrs[i + num_pushes - 1];
                size_t n = in.imm;
                auto& ipopn = f.instrs[i + num_pushes + 1];
                if(isetln.instr == I_SETLN && m >= n && num_pushes >= m + n + 1 &&
                    ipopn.instr == I_POPN && ipopn.imm == m - n)
                {
                    size_t istart = i + num_pushes - m - n - 1;
                    for(size_t j = 0; j < m; ++j)
                        f.instrs[istart + j].instr = I_REMOVE;
                    in.instr = I_REMOVE;
                    isetln.instr = I_REMOVE;
                    ipopn.instr = I_REMOVE;
                    t = true;
                    continue;
                }
            }
        }

        // replace the GETLN with PUSHs in:
        //     PUSH <N times>; PUSH <K>; GETLN M;  (M+K <= N)
        {
            size_t num_pushes = 0;
            for(size_t j = i; j + 1 < f.instrs.size(); ++j, ++num_pushes)
                if(f.instrs[j].instr != I_PUSH) break;
            if(num_pushes >= 2)
            {
                auto& ig = f.instrs[i + num_pushes];
                size_t m = ig.imm;
                auto& in = f.instrs[i + num_pushes - 1];
                size_t k = in.imm;
                if(ig.instr == I_GETLN && m + k <= num_pushes && k >= 1)
                {
                    ig.instr = I_REMOVE;
                    in.instr = I_REMOVE;
                    if(k > 2)
                        f.instrs.insert(f.instrs.begin() + i + num_pushes, k - 2, ig);
                    for(size_t j = 0; j < k; ++j)
                    {
                        auto& idst = f.instrs[i + num_pushes + j - 1];
                        auto& isrc = f.instrs[i + num_pushes + j - 1 - m];
                        idst.instr = I_PUSH;
                        idst.imm = isrc.imm;
                    }
                    t = true;
                    continue;
                }
            }
        }

        auto& i0 = f.instrs[i + 0];

        if((i0.instr == I_POPN || i0.instr == I_ALLOC) && i0.imm == 0)
        {
            i0.instr = I_REMOVE;
            t = true;
            continue;
        }

        auto& i1 = f.instrs[i + 1];

        // combine POPN N; ALLOC M; or ALLOC M; POPN N;
        if((i0.instr == I_POPN && i1.instr == I_ALLOC ||
            i0.instr == I_ALLOC && i1.instr == I_POPN) &&
            i0.imm >= 1 && i1.imm >= 1)
        {
            auto n = std::min(i0.imm, i1.imm);
            if((i0.imm -= n) == 0) i0.instr = I_REMOVE;
            if((i1.imm -= n) == 0) i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH N; DUP with PUSH N; PUSH N
        // (allows future optimizations)
        if(i0.instr == I_PUSH && i1.instr == I_DUP)
        {
            i1.instr = I_PUSH;
            i1.imm = i0.imm;
            t = true;
            continue;
        }

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

        // replace PUSHL <LABEL>; GETPN <N> with a bunch of pushes
        if(i0.instr == I_PUSHL && i1.instr == I_GETPN && i1.imm <= 16)
        {
            // locate label
            auto offset = i0.imm;
            auto n = i1.imm;
            auto it = progdata.find(i0.label);
            if(it == progdata.end())
                continue;
            offset += it->second.offset;
            uint8_t const* d = nullptr;
            for(auto const& [k, v] : progdata)
            {
                if(offset < v.offset)
                    continue;
                uint32_t off = offset - v.offset;
                if(size_t(off + n) > v.data.size())
                    continue;
                bool valid = true;
                for(auto const& p : v.relocs_glob)
                    if(p.first >= off && p.first < off + n)
                        valid = false;
                for(auto const& p : v.relocs_prog)
                    if(p.first >= off && p.first < off + n)
                        valid = false;
                if(!valid)
                    break;
                d = v.data.data() + off;
                break;
            }
            if(!d) continue;

            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;

            auto line = i0.line;
            auto file = i0.file;
            f.instrs.insert(f.instrs.begin() + i, n, i0);

            // i0, i1 invalidated now

            for(size_t j = 0; j < n; ++j)
            {
                auto& ti = f.instrs[i + j];
                ti.instr = I_PUSH;
                ti.line = line;
                ti.imm = d[j];
                ti.file = file;
            }
            t = true;
            continue;
        }

        // replace GETPN <N>; POPN M with GETPN <N-1>
        if(i0.instr == I_GETPN && i0.imm > 1 && i1.instr == I_POPN && i1.imm >= 1)
        {
            auto n = std::min(i0.imm, i1.imm);
            i0.imm -= n;
            i1.imm -= n;
            if(i0.imm == 0)
            {
                // pop prog addr
                i0.instr = I_POPN;
                i0.imm = 3;
            }
            if(i1.imm == 0)
                i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // remove PUSH N; POP
        if(i0.instr == I_PUSH && i1.instr == I_POPN && i1.imm >= 1)
        {
            i0.instr = I_REMOVE;
            if(--i1.imm == 0)
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

        // replace PUSH N; BOOL with PUSH N (N == 0 or 1)
        if(i0.instr == I_PUSH && i0.imm <= 1 && i1.instr == I_BOOL)
        {
            i0.imm = (i0.imm == 0 ? 0 : 1);
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH N; NOT with PUSH !N
        if(i0.instr == I_PUSH && i1.instr == I_NOT)
        {
            i0.imm = (i0.imm == 0 ? 1 : 0);
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

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

        // try and bake GETLN
        if(i0.instr == I_PUSH && i1.instr == I_GETLN && i >= i1.imm)
        {
            bool valid = true;
            for(size_t j = i - i1.imm; j < i; ++j)
            {
                if(f.instrs[j].instr != I_PUSH)
                {
                    valid = false;
                    break;
                }
            }
            if(valid)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_REMOVE;
                auto n = i0.imm;
                auto off = i1.imm;
                f.instrs.insert(f.instrs.begin() + i, n, i1);
                for(size_t j = 0; j < n; ++j)
                {
                    f.instrs[i + j].instr = I_PUSH;
                    f.instrs[i + j].imm = f.instrs[i + j - off].imm;
                }
                t = true;
                continue;
            }
        }

        // remove REFG N; POPN 2
        if(i0.instr == I_PUSHG &&
            i1.instr == I_POPN && i1.imm == 2)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }

        if(i + 2 >= f.instrs.size()) continue;
        auto& i2 = f.instrs[i + 2];

        // replace PUSH <N>; GETLN <M>; POP with PUSH <N-1>; GETLN <M>
        // replace PUSH <N>; GETGN <M>; POP with PUSH <N-1>; GETGN <M>
        if(i0.instr == I_PUSH && i2.instr == I_POPN && i2.instr >= 1 &&
            (i1.instr == I_GETLN || i1.instr == I_GETGN))
        {
            assert(i0.imm > 0);
            if(--i2.imm == 0)
                i2.instr = I_REMOVE;
            if(i0.imm == 1)
            {
                i0.instr = I_REMOVE;
                i1.instr = I_REMOVE;
            }
            else
            {
                i0.imm -= 1;
            }
            t = true;
            continue;
        }

        // replace PUSH <M>; GETLN <N>; POP with PUSH <M-1>; GETLN <N>
        if(i0.instr == I_PUSH && i1.instr == I_GETLN && i1.imm > 1 &&
            i2.instr == I_POPN && i2.imm >= 1)
        {
            i0.imm -= 1;
            if(--i2.imm == 0)
                i2.instr = I_REMOVE;
            t = true;
            continue;
        }

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

        // replace PUSH M; PUSH N; OP with PUSH OP(M,N)
        if(i0.instr == I_PUSH && i1.instr == I_PUSH)
        {
            bool tt = true;
            switch(i2.instr)
            {
            case I_ADD: i0.imm = uint8_t(i0.imm + i1.imm); break;
            case I_SUB: i0.imm = uint8_t(i0.imm - i1.imm); break;
            case I_MUL: i0.imm = uint8_t(i0.imm * i1.imm); break;
            case I_AND: i0.imm = uint8_t(i0.imm & i1.imm); break;
            case I_OR: i0.imm = uint8_t(i0.imm | i1.imm); break;
            case I_XOR: i0.imm = uint8_t(i0.imm ^ i1.imm); break;
            case I_LSL: i0.imm = uint8_t(i0.imm << i1.imm); break;
            case I_LSR: i0.imm = uint8_t(i0.imm >> i1.imm); break;
            case I_CULT: i0.imm = i0.imm < i1.imm ? 1 : 0; break;
            case I_CSLT: i0.imm = (int8_t)i0.imm < (int8_t)i1.imm ? 1 : 0; break;
            case I_BOOL2: i0.imm = i0.imm != 0 || i1.imm != 0 ? 1 : 0; break;
            case I_COMP2:
            {
                uint16_t imm16 = uint16_t(-int16_t(i0.imm + i1.imm * 256));
                i0.imm = uint8_t(imm16 >> 0);
                i1.imm = uint8_t(imm16 >> 8);
                i2.instr = I_REMOVE;
                t = true;
                continue;
            }
            default: tt = false; break;
            }
            if(tt)
            {
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                t = true;
                continue;
            }
        }

        if(i + 3 >= f.instrs.size()) continue;
        auto& i3 = f.instrs[i + 3];

        // replace PUSH; PUSH; PUSH; GETPN <N> with a bunch of pushes
        if(i0.instr == I_PUSH && i1.instr == I_PUSH && i2.instr == I_PUSH &&
            i3.instr == I_GETPN && i3.imm <= max_getpn_bake)
        {
            size_t n = i3.imm;
            uint32_t offset = 0;
            offset += (i0.imm << 0);
            offset += (i1.imm << 8);
            offset += (i2.imm << 16);
            uint8_t const* d = nullptr;
            for(auto const& [k, v] : progdata)
            {
                if(offset < v.offset)
                    continue;
                uint32_t off = offset - v.offset;
                if(size_t(off + n) > v.data.size())
                    continue;
                bool valid = true;
                for(auto const& p : v.relocs_glob)
                    if(p.first >= off && p.first < off + n)
                        valid = false;
                for(auto const& p : v.relocs_prog)
                    if(p.first >= off && p.first < off + n)
                        valid = false;
                if(!valid)
                    break;
                d = v.data.data() + off;
                break;
            }
            if(!d) continue;

            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;

            auto line = i0.line;
            auto file = i0.file;
            if(n > 4)
                f.instrs.insert(f.instrs.begin() + i, n - 4, i0);

            // i0, i1 invalidated now

            for(size_t j = 0; j < n; ++j)
            {
                auto& ti = f.instrs[i + j];
                ti.instr = I_PUSH;
                ti.line = line;
                ti.imm = d[j];
                ti.file = file;
            }
            t = true;
            continue;
        }

        // replace:
        //     PUSH N
        //     GETGN/GETLN M
        //     PUSH N
        //     SETLN N
        // with:
        //     POPN N
        //     PUSH N
        //     GETGN/GETLN M
        if(i0.instr == I_PUSH &&
            (i1.instr == I_GETGN || i1.instr == I_GETLN) &&
            i2.instr == I_PUSH && i3.instr == I_SETLN &&
            i0.imm == i2.imm && i0.imm == i3.imm)
        {
            i2 = i1;
            i1.instr = I_PUSH;
            i1.imm = i0.imm;
            i0.instr = I_POPN;
            i3.instr = I_REMOVE;
            t = true;
            continue;
        }

        // combine consecutive GETLNs to adjacant stack locations
        if(i0.instr == I_PUSH && i1.instr == I_GETLN &&
            i2.instr == I_PUSH && i3.instr == I_GETLN &&
            i1.imm == i3.imm &&
            i0.imm + i2.imm <= i1.imm)
        {
            i0.imm += i2.imm;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            t = true;
            continue;
        }


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

        // bake PUSH A; PUSH B; PUSH C; BOOL3
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_BOOL3)
        {
            i0.imm = uint8_t(i0.imm || i1.imm || i2.imm);
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            t = true;
            continue;
        }

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

        // replace REFG N; PUSH A0; PUSH A1; ADD2 with REFG N+A
        if(i0.instr == I_PUSHG &&
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

        if(i + 4 >= f.instrs.size()) continue;
        auto& i4 = f.instrs[i + 4];

        // replace:
        //    PUSH <N>
        //    GETLN <N>
        //    PUSH <N>
        //    SETLN <M>
        //    POPN N
        // with:
        //    PUSH <N>
        //    SETLN <M-N>
        if(i0.instr == I_PUSH && i1.instr == I_GETLN &&
            i2.instr == I_PUSH && i3.instr == I_SETLN &&
            i4.instr == I_POPN &&
            i0.imm == i1.imm && i0.imm == i2.imm && i0.imm == i4.imm &&
            i3.imm > i0.imm)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i3.imm -= i0.imm;
            i4.instr = I_REMOVE;
            t = true;
            continue;
        }

        // convert PUSHL <LABEL>; PUSH ..; PUSH ..; PUSH ...; ADD3 to PUSHL LABEL+..
        if(i0.instr == I_PUSHL &&
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

        // bake PUSH A; PUSH B; PUSH C; PUSH D; BOOL4
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_PUSH &&
            i4.instr == I_BOOL4)
        {
            i0.imm = uint8_t(i0.imm || i1.imm || i2.imm || i3.imm);
            i1.instr = I_REMOVE;
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            i4.instr = I_REMOVE;
            t = true;
            continue;
        }

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

        // bake PUSH A; PUSH B; PUSH C; PUSH D; ADD2/SUB2/MUL2
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_PUSH &&
            (i4.instr == I_ADD2 || i4.instr == I_SUB2 || i4.instr == I_MUL2))
        {
            uint16_t dst = uint16_t(i0.imm + i1.imm * 256);
            uint16_t src = uint16_t(i2.imm + i3.imm * 256);
            if(i4.instr == I_ADD2) dst += src;
            if(i4.instr == I_SUB2) dst -= src;
            if(i4.instr == I_MUL2) dst *= src;
            i0.imm = uint8_t(dst >> 0);
            i1.imm = uint8_t(dst >> 8);
            i2.instr = I_REMOVE;
            i3.instr = I_REMOVE;
            i4.instr = I_REMOVE;
            t = true;
            continue;
        }

        // replace PUSH x4; OP2 with PUSH OP2(M,N)
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_PUSH)
        {
            bool tt = true;
            uint32_t imm0 = i0.imm + (i1.imm << 8);
            uint32_t imm1 = i2.imm + (i3.imm << 8);
            switch(i4.instr)
            {
            case I_ADD2: imm0 += imm1; break;
            case I_SUB2: imm0 -= imm1; break;
            case I_MUL2: imm0 *= imm1; break;
            case I_AND2: imm0 &= imm1; break;
            case I_OR2:  imm0 |= imm1; break;
            case I_XOR2: imm0 ^= imm1; break;
            case I_CULT2:
                imm0 = imm0 < imm1 ? 1 : 0;
                i1.instr = I_REMOVE;
                break;
            case I_CSLT2:
                imm0 = (int16_t)imm0 < (int16_t)imm1 ? 1 : 0;
                i1.instr = I_REMOVE;
                break;
            case I_BOOL4:
                imm0 = imm0 != 0 || imm1 != 0 ? 1 : 0;
                i1.instr = I_REMOVE;
                break;
            case I_COMP4:
            {
                uint32_t imm32 = uint32_t(-int32_t(imm0 + (imm1 << 16)));
                i0.imm = uint8_t(imm32 >> 0);
                i1.imm = uint8_t(imm32 >> 8);
                i2.imm = uint8_t(imm32 >> 16);
                i3.imm = uint8_t(imm32 >> 24);
                i4.instr = I_REMOVE;
                t = true;
                continue;
            }
            default: tt = false; break;
            }
            if(tt)
            {
                i0.imm = uint8_t(imm0 >> 0);
                i1.imm = uint8_t(imm0 >> 8);
                i2.instr = I_REMOVE;
                i3.instr = I_REMOVE;
                i4.instr = I_REMOVE;
                t = true;
                continue;
            }
        }

        if(i + 5 >= f.instrs.size()) continue;
        auto& i5 = f.instrs[i + 5];

        if(i + 6 >= f.instrs.size()) continue;
        auto& i6 = f.instrs[i + 6];

        // bake PUSH A; ... PUSH F; ADD3/SUB3
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_PUSH &&
            i4.instr == I_PUSH && i5.instr == I_PUSH &&
            (i6.instr == I_ADD3 || i6.instr == I_SUB3))
        {
            uint32_t dst = uint32_t(i0.imm + (i1.imm << 8) + (i2.imm << 16));
            uint32_t src = uint32_t(i3.imm + (i4.imm << 8) + (i5.imm << 16));
            if(i6.instr == I_ADD3) dst += src;
            if(i6.instr == I_SUB3) dst -= src;
            i0.imm = uint8_t(dst >> 0);
            i1.imm = uint8_t(dst >> 8);
            i2.imm = uint8_t(dst >> 16);
            i3.instr = I_REMOVE;
            i4.instr = I_REMOVE;
            i5.instr = I_REMOVE;
            i6.instr = I_REMOVE;
            t = true;
            continue;
        }
        
        // replace PUSH x6; OP3 with PUSH OP3(M,N)
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_PUSH &&
            i4.instr == I_PUSH && i5.instr == I_PUSH)
        {
            bool tt = true;
            uint32_t imm0 = i0.imm + (i1.imm << 8) + (i2.imm << 16);
            uint32_t imm1 = i3.imm + (i4.imm << 8) + (i5.imm << 16);
            switch(i6.instr)
            {
            case I_ADD3: imm0 += imm1; break;
            case I_SUB3: imm0 -= imm1; break;
            case I_MUL3: imm0 *= imm1; break;
            case I_CULT3:
                imm0 = imm0 < imm1 ? 1 : 0;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                break;
            case I_CSLT3:
                if(imm0 & 0x800000) imm0 |= 0xff000000;
                if(imm1 & 0x800000) imm1 |= 0xff000000;
                imm0 = (int32_t)imm0 < (int32_t)imm1 ? 1 : 0;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                break;
            default: tt = false; break;
            }
            if(tt)
            {
                i0.imm = uint8_t(imm0 >> 0);
                i1.imm = uint8_t(imm0 >> 8);
                i2.imm = uint8_t(imm0 >> 16);
                i3.instr = I_REMOVE;
                i4.instr = I_REMOVE;
                i5.instr = I_REMOVE;
                i6.instr = I_REMOVE;
                t = true;
                continue;
            }
        }

        if(i + 7 >= f.instrs.size()) continue;
        auto& i7 = f.instrs[i + 7];

        if(i + 8 >= f.instrs.size()) continue;
        auto& i8 = f.instrs[i + 8];

        // replace PUSH x8; OP4 with PUSH OP4(M,N)
        if(i0.instr == I_PUSH && i1.instr == I_PUSH &&
            i2.instr == I_PUSH && i3.instr == I_PUSH &&
            i4.instr == I_PUSH && i5.instr == I_PUSH &&
            i6.instr == I_PUSH && i7.instr == I_PUSH)
        {
            bool tt = true;
            uint32_t imm0 = i0.imm + (i1.imm << 8) + (i2.imm << 16) + (i3.imm << 24);
            uint32_t imm1 = i4.imm + (i5.imm << 8) + (i6.imm << 16) + (i7.imm << 24);
            switch(i8.instr)
            {
            case I_ADD4: imm0 += imm1; break;
            case I_SUB4: imm0 -= imm1; break;
            case I_MUL4: imm0 *= imm1; break;
            case I_CULT4:
                imm0 = imm0 < imm1 ? 1 : 0;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                i3.instr = I_REMOVE;
                break;
            case I_CSLT4:
                imm0 = (int32_t)imm0 < (int32_t)imm1 ? 1 : 0;
                i1.instr = I_REMOVE;
                i2.instr = I_REMOVE;
                i3.instr = I_REMOVE;
                break;
            default: tt = false; break;
            }
            if(tt)
            {
                i0.imm = uint8_t(imm0 >> 0);
                i1.imm = uint8_t(imm0 >> 8);
                i2.imm = uint8_t(imm0 >> 16);
                i3.imm = uint8_t(imm0 >> 24);
                i4.instr = I_REMOVE;
                i5.instr = I_REMOVE;
                i6.instr = I_REMOVE;
                i7.instr = I_REMOVE;
                i8.instr = I_REMOVE;
                t = true;
                continue;
            }
        }

    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_linc(compiler_func_t& f)
{
    bool t = false;

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

        if(i0.instr == I_GETL && i1.instr == I_DEC && i2.instr == I_SETL &&
            i0.imm == i2.imm && i0.imm == 1)
        {
            i0.instr = I_REMOVE;
            i1.instr = I_REMOVE;
            i2.instr = I_DEC;
            t = true;
            continue;
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_dup_sext(compiler_func_t& f)
{
    bool t = false;

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
    clear_removed_instrs(f.instrs);

    return t;
}

bool compiler_t::peephole_pre_push_compress(compiler_func_t& f)
{
    bool t = false;

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
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_bzp(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i + 2 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];

        // replace DUP; B[N]Z; POPN 1 with B[N]ZP
        if(i0.instr == I_DUP && i2.instr == I_POPN && i2.imm >= 1 && (
            i1.instr == I_BZ || i1.instr == I_BNZ))
        {
            i0.instr = I_REMOVE;
            i1.instr = (i1.instr == I_BZ ? I_BZP : I_BNZP);
            if(--i2.imm == 0)
                i2.instr = I_REMOVE;
            t = true;
            continue;
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_redundant_bzp(compiler_func_t& f)
{
    bool t = false;

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
                    I_NOP, f.instrs[j].line, 0, 0, label, true, f.instrs[j].file });
                i0.instr = (i0.instr == I_BZP ? I_BZ : I_BNZ);
                i0.label = label;
                t = true;
            }
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_push_setl_popn(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i + 2 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];
        auto& i2 = f.instrs[i + 2];

        // replace PUSH M; SETL N+1; POPN N; with POPN N+1; PUSH M;
        if(i0.instr == I_PUSH && i1.instr == I_SETL &&
            (i2.instr == I_POPN && i2.imm + 1 == i1.imm))
        {
            i0.instr = I_POPN;
            i1.instr = I_PUSH;
            i2.instr = I_REMOVE;
            std::swap(i0.imm, i1.imm);
            t = true;
            continue;
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_combine_pop(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i + 1 < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        auto& i1 = f.instrs[i + 1];

        // replace POPN M; POPN N with POPN M+N
        if(i0.instr == I_POPN && i1.instr == I_POPN)
        {
            i0.imm = uint8_t(i0.imm + i1.imm);
            i1.instr = I_REMOVE;
            t = true;
            continue;
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_compress_pop(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];
        if(i0.instr != I_POPN) continue;
        switch(i0.imm)
        {
        case 1: i0.instr = I_POP;  t = true; break;
        case 2: i0.instr = I_POP2; t = true; break;
        case 3: i0.instr = I_POP3; t = true; break;
        case 4: i0.instr = I_POP4; t = true; break;
        default: break;
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_compress_push_sequence(compiler_func_t& f)
{
    bool t = false;

    std::vector< compiler_instr_t> pi;

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        size_t n = 0;
        for(size_t j = i; j < f.instrs.size() && f.instrs[j].instr == I_PUSH; ++j, ++n)
            f.instrs[j].instr = I_REMOVE;
        if(n == 0)
            continue;
        pi.clear();

        push_compression(pi, f.instrs.data() + i, f.instrs.data() + i, n);

        if(pi.size() > n)
        {
            f.instrs.insert(
                f.instrs.begin() + i + n,
                size_t(pi.size() - n),
                {});
        }

        for(size_t j = 0; j < pi.size(); ++j)
            f.instrs[i + j] = pi[j];
        i += std::max(n, pi.size());
    }
    clear_removed_instrs(f.instrs);
    return t;
}

static compiler_instr_t instr(compiler_instr_t i, instr_t ii, uint32_t imm = 0)
{
    i.instr = ii;
    i.imm = imm;
    return i;
}

void compiler_t::push_compression(
    std::vector<compiler_instr_t>& pi,
    compiler_instr_t const* b,
    compiler_instr_t const* d, size_t n)
{
    compiler_instr_t const* dend = d + n;
    while(d < dend)
    {
        n = (dend - d);

        // sequence of all zeros
        size_t nz = 0;
        for(; nz < n && d[nz].imm == 0; ++nz)
            ;

        // PZ16
        if(nz >= 16)
        {
            pi.push_back(instr(*d, I_PZ16));
            d += 16;
            continue;
        }
        // PZ8
        if(nz >= 8)
        {
            pi.push_back(instr(*d, I_PZ8));
            d += 8;
            continue;
        }
        // P0000
        if(nz >= 4)
        {
            pi.push_back(instr(*d, I_P0000));
            d += 4;
            continue;
        }
        // P000
        if(nz >= 3)
        {
            pi.push_back(instr(*d, I_P000));
            d += 3;
            continue;
        }
        // P00
        if(nz >= 2)
        {
            pi.push_back(instr(*d, I_P00));
            d += 2;
            continue;
        }

        // GETL4
        if(n >= 4)
        {
            uint8_t i0 = (uint8_t)d[0].imm;
            uint8_t i1 = (uint8_t)d[1].imm;
            uint8_t i2 = (uint8_t)d[2].imm;
            uint8_t i3 = (uint8_t)d[3].imm;
            compiler_instr_t const* p = d;
            bool found = false;
            while(b + 4 <= p && p + 252 > d)
            {
                if( p[-4].imm == i0 && p[-3].imm == i1 &&
                    p[-2].imm == i2 && p[-1].imm == i3)
                {
                    found = true;
                    break;
                }
                --p;
            }
            if(found)
            {
                pi.push_back(instr(*d, I_GETL4, (uint32_t)(uintptr_t)(d - p) + 4));
                d += 4;
                continue;
            }
        }

        // DUPWn
        if(n >= 2)
        {
            uint8_t i0 = (uint8_t)d[0].imm;
            uint8_t i1 = (uint8_t)d[1].imm;
            compiler_instr_t const* p = d;
            while(b + 2 <= p && p + 8 > d)
            {
                if(p[-2].imm == i0 && p[-1].imm == i1)
                    break;
                --p;
            }
            if(b + 2 <= p && p + 8 > d)
            {
                pi.push_back(instr(*d, instr_t(I_DUPW + uintptr_t(d - p))));
                d += 2;
                continue;
            }
        }

        // DUP
        if(b < d && d[-1].imm == d[0].imm)
        {
            pi.push_back(instr(*d++, I_DUP));
            continue;
        }

        // normal PUSH: try to combine with previous PUSH/PUSH2/PUSH3
        if(!pi.empty())
        {
            auto& prev = pi.back();
            switch(prev.instr)
            {
            case I_PUSH:
                prev.instr = I_PUSH2;
                prev.imm += (((d++)->imm & 0xff) << 8);
                continue;
            case I_PUSH2:
                prev.instr = I_PUSH3;
                prev.imm += (((d++)->imm & 0xff) << 16);
                continue;
            case I_PUSH3:
                prev.instr = I_PUSH4;
                prev.imm += (((d++)->imm & 0xff) << 24);
                continue;
            default:
                break;
            }
        }

        switch(d->imm)
        {
        case 0:   pi.push_back(instr(*d, I_P0  )); ++d; continue;
        case 1:   pi.push_back(instr(*d, I_P1  )); ++d; continue;
        case 2:   pi.push_back(instr(*d, I_P2  )); ++d; continue;
        case 3:   pi.push_back(instr(*d, I_P3  )); ++d; continue;
        case 4:   pi.push_back(instr(*d, I_P4  )); ++d; continue;
        case 5:   pi.push_back(instr(*d, I_P5  )); ++d; continue;
        case 6:   pi.push_back(instr(*d, I_P6  )); ++d; continue;
        case 7:   pi.push_back(instr(*d, I_P7  )); ++d; continue;
        case 8:   pi.push_back(instr(*d, I_P8  )); ++d; continue;
        case 16:  pi.push_back(instr(*d, I_P16 )); ++d; continue;
        case 32:  pi.push_back(instr(*d, I_P32 )); ++d; continue;
        case 64:  pi.push_back(instr(*d, I_P64 )); ++d; continue;
        case 128: pi.push_back(instr(*d, I_P128)); ++d; continue;
        default:
            break;
        }

        // DUPn
        {
            uint8_t i0 = (uint8_t)d[0].imm;
            compiler_instr_t const* p = d;
            while(b + 1 <= p && p + 8 > d)
            {
                if(p[-1].imm == i0)
                    break;
                --p;
            }
            if(b + 1 <= p && p + 8 > d)
            {
                pi.push_back(instr(*d, instr_t(I_DUP + uintptr_t(d - p))));
                d += 1;
                continue;
            }
        }

        if(!pi.empty() && n <= 3)
        {
            auto& prev = pi.back();
            switch(prev.instr)
            {
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
            {
                static uint8_t const C[] =
                {
                    0, 1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128
                };
                prev.imm = C[prev.instr - I_P0] + (((d++)->imm & 0xff) << 8);
                prev.instr = I_PUSH2;
                continue;
            }
            default:
                break;
            }
        }

        // normal PUSH
        pi.push_back(instr(*d, I_PUSH, d->imm));
        ++d;

    }
}

bool compiler_t::peephole_remove_inaccessible_code(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];

        // remove all code between jmp and label
        if(i0.instr == I_JMP)
        {
            for(size_t j = i + 1; j < f.instrs.size(); ++j)
            {
                auto& ti = f.instrs[j];
                if(ti.is_label) break;
                ti.instr = I_REMOVE;
            }
        }
    }
    clear_removed_instrs(f.instrs);
    return t;
}

bool compiler_t::peephole_jmp_to_ret(compiler_func_t& f)
{
    bool t = false;

    for(size_t i = 0; i < f.instrs.size(); ++i)
    {
        auto& i0 = f.instrs[i + 0];

        if(i0.instr != I_JMP) continue;

        size_t labeli;
        for(labeli = 0; labeli < f.instrs.size(); ++labeli)
            if(f.instrs[labeli].is_label && f.instrs[labeli].label == i0.label)
                break;
        if(labeli >= f.instrs.size()) continue;

        size_t n = 0;
        for(size_t j = labeli + 1; j < f.instrs.size(); ++j)
        {
            auto& j0 = f.instrs[j];
            if(n >= 16 || j == i || j0.is_label ||
                0)
                //j0.instr == I_BZ || j0.instr == I_BNZ ||
                //j0.instr == I_BZP || j0.instr == I_BNZP)
            {
                n = 0;
                break;
            };

            n += 1;

            if(j0.instr == I_JMP || j0.instr == I_RET)
                break;
        }
        if(n == 0) continue;

        std::vector<compiler_instr_t> slice(
            f.instrs.begin() + labeli + 1,
            f.instrs.begin() + labeli + 1 + n);
        i0.instr = I_REMOVE;
        f.instrs.insert(f.instrs.begin() + i, slice.begin(), slice.end());

        t = true;
    }
    clear_removed_instrs(f.instrs);
    return t;
}

}
