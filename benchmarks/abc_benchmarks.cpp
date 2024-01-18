#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <ards_assembler.hpp>
#include <ards_compiler.hpp>
#include <vm_hex_arduboyfx.hpp>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <filesystem>
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

    // HACK: disable all interrupts
    arduboy->cpu.sreg() &= ~absim::SREG_I;

    arduboy->allow_nonstep_breakpoints = true;
    arduboy->paused = false;
    arduboy->cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(10'000'000'000'000ull); // up to 10 seconds init
    arduboy->cpu.sreg() &= ~absim::SREG_I;
    assert(arduboy->paused);
    cycle_a = arduboy->cpu.cycle_count;
    arduboy->paused = false;
    arduboy->advance(10'000'000'000'000ull); // up to 10 seconds
    assert(arduboy->paused);
    cycle_b = arduboy->cpu.cycle_count;

    return cycle_b - cycle_a;
}

static std::vector<uint8_t> compile(std::string const& fname)
{
    std::string abc_asm;
    ards::compiler_t c{};
    std::ostringstream fo;
    std::filesystem::path p(fname);
    auto path = p.parent_path().generic_string();
    auto name = p.stem().generic_string();
    c.compile(path, name, fo);
    for(auto const& e : c.errors())
    {
        printf("%s ERROR (line %d): %s\n",
            name.c_str(),
            (int)e.line_info.first,
            e.msg.c_str());
    }
    if(!c.errors().empty())
        return {};
    abc_asm = fo.str();

    ards::assembler_t a{};
    std::istrstream ss(abc_asm.data(), (int)abc_asm.size());
    auto e = a.assemble(ss);
    assert(e.msg.empty());
    e = a.link();
    assert(e.msg.empty());
    return a.data();
}

