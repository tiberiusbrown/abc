#include "ards_assembler.hpp"

#include <sstream>
#include <assert.h>

namespace ards
{

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
    if(!isalpha(t[0]) && t[0] != '$')
        e.msg = "Label \"" + t + "\" must begin with[a - zA - Z_]";
    for(size_t i = 1; i < t.size(); ++i)
    {
        if(!isalnum(t[i]) && t[i] != '$' && t[i] != '.')
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

uint32_t read_imm(std::istream& f, error_t& e)
{
#if 1
    (void)e;
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
    return (uint32_t)x;
}

void assembler_t::push_instr(instr_t i)
{
    nodes.push_back({ byte_count, INSTR, i, 1 });
    byte_count += 1;
}

void assembler_t::push_label(std::istream& f, bool has_offset)
{
    std::string label = read_label(f, error);
    if(!error.msg.empty()) return;
    uint32_t offset = 0;
    if(has_offset)
        f >> offset;
    nodes.push_back({ byte_count, LABEL, I_NOP, (uint16_t)3, offset, label });
    byte_count += 3;
}

void assembler_t::push_global(std::istream& f, bool has_offset)
{
    std::string label = read_label(f, error);
    if(!error.msg.empty()) return;
    uint32_t offset = 0;
    if(has_offset)
        f >> offset;
    nodes.push_back({ byte_count, GLOBAL, I_NOP, (uint16_t)2, offset, label });
    byte_count += 2;
}

static std::unordered_map<std::string, instr_t> const SINGLE_INSTR_NAMES =
{
    { "nop", I_NOP },
    { "p0", I_P0 },
    { "p1", I_P1 },
    { "p2", I_P2 },
    { "p3", I_P3 },
    { "p4", I_P4 },
    { "p5", I_P5 },
    { "p6", I_P6 },
    { "p7", I_P7 },
    { "p8", I_P8 },
    { "p16", I_P16 },
    { "p32", I_P32 },
    { "p64", I_P64 },
    { "p128", I_P128 },
    { "p00", I_P00 },
    { "p000", I_P000 },
    { "p0000", I_P0000 },
    { "sext", I_SEXT },
    { "sext2", I_SEXT2 },
    { "sext3", I_SEXT3 },
    { "dup", I_DUP },
    { "dup2", I_DUP2 },
    { "dup3", I_DUP3 },
    { "dup4", I_DUP4 },
    { "dup5", I_DUP5 },
    { "dup6", I_DUP6 },
    { "dup7", I_DUP7 },
    { "dup8", I_DUP8 },
    { "dupw", I_DUPW },
    { "dupw2", I_DUPW2 },
    { "dupw3", I_DUPW3 },
    { "dupw4", I_DUPW4 },
    { "dupw5", I_DUPW5 },
    { "dupw6", I_DUPW6 },
    { "dupw7", I_DUPW7 },
    { "dupw8", I_DUPW8 },
    { "getr", I_GETR },
    { "getr2", I_GETR2 },
    { "setr", I_SETR },
    { "setr2", I_SETR2 },
    { "pop", I_POP },
    { "pop2", I_POP2 },
    { "pop3", I_POP3 },
    { "pop4", I_POP4 },
    { "inc", I_INC },
    { "dec", I_DEC },
    { "pinc", I_PINC },
    { "pinc2", I_PINC2 },
    { "pinc3", I_PINC3 },
    { "pinc4", I_PINC4 },
    { "pdec", I_PDEC },
    { "pdec2", I_PDEC2 },
    { "pdec3", I_PDEC3 },
    { "pdec4", I_PDEC4 },
    { "pincf", I_PINCF },
    { "pdecf", I_PDECF },
    { "add", I_ADD },
    { "add2", I_ADD2 },
    { "add3", I_ADD3 },
    { "add4", I_ADD4 },
    { "sub", I_SUB },
    { "sub2", I_SUB2 },
    { "sub3", I_SUB3 },
    { "sub4", I_SUB4 },
    { "add2b", I_ADD2B },
    { "add3b", I_ADD3B },
    { "sub2b", I_SUB2B },
    { "mul2b", I_MUL2B },
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
    { "lsl4", I_LSL4 },
    { "lsr", I_LSR },
    { "lsr2", I_LSR2 },
    { "lsr4", I_LSR4 },
    { "and",  I_AND },
    { "and2", I_AND2 },
    { "and4", I_AND4 },
    { "or",  I_OR },
    { "or2", I_OR2 },
    { "or4", I_OR4 },
    { "xor",  I_XOR },
    { "xor2", I_XOR2 },
    { "xor4", I_XOR4 },
    { "comp",  I_COMP },
    { "comp2", I_COMP2 },
    { "comp4", I_COMP4 },
    { "asr", I_ASR },
    { "asr2", I_ASR2 },
    { "asr4", I_ASR4 },
    { "bool", I_BOOL },
    { "bool2", I_BOOL2 },
    { "bool3", I_BOOL3 },
    { "bool4", I_BOOL4 },
    { "cult", I_CULT },
    { "cult2", I_CULT2 },
    { "cult3", I_CULT3 },
    { "cult4", I_CULT4 },
    { "cslt", I_CSLT },
    { "cslt2", I_CSLT2 },
    { "cslt3", I_CSLT3 },
    { "cslt4", I_CSLT4 },
    { "cfeq", I_CFEQ },
    { "cflt", I_CFLT },
    { "not", I_NOT },
    { "fadd", I_FADD },
    { "fsub", I_FSUB },
    { "fmul", I_FMUL },
    { "fdiv", I_FDIV },
    { "f2i", I_F2I },
    { "f2u", I_F2U },
    { "i2f", I_I2F },
    { "u2f", I_U2F },
    { "ret", I_RET },
};

error_t assembler_t::assemble(std::istream& f)
{
    std::string t;

    //counting_stream_buffer counting_buf(f_orig.rdbuf(), error);
    //std::istream f(&counting_buf);

    data_bytes = 0;
    while(error.msg.empty() && !f.eof())
    {
        t.clear();
        if(!(f >> t))
            break;
        if(t.empty()) continue;
        if(t[0] == ';')
        {
            std::string line;
            std::getline(f, line);
            continue;
        }
        if(t.back() == ':')
        {
            t.pop_back();
            if(labels.count(t) != 0)
                error.msg = "Duplicate label: \"" + t + "\"";
            else if(check_label(t, error))
                labels[t] = nodes.size();
            continue;
        }
        for(char& c : t)
            c = tolower(c);
        if(auto it = SINGLE_INSTR_NAMES.find(t); it != SINGLE_INSTR_NAMES.end())
        {
            push_instr(it->second);
        }
        else if(t == "push")
        {
            push_instr(I_PUSH);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "pushg")
        {
            std::string label = read_label(f, error);
            if(!error.msg.empty()) return error;
            uint32_t offset = 0;
            f >> offset;
            if(auto git = globals.find(label); git != globals.end())
            {
                uint32_t imm = (uint32_t)git->second + offset;
                if(imm < 256)
                {
                    push_instr(I_REFGB);
                    push_imm(imm, 1);
                }
                else
                {
                    imm += 0x200;
                    push_instr(I_PUSHG);
                    push_imm(imm, 2);
                }
            }
            else
                return { "Unknown global \"" + label + "\"" };
        }
        else if(t == "push2")
        {
            push_instr(I_PUSHG);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "pushl")
        {
            push_instr(I_PUSHL);
            push_label(f, true);
        }
        else if(t == "push3")
        {
            push_instr(I_PUSHL);
            push_imm(read_imm(f, error), 3);
        }
        else if(t == "push4")
        {
            push_instr(I_PUSH4);
            push_imm(read_imm(f, error), 4);
        }
        else if(t == "pzn")
        {
            push_instr(I_PZN);
            auto n = read_imm(f, error);
            assert(n >= 5);
            if(n < 5)
            {
                error.msg = "PZN instr imm must be >= 5";
                continue;
            }
            push_imm(n - 3, 1);
        }
        else if(t == "popn")
        {
            push_instr(I_POPN);
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
        else if(t == "getl4")
        {
            push_instr(I_GETL4);
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
        else if(t == "setl2")
        {
            push_instr(I_SETL2);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "setl4")
        {
            push_instr(I_SETL4);
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
            push_global(f, true);
        }
        else if(t == "getg2")
        {
            push_instr(I_GETG2);
            push_global(f, true);
        }
        else if(t == "getg4")
        {
            push_instr(I_GETG4);
            push_global(f, true);
        }
        else if(t == "getgn")
        {
            push_instr(I_GETGN);
            push_global(f, true);
        }
        else if(t == "setg")
        {
            push_instr(I_SETG);
            push_global(f, true);
        }
        else if(t == "setg2")
        {
            push_instr(I_SETG2);
            push_global(f, true);
        }
        else if(t == "setg4")
        {
            push_instr(I_SETG4);
            push_global(f, true);
            }
        else if(t == "setgn")
        {
            push_instr(I_SETGN);
            push_global(f, true);
        }
        else if(t == "getp")
        {
            push_instr(I_GETP);
        }
        else if(t == "getpn")
        {
            push_instr(I_GETPN);
            push_imm(read_imm(f, error), 1);
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
        else if(t == "aixb1")
        {
            push_instr(I_AIXB1);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "aidx")
        {
            push_instr(I_AIDX);
            push_imm(read_imm(f, error), 2);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "pidxb")
        {
            push_instr(I_PIDXB);
            push_imm(read_imm(f, error), 1);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "pidx")
        {
            push_instr(I_PIDX);
            push_imm(read_imm(f, error), 2);
            push_imm(read_imm(f, error), 3);
        }
        else if(t == "uaidx")
        {
            push_instr(I_UAIDX);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "upidx")
        {
            push_instr(I_UPIDX);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "aslc")
        {
            push_instr(I_ASLC);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "pslc")
        {
            push_instr(I_PSLC);
            push_imm(read_imm(f, error), 2);
        }
        else if(t == "refl")
        {
            push_instr(I_REFL);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "refgb")
        {
            assert(false);
        }
        else if(t == "linc")
        {
            push_instr(I_LINC);
            push_imm(read_imm(f, error), 1);
        }
        else if(t == "bz")
        {
            push_instr(I_BZ);
            push_label(f);
        }
        else if(t == "bnz")
        {
            push_instr(I_BNZ);
            push_label(f);
        }
        else if(t == "bzp")
        {
            push_instr(I_BZP);
            push_label(f);
        }
        else if(t == "bnzp")
        {
            push_instr(I_BNZP);
            push_label(f);
        }
        else if(t == "jmp")
        {
            push_instr(I_JMP);
            push_label(f);
        }
        else if(t == "call")
        {
            push_instr(I_CALL);
            push_label(f);
        }
        else if(t == "sys")
        {
            auto i = read_sys(f, error) * 2;
            push_instr(I_SYS);
            push_imm(i, 1);
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
            uint32_t ti = read_imm(f, error);
            if(!error.msg.empty()) break;
            if(ti == 0)
            {
                error.msg = "Global \"" + label + "\" has zero size";
                break;
            }
            globals[label] = globals_bytes;
            globals_bytes += ti;
        }
        else if(t == ".b")
        {
            auto flags = f.flags();
            f >> std::hex;
            int num;
            f >> num;
            for(int i = 0; i <= num; ++i)
            {
                int x;
                f >> x;
                push_imm(x, 1);
            }
            f.flags(flags);
            data_bytes += num + 1;
        }
        else if(t == ".rg")
        {
            push_global(f);
            data_bytes += 2;
        }
        else if(t == ".rp")
        {
            push_label(f);
            data_bytes += 3;
        }
        else if(t == ".file")
        {
            std::string filename;
            uint8_t file = 0;
            f >> filename;
            auto fit = file_table.find(filename);
            if(fit == file_table.end())
            {
                file = (uint8_t)file_table.size();
                if(file_table.size() >= 256)
                {
                    error.msg = "Too many input files";
                    return error;
                }
                file_table[filename] = file;
            }
            else file = fit->second;
            push_file(file);
        }
        else if(t == ".line")
        {
            uint16_t line;
            f >> line;
            push_line(line);
        }
        else if(t == ".saved")
        {
            f >> saved_bytes;
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
        if(n.instr != I_JMP &&
            n.instr != I_CALL &&
            n.instr != I_BZ &&
            n.instr != I_BNZ &&
            n.instr != I_BZP &&
            n.instr != I_BNZP &&
            true) continue;
        assert(i + 1 < nodes.size());
        auto& label = nodes[i + 1];
        assert(label.type == LABEL);

        auto it = labels.find(label.label);
        if(it == labels.end())
            return;
        if(it->second >= nodes.size())
            return;
        size_t addr = nodes[it->second].offset;

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

    // offset 0: signature 0xABC00ABC in big-endian order
    linked_data.push_back(0xAB);
    linked_data.push_back(0xC0);
    linked_data.push_back(0x0A);
    linked_data.push_back(0xBC);

    // offset 4: 6 dummy bytes
    for(int i = 0; i < 6; ++i)
        linked_data.push_back(0);

    // offset 10: save size
    linked_data.push_back(uint8_t(saved_bytes >> 0));
    linked_data.push_back(uint8_t(saved_bytes >> 8));

    // offset 12: file table size (1 byte) and offset (3 bytes)
    for(int i = 0; i < 4; ++i)
        linked_data.push_back(0);

    // offset 16: line table offset (3 bytes + 1 dummy byte)
    for(int i = 0; i < 4; ++i)
        linked_data.push_back(0);

    // offset 20: call $globinit, loop { call main }
    linked_data.push_back(I_CALL);
    {
        auto it = labels.find("$globinit");
        assert(it != labels.end());
        size_t index = it->second;
        assert(index < nodes.size());
        auto offset = nodes[index].offset;
        linked_data.push_back(uint8_t(offset >> 0));
        linked_data.push_back(uint8_t(offset >> 8));
        linked_data.push_back(uint8_t(offset >> 16));
    }
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
    linked_data.push_back(24);
    linked_data.push_back(0);
    linked_data.push_back(0);

    // header size is fixed at 256 bytes
    linked_data.resize(256);

    prev_pc_offset = 0;
    uint8_t current_file = 0;
    uint16_t current_line = 0;

    code_bytes = 0;
    debug_bytes = 0;
    for(size_t i = 0; i < nodes.size(); ++i)
    {
        auto const& n = nodes[i];

        switch(n.type)
        {
        case INSTR:
            linked_data.push_back(n.instr);
            code_bytes += 1;
            break;
        case IMM:
            if(n.size >= 1) linked_data.push_back(uint8_t(n.imm >> 0));
            if(n.size >= 2) linked_data.push_back(uint8_t(n.imm >> 8));
            if(n.size >= 3) linked_data.push_back(uint8_t(n.imm >> 16));
            if(n.size >= 4) linked_data.push_back(uint8_t(n.imm >> 24));
            code_bytes += n.size;
            break;
        case GLOBAL:
        {
            size_t offset = 0;
            if(n.label.empty())
                offset = n.imm;
            else
            {
                auto it = globals.find(n.label);
                if(it == globals.end())
                {
                    error.msg = "Global not found: \"" + n.label + "\"";
                    return error;
                }
                offset = it->second;
                offset += n.imm;
                offset += 0x200;
            }
            linked_data.push_back(uint8_t(offset >> 0));
            if(n.size >= 2)
                linked_data.push_back(uint8_t(offset >> 8));
            code_bytes += n.size;
            break;
        }
        case LABEL:
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
                offset += n.imm;
                if(n.size >= 1) linked_data.push_back(uint8_t(offset >> 0));
                if(n.size >= 2) linked_data.push_back(uint8_t(offset >> 8));
                if(n.size >= 3) linked_data.push_back(uint8_t(offset >> 16));
                code_bytes += n.size;
            }
            else
            {
                error.msg = "Label \"" + n.label + "\" has no associated code";
                return error;
            }
            break;
        }
        case FILE:
        {
            uint8_t file = (uint8_t)n.imm;
            if(file == current_file)
                break;
            advance_pc_offset();
            line_table.push_back(253);
            line_table.push_back(file);
            current_file = file;
            break;
        }
        case LINE:
        {
            uint16_t line = (uint16_t)n.imm;
            if(line == current_line)
                break;
            advance_pc_offset();
            if(line > current_line && line < current_line + 126)
            {
                uint32_t t = line - current_line + 127;
                assert(t >= 128 && t <= 252);
                line_table.push_back((uint8_t)t);
            }
            else
            {
                line_table.push_back(254);
                line_table.push_back(uint8_t(line >> 0));
                line_table.push_back(uint8_t(line >> 8));
            }
            current_line = line;
            break;
        }
        case CUSTOM_DATA:
        {
            auto it = custom_labels.find(n.label);
            assert(it != custom_labels.end());
            auto const& d = it->second;
            linked_data.insert(linked_data.end(), d.begin(), d.end());
            data_bytes += d.size();
            code_bytes += d.size();
            break;
        }
        default:
            break;
        }

    }
    code_bytes -= data_bytes;

    // add file table info (offset 12)
    linked_data[12] = (uint8_t)file_table.size();
    linked_data[13] = uint8_t(linked_data.size() >> 0);
    linked_data[14] = uint8_t(linked_data.size() >> 8);
    linked_data[15] = uint8_t(linked_data.size() >> 16);
    {
        std::vector<std::string> files(file_table.size());
        for(auto const& [k, v] : file_table)
            files[v] = k;
        for(auto const& f : files)
        {
            for(size_t i = 0; i < FILE_TABLE_STRING_LENGTH; ++i)
            {
                if(i >= f.size() || i == FILE_TABLE_STRING_LENGTH - 1)
                    linked_data.push_back(0);
                else
                    linked_data.push_back((uint8_t)f[i]);
                debug_bytes += 1;
            }
        }
    }

    // add line table info (offset 16)
    linked_data[16] = uint8_t(linked_data.size() >> 0);
    linked_data[17] = uint8_t(linked_data.size() >> 8);
    linked_data[18] = uint8_t(linked_data.size() >> 16);
    linked_data.insert(linked_data.end(), line_table.begin(), line_table.end());
    debug_bytes += line_table.size();

    // page-align and insert dev length at end
    size_t pages = (linked_data.size() + 255 + 6) / 256;
    size_t size = pages * 256;
    linked_data.resize(size);
    pages = 65536 - pages;
    if(has_save())
        pages -= 16;

    linked_data[size - 6] = uint8_t(pages >> 0);
    linked_data[size - 5] = uint8_t(pages >> 8);

    linked_data[size - 4] = 0xAB;
    linked_data[size - 3] = 0xCE;
    linked_data[size - 2] = 0xEA;
    linked_data[size - 1] = 0xBC;

    // TODO: revisit this
    if(has_save())
    {
        for(int i = 0; i < 4096; ++i)
            linked_data.push_back(0xff);
    }

    return error;
}

void assembler_t::advance_pc_offset()
{
    uint32_t offset = (uint32_t)linked_data.size();
    if(offset == prev_pc_offset)
        return;
    if(offset > prev_pc_offset && offset < prev_pc_offset + 129)
    {
        uint32_t t = offset - prev_pc_offset - 1;
        assert(t >= 0 && t <= 127);
        line_table.push_back((uint8_t)t);
    }
    else
    {
        line_table.push_back(255);
        line_table.push_back(uint8_t(offset >> 0));
        line_table.push_back(uint8_t(offset >> 8));
        line_table.push_back(uint8_t(offset >> 16));
    }
    prev_pc_offset = offset;
}

error_t assembler_t::add_custom_label(
        std::string const& name,
        std::vector<uint8_t> const& data)
{
    if(labels.count(name) + custom_labels.count(name) != 0)
        return { "Duplicate label: \"" + name + "\"" };

    labels[name] = nodes.size();
    nodes.push_back({
        byte_count, CUSTOM_DATA, I_NOP, (uint32_t)data.size(), 0, name });
    custom_labels[name] = data;
    return {};
}

}
