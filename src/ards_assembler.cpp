#include "ards_assembler.hpp"

#include <sstream>
#include <assert.h>

namespace ards
{

std::unordered_map<std::string, sysfunc_t> const sys_names =
{
    { "display",          SYS_DISPLAY          },
    { "draw_pixel",       SYS_DRAW_PIXEL       },
    { "draw_filled_rect", SYS_DRAW_FILLED_RECT },
    { "set_frame_rate",   SYS_SET_FRAME_RATE   },
    { "next_frame",       SYS_NEXT_FRAME       },
    { "idle",             SYS_IDLE             },
    { "debug_break",      SYS_DEBUG_BREAK      },
    { "assert",           SYS_ASSERT           },
};

static bool isdigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool isalpha(char c)
{
    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_';
}

static bool isalnum(char c)
{
    return isalpha(c) || c >= '0' && c <= '9';
}

static char tolower(char c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

bool check_label(std::string const& t, error_t& e)
{
    if(t.empty()) e.msg = "Expected label";
    if(!isalpha(t[0]) && t[0] != '$') e.msg = "Label \"" + t + "\" must begin with[a - zA - Z_]";
    for(size_t i = 1; i < t.size(); ++i)
    {
        if(!isalnum(t[i]))
        {
            e.msg = "Label \"" + t + "\" has an invalid character";
            break;
        }
    }
    return e.msg.empty();
}

std::string read_label(std::istream& f, error_t& e)
{
    std::string t;
    f >> t;
    for(char& c : t)
        c = tolower(c);
    return check_label(t, e) ? t : "";
}

uint16_t read_sys(std::istream& f, error_t& e)
{
    std::string t;
    f >> t;
    auto it = sys_names.find(t);
    if(it == sys_names.end())
    {
        e.msg = "Undefined sys name \"" + t + "\"";
        return UINT16_MAX;
    }
    return it->second;
}

static int hex2val(char c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c + (10 - 'a');
    return -1;
}

uint32_t read_imm(std::istream& f, error_t& e)
{
#if 1
    int64_t x;
    f >> x;
#else
    std::string t;
    f >> t;
    if(t.size() != 2 && t.size() != 4 && t.size() != 6)
    {
        e.msg = "Bad immediate size: \"" + t + "\"";
        return 0;
    }
    uint32_t x = 0;
    for(size_t i = 0; i < t.size(); i += 2)
    {
        int v0 = hex2val(t[i + 0]);
        int v1 = hex2val(t[i + 1]);
        if(v0 < 0 || v1 < 0)
        {
            e.msg = "Bad immediate format: \"" + t + "\"";
            return 0;
        }
        x <<= 8;
        x += (v0 * 16 + v1);
    }
#endif
    return x;
}

void assembler_t::push_global(std::istream& f)
{
    std::string label = read_label(f, error);
    if(!error.msg.empty()) return;
    uint32_t offset = 0;
    if(f.peek() == '+')
    {
        f.get();
        offset = read_imm(f, error);
    }
    nodes.push_back({ byte_count, GLOBAL, I_NOP, 3, offset, label });
    byte_count += 2;
}

static std::unordered_map<std::string, instr_t> const SINGLE_INSTR_NAMES =
{
    { "p0", I_P0 },
    { "p1", I_P1 },
    { "p2", I_P2 },
    { "p3", I_P3 },
    { "p4", I_P4 },
    { "p00", I_P00 },
    { "sext", I_SEXT },
    { "dup", I_DUP },
    { "getr", I_GETR },
    { "getr2", I_GETR2 },
    { "setr", I_SETR },
    { "setr2", I_SETR2 },
    { "pop", I_POP },
    { "pop2", I_POP2 },
    { "pop3", I_POP3 },
    { "pop4", I_POP4 },
    { "inc", I_INC },
    { "add", I_ADD },
    { "add2", I_ADD2 },
    { "add3", I_ADD3 },
    { "add4", I_ADD4 },
    { "sub", I_SUB },
    { "sub2", I_SUB2 },
    { "sub3", I_SUB3 },
    { "sub4", I_SUB4 },
    { "mul", I_MUL },
    { "mul2", I_MUL2 },
    { "mul3", I_MUL3 },
    { "mul4", I_MUL4 },
    { "udiv2", I_UDIV2 },
    { "udiv4", I_UDIV4 },
    { "div2", I_DIV2 },
    { "div4", I_DIV4 },
    { "umod2", I_UMOD2 },
    { "umod4", I_UMOD4 },
    { "mod2", I_MOD2 },
    { "mod4", I_MOD4 },
    { "lsl", I_LSL },
    { "lsl2", I_LSL2 },
    { "lsl3", I_LSL3 },
    { "lsl4", I_LSL4 },
    { "lsr", I_LSR },
    { "lsr2", I_LSR2 },
    { "lsr3", I_LSR3 },
    { "lsr4", I_LSR4 },
    { "asr", I_ASR },
    { "asr2", I_ASR2 },
    { "asr3", I_ASR3 },
    { "asr4", I_ASR4 },
    { "bool", I_BOOL },
    { "bool2", I_BOOL2 },
    { "bool3", I_BOOL3 },
    { "bool4", I_BOOL4 },
    { "cult", I_CULT },
    { "cult2", I_CULT2 },
    { "cult3", I_CULT3 },
    { "cult4", I_CULT4 },
    { "cule", I_CULE },
    { "cule2", I_CULE2 },
    { "cule3", I_CULE3 },
    { "cule4", I_CULE4 },
    { "cslt", I_CSLT },
    { "cslt2", I_CSLT2 },
    { "cslt3", I_CSLT3 },
    { "cslt4", I_CSLT4 },
    { "csle", I_CSLE },
    { "csle2", I_CSLE2 },
    { "csle3", I_CSLE3 },
    { "csle4", I_CSLE4 },
    { "not", I_NOT },
    { "ret", I_RET },
};

error_t assembler_t::assemble(std::istream& f)
{
	std::string t;

    //counting_stream_buffer counting_buf(f_orig.rdbuf(), error);
    //std::istream f(&counting_buf);

    while(error.msg.empty() && !f.eof())
    {
        t.clear();
        f >> t;
        if(t.empty()) continue;
        for(char& c : t)
            c = tolower(c);
        if(t.back() == ':')
        {
            t.pop_back();
            if(labels.count(t) != 0)
                error.msg = "Duplicate label: \"" + t + "\"";
            else if(check_label(t, error))
                labels[t] = nodes.size();
        }
        else if(auto it = SINGLE_INSTR_NAMES.find(t); it != SINGLE_INSTR_NAMES.end())
        {
            push_instr(it->second);
        }
        else if(t == "push")
        {
            push_instr(I_PUSH);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "getl")
        {
            push_instr(I_GETL);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "getl2")
        {
            push_instr(I_GETL2);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "getln")
        {
            push_instr(I_GETLN);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "setl")
        {
            push_instr(I_SETL);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "setln")
        {
            push_instr(I_SETLN);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "getg")
        {
            push_instr(I_GETG);
            push_global(f);
        }
        else if(t == "getgn")
        {
            push_instr(I_GETGN);
            push_global(f);
        }
        else if(t == "setg")
        {
            push_instr(I_SETG);
            push_global(f);
        }
        else if(t == "setgn")
        {
            push_instr(I_SETGN);
            push_global(f);
        }
        else if(t == "getrn")
        {
            push_instr(I_GETRN);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "setrn")
        {
            push_instr(I_SETRN);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "aidxb")
        {
            push_instr(I_AIDXB);
            push_imm(read_imm(f, error), 1);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "aidx")
        {
            push_instr(I_AIDX);
            push_imm(read_imm(f, error), 2);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "refl")
        {
            push_instr(I_REFL);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "refg")
        {
            push_instr(I_REFG);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "refgb")
        {
            push_instr(I_REFGB);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "bz")
        {
            push_instr(I_BZ);
            push_label(read_label(f, error));
        }
        else if(t == "bnz")
        {
            push_instr(I_BNZ);
            push_label(read_label(f, error));
        }
        else if(t == "jmp")
        {
            push_instr(I_JMP);
            push_label(read_label(f, error));
        }
        else if(t == "call")
        {
            push_instr(I_CALL);
            push_label(read_label(f, error));
        }
        else if(t == "sys")
        {
            auto i = read_sys(f, error) * 2;
            if(i < 256)
            {
                push_instr(I_SYSB);
                push_imm(i, 1);
            }
            else
            {
                push_instr(I_SYS);
                push_imm(i, 2);
            }
        }
        else if(t == ".global")
        {
            std::string label = read_label(f, error);
            if(!error.msg.empty()) break;
            if(globals.count(label) != 0)
            {
                error.msg = "Duplicate global \"" + label + "\"";
                break;
            }
            uint32_t t = read_imm(f, error);
            if(!error.msg.empty()) break;
            if(t == 0)
            {
                error.msg = "Global \"" + label + "\" has zero size";
                break;
            }
            globals[label] = globals_bytes;
            globals_bytes += t;
        }
        else if(t == ".b")
        {
            push_imm(read_imm(f, error), 1);
        }
        else
        {
            error.msg = "Unknown instruction or directive \"" + t + "\"";
        }
	}

    return error;
}

void assembler_t::relax_jumps()
{
    for(size_t i = 0; i < nodes.size(); ++i)
    {
        auto& n = nodes[i];
        if( n.instr != I_JMP &&
            n.instr != I_CALL &&
            n.instr != I_BZ &&
            n.instr != I_BNZ &&
            true) continue;
        assert(i + 1 < nodes.size());
        auto& label = nodes[i + 1];
        assert(label.type == LABEL);

        auto it = labels.find(label.label);
        if(it == labels.end()) return;
        size_t addr = it->second;

        int32_t offset = int32_t(addr) - int32_t(n.offset);
        int32_t abs_offset = offset < 0 ? -offset : offset;

        if(abs_offset < 124)
        {
            n.instr = instr_t(n.instr + 1);
            label.size = 1;
        }

        int bytes_shortened = 3 - label.size;
        if(bytes_shortened)
        {
            for(size_t j = i + 1; j < nodes.size(); ++j)
            {
                nodes[j].offset -= bytes_shortened;
            }
        }
    }
}

error_t assembler_t::link()
{
    linked_data.clear();

    relax_jumps();

    if(globals_bytes > 1024)
    {
        std::ostringstream ss;
        ss << "Too much global data: " << globals_bytes << " bytes (1024 max)";
        error.msg = ss.str();
        return error;
    }

    // add signature: 0xABC00ABC in big-endian order
    linked_data.push_back(0xAB);
    linked_data.push_back(0xC0);
    linked_data.push_back(0x0A);
    linked_data.push_back(0xBC);

    linked_data.push_back(I_CALL);
    {
        auto it = labels.find("main");
        if(it == labels.end())
        {
            error.msg = "No entry point \"main\" found";
            return error;
        }
        size_t index = it->second;
        assert(index < nodes.size());
        auto offset = nodes[index].offset;
        linked_data.push_back(uint8_t(offset >> 0));
        linked_data.push_back(uint8_t(offset >> 8));
        linked_data.push_back(uint8_t(offset >> 16));
    }

    linked_data.push_back(I_JMP);
    linked_data.push_back(4);
    linked_data.push_back(0);
    linked_data.push_back(0);

    for(size_t i = 0; i < nodes.size(); ++i)
    {
        auto const& n = nodes[i];
        if(n.type == INSTR)
        {
            linked_data.push_back(n.instr);
        }
        else if(n.type == IMM)
        {
            if(n.size >= 1) linked_data.push_back(uint8_t(n.imm >> 0));
            if(n.size >= 2) linked_data.push_back(uint8_t(n.imm >> 8));
            if(n.size >= 3) linked_data.push_back(uint8_t(n.imm >> 16));
        }
        else if(n.type == GLOBAL)
        {
            auto it = globals.find(n.label);
            if(it == globals.end())
            {
                error.msg = "Global not found: \"" + n.label + "\"";
                return error;
            }
            size_t offset = it->second;
            linked_data.push_back(uint8_t(offset >> 0));
            linked_data.push_back(uint8_t(offset >> 8));
        }
        else if(n.type == LABEL)
        {
            auto it = labels.find(n.label);
            if(it == labels.end())
            {
                error.msg = "Label not found: \"" + n.label + "\"";
                return error;
            }
            size_t index = it->second;
            if(index < nodes.size())
            {
                auto offset = nodes[index].offset;
                if(n.size < 3)
                {
                    offset -= n.offset;
                    offset -= (n.size + 2);
                }
                if(n.size >= 1) linked_data.push_back(uint8_t(offset >> 0));
                if(n.size >= 2) linked_data.push_back(uint8_t(offset >> 8));
                if(n.size >= 3) linked_data.push_back(uint8_t(offset >> 16));
            }
            else
            {
                error.msg = "Label \"" + n.label + "\" has no associated code";
                return error;
            }
        }
    }

    // insert dev length at end
    size_t pages = (linked_data.size() + 255 + 2) / 256;
    size_t size = pages * 256;
    linked_data.resize(size);
    pages = 65536 - pages;
    linked_data[size - 2] = uint8_t(pages >> 0);
    linked_data[size - 1] = uint8_t(pages >> 8);

    return error;
}

}
