#include "ards_assembler.hpp"

#include "ards_countstream.hpp"

#include <assert.h>

namespace ards
{

static std::unordered_map<std::string, uint16_t> const sys_names =
{
    { "display",        0 },
    { "draw_pixel",     1 },
    { "set_frame_rate", 2 },
    { "next_frame",     3 },
    { "idle",           4 },
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
    if(!isalpha(t[0])) e.msg = "Label \"" + t + "\" must begin with[a - zA - Z_]";
    for(auto c : t)
    {
        if(!isalnum(c))
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
    return x;
}

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
        else if(t == "setl")
        {
            push_instr(I_SETL);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "setl2")
        {
            push_instr(I_SETL2);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "pop")
            push_instr(I_POP);
        else if(t == "add")
            push_instr(I_ADD);
        else if(t == "add2")
            push_instr(I_ADD2);
        else if(t == "sub")
            push_instr(I_SUB);
        else if(t == "sub2")
            push_instr(I_SUB2);
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
        else if(t == "bneg")
        {
            push_instr(I_BNEG);
            push_label(read_label(f, error));
        }
        else if(t == "jmp")
        {
            push_instr(I_JMP);
            push_label(read_label(f, error));
        }
        else if(t == "sys")
        {
            push_instr(I_SYS);
            push_imm(read_sys(f, error) * 2, 2);
        }
        else
        {
            error.msg = "Unknown instruction or directive \"" + t + "\"";
        }
	}

    return error;
}


error_t assembler_t::link()
{
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
        else if(n.type == LABEL)
        {
            auto it = labels.find(n.label);
            if(it == labels.end())
            {
                error.msg = "Label not found: \"" + n.label + "\"";
                return error;
            }
            if(it != labels.end())
            {
                size_t index = it->second;
                if(index < nodes.size())
                {
                    auto offset = nodes[index].offset;
                    linked_data.push_back(uint8_t(offset >> 0));
                    linked_data.push_back(uint8_t(offset >> 8));
                    linked_data.push_back(uint8_t(offset >> 16));
                }
                else
                {
                    error.msg = "Label \"" + n.label + "\" has no associated code";
                    return error;
                }
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
