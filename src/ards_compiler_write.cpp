#include "ards_compiler.hpp"

#include <algorithm>
#include <filesystem>

#include <assert.h>
#include <stdlib.h>

namespace ards
{

void compiler_t::write_instr(
    std::ostream& f, compiler_instr_t const& instr, uint16_t& line,
    uint16_t& file, std::vector<std::string> const& filenames)
{
    if(instr.instr == I_REMOVE)
        return;
    if(file != instr.file && instr.file != 0)
    {
        file = instr.file;
        f << "  .file " << filenames[instr.file - 1] << ".abc\n";
    }
    if(instr.is_label)
    {
        f << instr.label << ":\n";
        return;
    }
    assert(instr.line != 0);
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
    case I_P16:   f << "p16"; break;
    case I_P32:   f << "p32"; break;
    case I_P64:   f << "p64"; break;
    case I_P128:  f << "p128"; break;
    case I_P00:   f << "p00"; break;
    case I_P000:  f << "p000"; break;
    case I_P0000: f << "p0000"; break;
    case I_PZ8:   f << "pz8"; break;
    case I_PZ16:  f << "pz16"; break;
    case I_PUSHG: f << "pushg " << instr.label << " " << instr.imm; break;
    case I_PUSH2: f << "push2 " << instr.imm; break;
    case I_PUSHL: f << "pushl " << instr.label << " " << instr.imm; break;
    case I_PUSH3: f << "push3 " << instr.imm; break;
    case I_PUSH4: f << "push4 " << instr.imm; break;
    case I_SEXT:  f << "sext"; break;
    case I_SEXT2: f << "sext2"; break;
    case I_SEXT3: f << "sext3"; break;
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
    case I_GETL4: f << "getl4 " << instr.imm; break;
    case I_GETLN: f << "getln " << instr.imm; break;
    case I_SETL:  f << "setl  " << instr.imm; break;
    case I_SETL2: f << "setl2 " << instr.imm; break;
    case I_SETL4: f << "setl4 " << instr.imm; break;
    case I_SETLN: f << "setln " << instr.imm; break;
    case I_GETG:  f << "getg  " << instr.label << " " << instr.imm; break;
    case I_GETG2: f << "getg2 " << instr.label << " " << instr.imm; break;
    case I_GETG4: f << "getg4 " << instr.label << " " << instr.imm; break;
    case I_GETGN: f << "getgn " << instr.label << " " << instr.imm; break;
    case I_GTGB:  assert(false); break;
    case I_SETG:  f << "setg  " << instr.label << " " << instr.imm; break;
    case I_SETG2: f << "setg2 " << instr.label << " " << instr.imm; break;
    case I_SETG4: f << "setg4 " << instr.label << " " << instr.imm; break;
    case I_SETGN: f << "setgn " << instr.imm << " " << instr.label << " " << instr.imm2; break;
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
    case I_POPN:  f << "popn  " << instr.imm; break;
    case I_ALLOC: f << "alloc " << instr.imm; break;

    case I_AIXB1: f << "aixb1 " << instr.imm; break;
    case I_AIDXB: f << "aidxb " << instr.imm << " " << instr.imm2; break;
    case I_AIDX:  f << "aidx  " << instr.imm << " " << instr.imm2; break;

    case I_PIDXB: f << "pidxb " << instr.imm << " " << instr.imm2; break;
    case I_PIDX:  f << "pidx  " << instr.imm << " " << instr.imm2; break;

    case I_UAIDX: f << "uaidx " << instr.imm; break;
    case I_UPIDX: f << "upidx " << instr.imm; break;

    case I_ASLC:  f << "aslc  " << instr.imm; break;
    case I_PSLC:  f << "pslc  " << instr.imm; break;

    case I_REFL:  f << "refl  " << instr.imm; break;
    case I_REFGB: assert(false); break;

    case I_INC:   f << "inc"; break;
    case I_DEC:   f << "dec"; break;
    case I_LINC:  f << "linc  " << instr.imm; break;

    case I_PINC:  f << "pinc"; break;
    case I_PINC2: f << "pinc2"; break;
    case I_PINC3: f << "pinc3"; break;
    case I_PINC4: f << "pinc4"; break;
    case I_PDEC:  f << "pdec"; break;
    case I_PDEC2: f << "pdec2"; break;
    case I_PDEC3: f << "pdec3"; break;
    case I_PDEC4: f << "pdec4"; break;
    case I_PINCF: f << "pincf"; break;
    case I_PDECF: f << "pdecf"; break;

    case I_ADD:   f << "add"; break;
    case I_ADD2:  f << "add2"; break;
    case I_ADD3:  f << "add3"; break;
    case I_ADD4:  f << "add4"; break;
    case I_SUB:   f << "sub"; break;
    case I_SUB2:  f << "sub2"; break;
    case I_SUB3:  f << "sub3"; break;
    case I_SUB4:  f << "sub4"; break;
    case I_ADD2B: f << "add2b"; break;
    case I_ADD3B: f << "add3b"; break;
    case I_SUB2B: f << "sub2b"; break;
    case I_MUL2B: f << "mul2b"; break;

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
    case I_LSL4:  f << "lsl4"; break;
    case I_LSR:   f << "lsr"; break;
    case I_LSR2:  f << "lsr2"; break;
    case I_LSR4:  f << "lsr4"; break;
    case I_ASR:   f << "asr"; break;
    case I_ASR2:  f << "asr2"; break;
    case I_ASR4:  f << "asr4"; break;

    case I_AND:   f << "and"; break;
    case I_AND2:  f << "and2"; break;
    case I_AND4:  f << "and4"; break;
    case I_OR:    f << "or"; break;
    case I_OR2:   f << "or2"; break;
    case I_OR4:   f << "or4"; break;
    case I_XOR:   f << "xor"; break;
    case I_XOR2:  f << "xor2"; break;
    case I_XOR4:  f << "xor4"; break;
    case I_COMP:  f << "comp"; break;
    case I_COMP2: f << "comp2"; break;
    case I_COMP4: f << "comp4"; break;

    case I_BOOL:  f << "bool"; break;
    case I_BOOL2: f << "bool2"; break;
    case I_BOOL3: f << "bool3"; break;
    case I_BOOL4: f << "bool4"; break;
    case I_CULT:  f << "cult"; break;
    case I_CULT2: f << "cult2"; break;
    case I_CULT3: f << "cult3"; break;
    case I_CULT4: f << "cult4"; break;
    case I_CSLT:  f << "cslt"; break;
    case I_CSLT2: f << "cslt2"; break;
    case I_CSLT3: f << "cslt3"; break;
    case I_CSLT4: f << "cslt4"; break;
    case I_CFEQ:  f << "cfeq"; break;
    case I_CFLT:  f << "cflt"; break;
    case I_NOT:   f << "not"; break;

    case I_FADD:  f << "fadd"; break;
    case I_FSUB:  f << "fsub"; break;
    case I_FMUL:  f << "fmul"; break;
    case I_FDIV:  f << "fdiv"; break;
    case I_F2I:   f << "f2i"; break;
    case I_F2U:   f << "f2u"; break;
    case I_I2F:   f << "i2f"; break;
    case I_U2F:   f << "u2f"; break;

    case I_BZ:    f << "bz    " << instr.label; break;
    case I_BNZ:   f << "bnz   " << instr.label; break;
    case I_BZP:   f << "bzp   " << instr.label; break;
    case I_BNZP:  f << "bnzp  " << instr.label; break;
    case I_JMP:   f << "jmp   " << instr.label; break;
    case I_IJMP:  f << "ijmp"; break;
    case I_CALL:  f << "call  " << instr.label; break;
    case I_ICALL: f << "icall"; break;
    case I_RET:   f << "ret"; break;
    case I_SYS:
    {
        f << "sys   ";
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

static auto tie_var(compiler_global_t const* v)
{
    return std::tie(v->var.type.prim_size, v->name);
}

void compiler_t::write(std::ostream& f)
{
    // attempt to find git hash
    if(!do_suppress_githash)
    {
        std::filesystem::path p(base_path);
        std::string h;
        for(int i = 0; h.empty() && i < 64; ++i)
        {
            auto head = p / ".git" / "HEAD";
            if(std::filesystem::is_regular_file(head))
            {
                std::ifstream fhead(head);
                if(fhead.good())
                {
                    std::string t;
                    std::getline(fhead, t);
                    t = t.substr(5);
                    std::filesystem::path ref = p / ".git" / t;
                    if(std::filesystem::is_regular_file(ref))
                    {
                        std::ifstream fref(ref);
                        if(fref.good())
                            fref >> h;
                    }
                }
            }
            if(p.has_parent_path())
                p = p.parent_path();
        }
        if(!h.empty())
            f << ".githash " << h << "\n";
    }

    // .shades directive
    f << ".shades " << shades << "\n";

    // sort globals by ascending size for optimizing access

    std::vector<compiler_global_t const*> sorted_globals;

    for(auto const& [name, global] : globals)
        sorted_globals.push_back(&global);

    std::sort(sorted_globals.begin(), sorted_globals.end(),
        [](compiler_global_t const* a, compiler_global_t const* b) {
            return tie_var(a) < tie_var(b);
    });

    // saved globals first
    size_t saved_size = 0;
    for(auto const* gp : sorted_globals)
    {
        auto const& global = *gp;
        if(global.is_constexpr_ref() ||
            global.var.is_constexpr ||
            global.var.type.is_prog) continue;
        if(!global.saved) continue;
        f << ".global " << global.name << " " << global.var.type.prim_size << "\n";
        saved_size += global.var.type.prim_size;
    }

    f << ".saved " << saved_size << "\n";

    for(auto const* gp : sorted_globals)
    {
        auto const& global = *gp;
        if(global.is_constexpr_ref() ||
            global.var.is_constexpr ||
            global.var.type.is_prog) continue;
        if(global.saved) continue;
        f << ".global " << global.name << " " << global.var.type.prim_size << "\n";
    }

    f << "\n";

    for(auto const& [label, pd] : progdata)
    {
        if(pd.data.empty())
            continue;
        f << label << ":\n";
        size_t rp = 0;
        size_t rg = 0;
        size_t rl = 0;
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
            if(rl < pd.inter_labels.size() && pd.inter_labels[rl].first == i)
            {
                while(rl < pd.inter_labels.size() && pd.inter_labels[rl].first == i)
                {
                    f << pd.inter_labels[rl].second << ":\n";
                    rl += 1;
                }
                i -= 1;
                continue;
            }

            size_t num = 1;
            // TODO: optimize this?
            for(; num < 16; ++num)
            {
                if(!(i + num < pd.data.size() &&
                    (rp >= pd.relocs_prog.size() || pd.relocs_prog[rp].first > i + num) &&
                    (rg >= pd.relocs_glob.size() || pd.relocs_glob[rg].first > i + num) &&
                    (rl >= pd.inter_labels.size() || pd.inter_labels[rl].first > i + num) ))
                    break;
            }

            {
                char hex[16];
                snprintf(hex, sizeof(hex), "  .b %1x", (int)num - 1);
                f << hex;
                for(int j = 0; j < num; ++j)
                {
                    snprintf(hex, sizeof(hex), " %02x", pd.data[i + j]);
                    f << hex;
                }
                f << "\n";
                i += num - 1;
                continue;
            }
        }
    }

    f << "\n";

    for(auto const& [name, func] : funcs)
    {
        uint16_t line = 0;
        uint16_t file = 0;
        f << name << ":\n";
        //f << "  .file " << func.filename << ".abc\n";
        for(auto const& instr : func.instrs)
            write_instr(f, instr, line, file, debug_filenames);
        f << "\n";
    }
}

}
