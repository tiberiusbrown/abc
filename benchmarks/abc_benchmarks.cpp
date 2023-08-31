#include <absim.hpp>
#include <ards_assembler.hpp>
#include <ards_compiler.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <strstream>

static std::unique_ptr<absim::arduboy_t> arduboy;

static void bench(char const* name)
{
    arduboy->reset();
    
    std::string asm_out;
    std::vector<uint8_t> binary;

    {
        ards::compiler_t c{};
        std::string filename =
            std::string(BENCHMARKS_DIR) + "/" + name + "/" +
            name + ".abc";
        std::ifstream fi(filename.c_str());
        std::ostringstream fo;
        assert(fi);
        c.compile(fi, fo);
        assert(c.errors().empty());
        asm_out = fo.str();
    }

    {
        ards::assembler_t a{};
        std::istrstream ss(asm_out.data(), asm_out.size());
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

    uint64_t cycle_a, cycle_b, cycles_abc, cycles_native;

    arduboy->cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(1'000'000'000'000ull); // up to 1 second init
    assert(arduboy->paused);
    cycle_a = arduboy->cpu.cycle_count;
    arduboy->paused = false;
    arduboy->advance(60'000'000'000'000ull); // up to 60 seconds
    assert(arduboy->paused);
    cycle_b = arduboy->cpu.cycle_count;

    cycles_abc = cycle_b - cycle_a;

    {
        std::string filename =
            std::string(BENCHMARKS_DIR) + "/" + name + "/" +
            name + ".ino-arduboy-fx.hex";
        std::ifstream fi(filename.c_str());
        assert(fi);
        auto t = arduboy->load_file("native.hex", fi);
        assert(t.empty());
    }

    arduboy->cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(1'000'000'000'000ull); // up to 1 second init
    assert(arduboy->paused);
    cycle_a = arduboy->cpu.cycle_count;
    arduboy->paused = false;
    arduboy->advance(60'000'000'000'000ull); // up to 60 seconds
    assert(arduboy->paused);
    cycle_b = arduboy->cpu.cycle_count;

    cycles_native = cycle_b - cycle_a;

    printf("%-20s%12" PRIu64 "%12" PRIu64 "%12.2fx\n",
        name, cycles_native, cycles_abc,
        double(cycles_abc) / cycles_native);
}

int main()
{
    arduboy = std::make_unique<absim::arduboy_t>();

    //bench("bubble");
    bench("fibonacci");

    return 0;
}