static void bench(char const* name)
{
    arduboy->reset();
    
    std::string abc_asm;
    std::vector<uint8_t> binary;

    binary = compile(std::string(BENCHMARKS_DIR) + "/" + name + "/" + name + ".abc");

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

int abc_benchmarks()
{
    arduboy = std::make_unique<absim::arduboy_t>();

#if 1
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
#endif

#if 1
    fout = fopen(PLATFORMER_DIR "/benchmark.txt", "w");
    if(!fout) return 1;
    {
        printf("\n");
        out_txt("Platformer benchmark...\n");
        auto binary = compile(PLATFORMER_DIR "/benchmark.abc");
        assert(!binary.empty());
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
        constexpr int N = 10;
        uint64_t total = 0;
        for(int i = 0; i < N; ++i)
        {
            uint64_t cycles = measure();
            total += cycles;
            out_txt("    %" PRIu64 "\n", cycles);
        }
        out_txt("    %.0f  (average)\n", double(total) / N);
    }
    fclose(fout);
#endif

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
#if 0
        {
            FILE* fbin = fopen(BENCHMARKS_DIR "/instructions.bin", "wb");
            fwrite(a.data().data(), 1, a.data().size(), fbin);
            fclose(fbin);
        }
#endif
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
        "sext", "sext (neg)", "set2", "sext2 (neg)", "sext3", "sext3 (neg)",
        "dup", "dup2", "dup3", "dup4", "dup5", "dup6", "dup7", "dup8",
        "dupw", "dupw2", "dupw3", "dupw4", "dupw5", "dupw6", "dupw7", "dupw8",
        "getl N", "getl2 N", "getl4 N",
        "getln N (5)", "getln N (8)", "getln N (16)", "getln N (32)",
        "setl N", "setl2 N", "setl4 N",
        "setln N (5)", "setln N (8)", "setln N (16)", "setln N (32)",
        "getg VAR", "getg2 VAR", "getg4 VAR",
        "getgn VAR (3)", "getgn VAR (5)", "getgn VAR (8)", "getgn VAR (16)", "getgn VAR (32)",
        "setg VAR", "setg2 VAR", "setg4 VAR",
        "setgn VAR (3)", "setgn VAR (5)", "setgn VAR (8)", "setgn VAR (16)", "setgn VAR (32)",
        "getp",
        "getpn 2", "getpn 3", "getpn 4", "getpn 8", "getpn 16", "getpn 32",
        "pop", "pop2", "pop3", "pop4", "popn N",
        "refl", "refgb", "refg",
        "getr", "getr2", "getrn 3", "getrn 4", "getrn 8", "getrn 16", "getrn 32",
        "setr", "setr2", "setrn 3", "setrn 4", "setrn 8", "setrn 16", "setrn 32",
        "aixb1", "aidxb", "aidx",
        "pidxb", "pidx",
        "uaidx", "upidx",
        "inc", "dec", "linc N",
        "pinc", "pinc2", "pinc3", "pinc4",
        "pdec", "pdec2", "pdec3", "pdec4",
        "pincf (0)", "pincf (1)", "pincf (1000000)",
        "pdecf (0)", "pdecf (1)", "pdecf (1000000)",
        "add", "add2", "add3", "add4",
        "sub", "sub2", "sub3", "sub4",
        "add2b",
        "mul", "mul2", "mul3", "mul4",
        "udiv2 (30000 / 7)", "udiv2 (30000 / 7000)",
        "udiv4 (300000000 / 7)", "udiv4 (300000000 / 70000000)",
        "div2 (30000 / 7)", "div2 (30000 / 7000)",
        "div4 (300000000 / 7)", "div4 (300000000 / 70000000)",
        "umod2 (30000 / 7)", "umod2 (30000 / 7000)",
        "umod4 (300000000 / 7)", "umod4 (300000000 / 70000000)",
        "mod2 (30000 / 7)", "mod2 (30000 / 7000)",
        "mod4 (300000000 / 7)", "mod4 (300000000 / 70000000)",
        "lsl (0)", "lsl (1)", "lsl (4)", "lsl (7)", "lsl (8)", "lsl (64)",
        "lsl2 (0)", "lsl2 (1)", "lsl2 (4)", "lsl2 (8)", "lsl2 (15)", "lsl2 (16)", "lsl2 (64)",
        "lsl3 (0)", "lsl3 (1)", "lsl3 (4)", "lsl3 (8)", "lsl3 (16)", "lsl3 (23)", "lsl3 (24)", "lsl3 (64)",
        "lsl4 (0)", "lsl4 (1)", "lsl4 (4)", "lsl4 (8)", "lsl4 (16)", "lsl4 (24)", "lsl4 (31)", "lsl4 (32)", "lsl4 (64)",
        "lsr (0)", "lsr (1)", "lsr (4)", "lsr (7)", "lsr (8)", "lsr (64)",
        "lsr2 (0)", "lsr2 (1)", "lsr2 (4)", "lsr2 (8)", "lsr2 (15)", "lsr2 (16)", "lsr2 (64)",
        "lsr3 (0)", "lsr3 (1)", "lsr3 (4)", "lsr3 (8)", "lsr3 (16)", "lsr3 (23)", "lsr3 (24)", "lsr3 (64)",
        "lsr4 (0)", "lsr4 (1)", "lsr4 (4)", "lsr4 (8)", "lsr4 (16)", "lsr4 (24)", "lsr4 (31)", "lsr4 (32)", "lsr4 (64)",
        "asr (0)", "asr (1)", "asr (4)", "asr (7)", "asr (8)", "asr (64)",
        "asr2 (0)", "asr2 (1)", "asr2 (4)", "asr2 (8)", "asr2 (15)", "asr2 (16)", "asr2 (64)",
        "asr3 (0)", "asr3 (1)", "asr3 (4)", "asr3 (8)", "asr3 (16)", "asr3 (23)", "asr3 (24)", "asr3 (64)",
        "asr4 (0)", "asr4 (1)", "asr4 (4)", "asr4 (8)", "asr4 (16)", "asr4 (24)", "asr4 (31)", "asr4 (32)", "asr4 (64)",
        "and", "and2", "and3", "and4",
        "or", "or2", "or3", "or4",
        "xor", "xor2", "xor3", "xor4",
        "comp", "comp2", "comp3", "comp4",
        "bool (0)", "bool (1)",
        "bool2 (0)", "bool2 (1)",
        "bool3 (0)", "bool3 (1)",
        "bool4 (0)", "bool4 (1)",
        "cult (pass)", "cult (fail)",
        "cult2 (pass)", "cult2 (fail)",
        "cult3 (pass)", "cult3 (fail)",
        "cult4 (pass)", "cult4 (fail)",
        "cslt (pass)", "cslt (fail)",
        "cslt2 (pass)", "cslt2 (fail)",
        "cslt3 (pass)", "cslt3 (fail)",
        "cslt4 (pass)", "cslt4 (fail)",
        "cule (pass)", "cule (fail)",
        "cule2 (pass)", "cule2 (fail)",
        "cule3 (pass)", "cule3 (fail)",
        "cule4 (pass)", "cule4 (fail)",
        "csle (pass)", "csle (fail)",
        "csle2 (pass)", "csle2 (fail)",
        "csle3 (pass)", "csle3 (fail)",
        "csle4 (pass)", "csle4 (fail)",
        // TODO: CFEQ
        // TODO: CFLT
        // TODO: CFLE
        "not (0)", "not (1)",
        // TODO: FADD
        // TODO: FSUB
        // TODO: FMUL
        // TODO: FDIV
        // TODO: F2I
        // TODO: F2U
        // TODO: I2F
        // TODO: U2F
        "bz (not taken)", "bz (taken)",
        "bz1 (not taken)", "bz1 (taken)",
        "bnz (not taken)", "bnz (taken)",
        "bnz1 (not taken)", "bnz1 (taken)",
        "bzp (not taken)", "bzp (taken)",
        "bzp1 (not taken)", "bzp1 (taken)",
        "bnzp (not taken)", "bnzp (taken)",
        "bnzp1 (not taken)", "bnzp1 (taken)",
        "jmp", "jmp1",
        "call", "call1", "ret",
    };
    (void)measure();
    uint64_t bn = measure();
    for(auto const* i : INSTRS)
    {
        uint64_t c = measure() - bn;
        fprintf(fout, "%3" PRIu64 "   %s\n", c, i);
    }

    fclose(fout);

    return 0;
}
