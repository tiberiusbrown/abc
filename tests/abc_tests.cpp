#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <abc_assembler.hpp>
#include <abc_compiler.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <abc_interp.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <strstream>

#include <cassert>
#include <cstdio>

static std::unique_ptr<absim::arduboy_t> arduboy;

static abc_interp_t interp;

static bool test(std::string const& fpath, std::string const& fname)
{
    std::vector<uint8_t> binary;

    if(fname.size() < 4) return false;
    std::string name = fname.substr(0, fname.size() - 4);
    abc::compiler_t c{};

    {
        std::ofstream fasm((fpath + "/../asm/" + fname + ".asm.txt").c_str());
        c.suppress_githash();
        std::ostringstream fo;
        c.compile(fpath, name, fasm);
        for(auto const& e : c.errors())
            printf("Line %d: %s\n", (int)e.line_info.first, e.msg.c_str());
        assert(c.errors().empty());
        if(!c.errors().empty()) return false;
    }

    {
        abc::assembler_t a{};
        auto e = a.assemble(c);
        assert(e.msg.empty());
        if(!e.msg.empty())
            return false;
        e = a.link();
        assert(e.msg.empty());
        if(!e.msg.empty())
            return false;
        binary = a.data();
    }

#if 1
    // test arduboy interpreter

    {
        std::ifstream vmhex(VMHEX_FILE);
        auto t = arduboy->load_file("vm.hex", vmhex);
        assert(t.empty());
        if(!t.empty())
            return false;
    }
    {
        std::istrstream ss((char const*)binary.data(), binary.size());
        auto t = arduboy->load_file("fxdata.bin", ss);
        assert(t.empty());
        if(!t.empty())
            return false;
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
    if(arduboy->cpu.data[0x0635] != 0)
        return false;
#endif

#if 1
    // test general interpreter

    {
        abc_host_t host{};

        host.user = &binary;
        host.prog = [](void* user, uint32_t addr) -> uint8_t {
            std::vector<uint8_t>& binary = *(std::vector<uint8_t>*)user;
            if(addr < binary.size())
                return binary[addr];
            return 0;
        };

        interp = {};

        int breaks = 0;

        for(;;)
        {
            auto r = abc_run(&interp, &host);
            if(r == ABC_RESULT_BREAK && ++breaks >= 2)
                break;
            if(r == ABC_RESULT_ERROR)
                return false;
        }
    }
#endif

    return true;
}

int abc_tests()
{
    int r = 0;
    namespace fs = std::filesystem;
    arduboy = std::make_unique<absim::arduboy_t>();

    printf("\nRunning tests...\n\n");

    for(auto const& entry : fs::recursive_directory_iterator(TESTS_DIR))
    {
        auto ext = entry.path().extension();
        if(entry.path().extension() != ".abc") continue;
        char const* status = "Pass";
        if(!test(entry.path().parent_path().generic_string(), entry.path().filename().generic_string()))
            status = "fail !!!", r = 1;
        printf("%-23s %s\n", entry.path().filename().generic_string().c_str(), status);
    }

    return r;
}
