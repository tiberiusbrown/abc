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

static bool is_branch_jmp_call(compiler_instr_t const& i)
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
    case I_GTGB:
    case I_GTGB2:
    case I_GTGB4:
    case I_GETG:
    case I_GETG2:
    case I_GETG4:
    case I_GETGN:
    case I_GETL:
    case I_GETL2:
    case I_GETL4:
    case I_GETLN:
        return true;
    default:
        return false;
    }
}

bool compiler_t::optimize_stack_func(std::vector<compiler_instr_t>& instrs)
{
    bool t = false;
    std::unordered_set<std::string> found_labels;
    for(size_t i = 0; i < instrs.size(); ++i)
    {
        auto& i0 = instrs[i];

        // remove outer pair of GETLN N N ... SETLN N N
        // when inside doesn't modify rest of stack
        // e.g.:
        //     dup
        //     p4
        //     add
        //     setl  1
        // becomes:
        //     p4
        //     add
        if(i >= 1 && i0.instr == I_SETLN && i0.imm == i0.imm2)
        {
            bool found = false;
            size_t j = i - 1;
            for(int n = 0; n < 32; ++n)
            {
                auto const& ij = instrs[j];
                if(ij.instr == I_GETLN && ij.imm == ij.imm2)
                {
                    found = true;
                    break;
                }
                if(j-- == 0)
                    break;
            }
            if(found)
            {
                // check stack access in intermediate instructions
                int n = (int)i0.imm;
                bool valid = true;
                for(size_t k = j + 1; k < i; ++k)
                {
                    auto const& ik = instrs[k];
                    if(ik.instr == I_RET || ik.is_label || is_branch_jmp_call(ik) ||
                        (int)instr_accesses_stack(ik, 1).second > n)
                    {
                        valid = false;
                        break;
                    }
                    n += instr_stack_mod(ik);
                }
                if(valid && n == (int)i0.imm)
                {
                    i0.instr = I_REMOVE;
                    instrs[j].instr = I_REMOVE;
                    t = true;
                    continue;
                }
            }
        }

        // eliminate PUSH ... POP that's unused in between
        if(!is_stack_eliminatable(i0))
            continue;

        bool elim = false;
        int n = instr_stack_mod(i0);
        int size = n;
        size_t j;
        found_labels.clear();
        for(j = i + 1; j < instrs.size(); ++j)
        {
            auto const& ij = instrs[j];
            if(is_pop(ij) || (
                ij.instr == I_SETLN && int(j - i - 1) >= (int)ij.imm))
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
            {
                found_labels.insert(ij.label);
                continue;
            }
            if(ij.instr == I_RET)
                break;
            if(is_branch_jmp_call(ij) && !found_labels.count(ij.label))
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

        if(!elim)
            continue;

        auto& iend = instrs[j];

        if(iend.instr == I_SETLN)
        {
            // check preceeding pushes
            {
                bool valid = true;
                for(int k = 1; k <= (int)iend.imm; ++k)
                {
                    if(instrs[j - k].instr != I_PUSH)
                        valid = false;
                }
                if(!valid)
                    continue;
            }

            size_t setln_off = iend.imm + iend.imm2 - n;

            // NOTE TO SELF:
            // THESE MOVES NEED TO BE ROTATES

            // move pushes in place of original instruction
            {
                assert(int(iend.imm + iend.imm2) >= n);
                assert((int)iend.imm >= size);
                size_t isrc = j - n + iend.imm2;
                size_t idst = i + 1;
                std::rotate(
                    instrs.begin() + idst,
                    instrs.begin() + isrc,
                    instrs.begin() + isrc + size
                );
                //std::move(
                //    instrs.begin() + isrc,
                //    instrs.begin() + isrc + size,
                //    instrs.begin() + idst);
            }

            // move orig instr to behind setln
            std::rotate(
                instrs.begin() + i,
                instrs.begin() + i + 1,
                instrs.begin() + j
            );
            //std::move_backward(
            //    instrs.begin() + i,
            //    instrs.begin() + i + 1,
            //    instrs.begin() + j);

            // split setln
            auto& iend0 = instrs[j - 1];
            auto& iend1 = instrs[j + 0];
            iend0 = iend1;
            iend0.imm = uint32_t(setln_off);
            iend0.imm2 += (iend1.imm - iend0.imm);
            iend1.imm -= uint32_t(setln_off + size);
            iend1.imm2 -= uint32_t(setln_off + size);
            if(iend0.imm == 0) iend0.instr = I_REMOVE;
            if(iend1.imm == 0) iend1.instr = I_REMOVE;
            t = true;
            continue;
            //__debugbreak();
        }

        static_assert(I_POP2 == I_POP + 1, "");
        static_assert(I_POP3 == I_POP2 + 1, "");
        static_assert(I_POP4 == I_POP3 + 1, "");
        i0.instr = I_REMOVE;
        switch(iend.instr)
        {
        case I_POP:
            assert(size <= 1);
            iend.instr = I_REMOVE;
            break;
        case I_POP2:
            assert(size <= 2);
            iend.instr = (size >= 2 ? I_REMOVE : instr_t(I_POP2 - size));
            break;
        case I_POP3:
            assert(size <= 3);
            iend.instr = (size >= 3 ? I_REMOVE : instr_t(I_POP3 - size));
            break;
        case I_POP4:
            assert(size <= 4);
            iend.instr = (size >= 4 ? I_REMOVE : instr_t(I_POP4 - size));
            break;
        case I_POPN:
            assert(size <= (int)iend.imm);
            if((iend.imm -= size) <= 0)
                iend.instr = I_REMOVE;
            break;
        default:
            assert(0);
            break;
        }
        t = true;
    }
    clear_removed_instrs(instrs);
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
