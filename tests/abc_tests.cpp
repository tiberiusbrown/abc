#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <ards_assembler.hpp>
#include <ards_compiler.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <strstream>

#include <cassert>
#include <cstdio>

static std::unique_ptr<absim::arduboy_t> arduboy;

static bool test(std::string const& fpath, std::string const& fname)
{
    std::string abc_asm;
    std::vector<uint8_t> binary;

    if(fname.size() < 4) return false;
    std::string name = fname.substr(0, fname.size() - 4);

    {
        ards::compiler_t c{};
        std::ostringstream fo;
        c.compile(fpath, name, fo);
        for(auto const& e : c.errors())
            printf("Line %d: %s\n", (int)e.line_info.first, e.msg.c_str());
        assert(c.errors().empty());
        abc_asm = fo.str();
    }

    {
        ards::assembler_t a{};
        std::istrstream ss(abc_asm.data(), (int)abc_asm.size());
        auto e = a.assemble(ss);
        assert(e.msg.empty());
        e = a.link();
        assert(e.msg.empty());
        binary = a.data();
    }

    {
        std::istrstream ss((char const*)VM_HEX_ARDUBOYFX, VM_HEX_ARDUBOYFX_SIZE);
        auto t = arduboy->load_file("vm.hex", ss);
        assert(t.empty());
    }
    {
        std::istrstream ss((char const*)binary.data(), binary.size());
        auto t = arduboy->load_file("fxdata.bin", ss);
        assert(t.empty());
    }

    arduboy->reset();
    arduboy->allow_nonstep_breakpoints = true;
    arduboy->cpu.enabled_autobreaks.reset();
    arduboy->breakpoints_wr.reset();
    arduboy->cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(1'000'000'000'000ull); // up to 1 second
    if(!arduboy->paused)
        return false;
    arduboy->paused = false;
    arduboy->breakpoints_wr.set(0x0665); // set breakpoint when writing to vm::error
    arduboy->advance(1'000'000'000'000ull); // up to 1 second
    if(!arduboy->paused)
        return false;
    return arduboy->cpu.data[0x0665] == 0;
}

int main()
{
    int r = 0;
    namespace fs = std::filesystem;
    arduboy = std::make_unique<absim::arduboy_t>();

    for(auto const& entry : fs::recursive_directory_iterator(TESTS_DIR))
    {
        char const* status = "Pass";
        if(!test(entry.path().parent_path().string(), entry.path().filename().string()))
            status = "fail", r = 1;
        printf("%-20s %s\n", entry.path().filename().string().c_str(), status);
    }

    return r;
}
