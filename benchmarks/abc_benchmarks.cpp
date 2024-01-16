#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <ards_assembler.hpp>
#include <ards_compiler.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <sstream>
#include <string>
#include <strstream>

static std::unique_ptr<absim::arduboy_t> arduboy;

static FILE* fout;
static FILE* fmd;

static void out(char const* fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    vfprintf(fmd, fmt, v);
    va_end(v);
}

static void out_txt(char const* fmt, ...)
{
    va_list v;
    va_start(v, fmt);
    vprintf(fmt, v);
    va_end(v);
    va_start(v, fmt);
    vfprintf(fout, fmt, v);
    va_end(v);
}

static uint64_t measure()
{
    uint64_t cycle_a, cycle_b;

    // HACK: disable timer0 by setting its divider to 0
    arduboy->cpu.timer0.divider = 0;

    arduboy->allow_nonstep_breakpoints = true;
    arduboy->paused = false;
    arduboy->cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(10'000'000'000'000ull); // up to 10 seconds init
    assert(arduboy->paused);
    cycle_a = arduboy->cpu.cycle_count;
    arduboy->paused = false;
    arduboy->advance(10'000'000'000'000ull); // up to 10 seconds
    assert(arduboy->paused);
    cycle_b = arduboy->cpu.cycle_count;

    return cycle_b - cycle_a;
}

static void bench(char const* name)
{
    arduboy->reset();
    
    std::string abc_asm;
    std::vector<uint8_t> binary;

    {
        ards::compiler_t c{};
        std::ostringstream fo;
        c.compile(std::string(BENCHMARKS_DIR) + "/" + name, name, fo);
        for(auto const& e : c.errors())
        {
            printf("%s ERROR (line %d): %s\n",
                name,
                (int)e.line_info.first,
                e.msg.c_str());
        }
        if(!c.errors().empty())
            return;
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
        std::istrstream ss((char const*)VM_HEX_ARDUBOYFX, (int)VM_HEX_ARDUBOYFX_SIZE);
        auto t = arduboy->load_file("vm.hex", ss);
        assert(t.empty());
    }
    {
        std::istrstream ss((char const*)binary.data(), (int)binary.size());
        auto t = arduboy->load_file("fxdata.bin", ss);
        assert(t.empty());
    }

    uint64_t cycles_abc = measure();

    {
        std::string filename =
            std::string(BENCHMARKS_DIR) + "/" + name + "/" +
            name + ".ino-arduboy-fx.hex";
        std::ifstream fi(filename.c_str());
        assert(fi);
        auto t = arduboy->load_file("native.hex", fi);
        assert(t.empty());
    }

    uint64_t cycles_native = measure();

    double slowdown = double(cycles_abc) / cycles_native;
    out("<details><summary>%s: %.2fx slowdown",
        name, slowdown);
    if(slowdown < 1.0)
        out(" (%.2fx speedup)", 1.0 / slowdown);
    out("</summary>\n");
    out("<table>\n");
    out("<tr><th>Native</th><th>ABC</th></tr>\n");
    out("<tr><td>Cycles: %" PRIu64 "</td><td>Cycles: %" PRIu64 "</td></tr>\n",
        cycles_native, cycles_abc);
    out("<tr>\n");

    out("<td>\n\n```c\n");
    {
        std::string filename =
            std::string(BENCHMARKS_DIR) + "/" + name + "/" +
            name + ".ino";
        std::ifstream fi(filename.c_str());
        std::ostringstream ss;
        ss << fi.rdbuf();
        out("%s\n", ss.str().c_str());
    }
    out("```\n\n</td>\n");

    out("<td>\n\n```c\n");
    {
        std::string filename =
            std::string(BENCHMARKS_DIR) + "/" + name + "/" +
            name + ".abc";
        std::ifstream fi(filename.c_str());
        std::ostringstream ss;
        ss << fi.rdbuf();
        out("%s\n", ss.str().c_str());
    }
    out("```\n\n</td>\n");

    out("</tr>\n</table>\n</details>\n\n");

    out_txt("%-20s%12" PRIu64 "%12" PRIu64 "%12.2fx\n",
        name, cycles_native, cycles_abc,
        double(cycles_abc) / cycles_native);
}

int main()
{
    arduboy = std::make_unique<absim::arduboy_t>();

    fout = fopen(BENCHMARKS_DIR "/benchmarks.txt", "w");
    if(!fout) return 1;
    fmd = fopen(BENCHMARKS_DIR "/benchmarks.md", "w");
    if(!fmd)
    {
        fclose(fout);
        return 1;
    }

    printf("\nRunning benchmarks...\n\n");

    out("## Benchmarks\n\n");

    bench("bubble1");
    bench("bubble2");
    bench("bubble3");
    bench("bubble4");
    bench("fibonacci");
    bench("sieve");
    bench("text");
    bench("tilessprite");
    bench("tilessprite16");
    bench("tilesrect");

    fclose(fout);
    fclose(fmd);

    fout = fopen(BENCHMARKS_DIR "/latencies.txt", "w");
    if(!fout) return 1;
    {
        ards::assembler_t a{};
        {
            std::ifstream fi(BENCHMARKS_DIR "/instructions.asm");
            auto e = a.assemble(fi);
            assert(e.msg.empty());
            e = a.link();
            assert(e.msg.empty());
        }
        {
            std::istrstream ss((char const*)VM_HEX_ARDUBOYFX, (int)VM_HEX_ARDUBOYFX_SIZE);
            auto t = arduboy->load_file("vm.hex", ss);
            assert(t.empty());
        }
        {
            std::vector<uint8_t> binary = a.data();
            std::istrstream ss((char const*)binary.data(), (int)binary.size());
            auto t = arduboy->load_file("fxdata.bin", ss);
            assert(t.empty());
        }
    }
    static char const* const INSTRS[] =
    {
        "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7",
        "p8", "p16", "p32", "p64", "p128",
        "p00", "push N", "push2 N", "push3 N", "push4 N",
        "pzn 3", "pzn 4", "pzn 8", "pzn 16", "pzn 32",
        "dup", "dup2", "dup3", "dup4", "dup5", "dup6", "dup7", "dup8",
        "dupw", "dupw2", "dupw3", "dupw4", "dupw5", "dupw6", "dupw7", "dupw8",
        "getl N", "getl2 N", "getl4 N",
        "getln N (5)", "getln N (8)", "getln N (16)", "getln N (32)",
        "setl N", "setl2 N", "setl4 N",
        "setln N (5)", "setln N (8)", "setln N (16)", "setln N (32)",
        "getg VAR", "getg2 VAR", "getg4 VAR",
        "getgn VAR (5)", "getgn VAR (8)", "getgn VAR (16)", "getgn VAR (32)",
        "setg VAR", "setg2 VAR", "setg4 VAR",
        "setgn VAR (5)", "setgn VAR (8)", "setgn VAR (16)", "setgn VAR (32)",
        "getp",
        "getpn 2", "getpn 3", "getpn 4", "getpn 8", "getpn 16", "getpn 32",
    };
    (void)measure();
    uint64_t bn = measure();
    for(auto const* i : INSTRS)
    {
        uint64_t c = measure() - bn;
        fprintf(fout, "%-20s %3" PRIu64 "\n", i, c);
    }

    fclose(fout);

    return 0;
}
