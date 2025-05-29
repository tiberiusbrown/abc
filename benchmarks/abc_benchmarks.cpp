#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include <absim.hpp>
#include <ards_assembler.hpp>
#include <ards_compiler.hpp>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <strstream>
#include <tuple>

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

static uint64_t measure(bool abc = false)
{
    uint64_t cycle_a, cycle_b;

    // HACK: disable all interrupts
    arduboy->cpu.sreg() &= ~absim::SREG_I;
    arduboy->cpu.data[0x6e] = 0;
    arduboy->cpu.data[0x6f] = 0;
    arduboy->cpu.data[0x71] = 0;
    arduboy->cpu.data[0x72] = 0;

    arduboy->allow_nonstep_breakpoints = true;
    arduboy->paused = false;
    arduboy->cpu.enabled_autobreaks.set(absim::AB_BREAK);
    arduboy->advance(10'000'000'000'000ull); // up to 10 seconds init

    arduboy->cpu.sreg() &= ~absim::SREG_I;
    arduboy->cpu.data[0x6e] = 0;
    arduboy->cpu.data[0x6f] = 0;
    arduboy->cpu.data[0x71] = 0;
    arduboy->cpu.data[0x72] = 0;

    if(abc && arduboy->cpu.data[0x0635] != 0)
        return 0;
    assert(arduboy->paused);
    cycle_a = arduboy->cpu.cycle_count;
    arduboy->paused = false;
    arduboy->advance(10'000'000'000'000ull); // up to 10 seconds
    assert(arduboy->paused);
    cycle_b = arduboy->cpu.cycle_count;

    if(abc && arduboy->cpu.data[0x0635] != 0)
        return 0;
    return cycle_b - cycle_a;
}

static std::vector<uint8_t> compile(std::string const& fname)
{
    std::string abc_asm;
    ards::compiler_t c{};
    std::ostringstream fo;
    std::filesystem::path p(fname);
    auto path = p.parent_path().lexically_normal().generic_string();
    auto name = p.stem().generic_string();
    c.suppress_githash();
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
    {
        std::ofstream fasm(path + "/asm.txt");
        fasm << abc_asm;
    }

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
        std::ifstream vmhex(VMHEX_FILE);
        auto t = arduboy->load_file("vm.hex", vmhex);
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
    bench("mat3rotation");
    bench("fibonacci");
    bench("format");
    bench("sieve");
    bench("text");
    bench("tilessprite");
    bench("tilessprite16");
    bench("tilesrect");

    fclose(fout);
    fclose(fmd);
#endif

#if 1
    if(compile(PLATFORMER_DIR "/../basic/main.abc"  ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../circle/main.abc" ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../font/main.abc"   ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../gray/main.abc"   ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../midi/main.abc"   ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../pong/main.abc"   ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../snake/main.abc"  ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../sprite/main.abc" ).empty()) return 1;
    if(compile(PLATFORMER_DIR "/../tilemap/main.abc").empty()) return 1;
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
            std::ifstream vmhex(VMHEX_FILE);
            auto t = arduboy->load_file("vm.hex", vmhex);
            assert(t.empty());
        }
        {
            std::istrstream ss((char const*)binary.data(), (int)binary.size());
            auto t = arduboy->load_file("fxdata.bin", ss);
            assert(t.empty());
        }
        (void)measure();
        out_txt("    %" PRIu64 "\n", measure());
    }
    fclose(fout);
#endif

    fout = fopen(BENCHMARKS_DIR "/cycles_instruction.txt", "w");
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
            std::ifstream vmhex(VMHEX_FILE);
            auto t = arduboy->load_file("vm.hex", vmhex);
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
        "nop",
        "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7",
        "p8", "p16", "p32", "p64", "p128",
        "p00", "p000", "p0000",
        "pz8", "pz16",
        "push N", "push2 N", "push3 N", "push4 N",
        "sext", "sext (neg)", "sext2", "sext2 (neg)", "sext3", "sext3 (neg)",
        "dup", "dup2", "dup3", "dup4", "dup5", "dup6", "dup7", "dup8",
        "dupw", "dupw2", "dupw3", "dupw4", "dupw5", "dupw6", "dupw7", "dupw8",
        "getl N", "getl2 N", "getl4 N",
        "getln N (3)", "getln N (5)", "getln N (8)", "getln N (16)", "getln N (32)",
        "setl N", "setl2 N", "setl4 N",
        "setln N (3)", "setln N (5)", "setln N (8)", "setln N (16)", "setln N (32)",
        "getg VAR", "getg2 VAR", "getg4 VAR",
        "getgn VAR (3)", "getgn VAR (5)", "getgn VAR (8)", "getgn VAR (16)", "getgn VAR (32)",
        "gtgb VAR", "gtgb2 VAR", "gtgb4 VAR",
        "setg VAR", "setg2 VAR", "setg4 VAR",
        "setgn VAR (3)", "setgn VAR (5)", "setgn VAR (8)", "setgn VAR (16)", "setgn VAR (32)",
        "getp",
        "getpn 2", "getpn 3", "getpn 4", "getpn 8", "getpn 16", "getpn 32",
        "pop", "pop2", "pop3", "pop4", "popn N",
        "refl", "refgb",
        "getr", "getr2", "getrn 3", "getrn 4", "getrn 8", "getrn 16", "getrn 32",
        "setr", "setr2", "setrn 3", "setrn 4", "setrn 8", "setrn 16", "setrn 32",
        "aixb1", "aidxb", "aidx",
        "pidxb", "pidx",
        "uaidx", "upidx",
        "aslc", "pslc",
        "inc", "dec", "linc N",
        "pinc", "pinc2", "pinc3", "pinc4",
        "pdec", "pdec2", "pdec3", "pdec4",
        "pincf (0)", "pincf (1)", "pincf (1000000)",
        "pdecf (0)", "pdecf (1)", "pdecf (1000000)",
        "add", "add2", "add3", "add4",
        "sub", "sub2", "sub3", "sub4",
        "add2b", "sub2b", "mul2b", "add3b",
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
        "lsl4 (0)", "lsl4 (1)", "lsl4 (4)", "lsl4 (8)", "lsl4 (16)", "lsl4 (24)", "lsl4 (31)", "lsl4 (32)", "lsl4 (64)",
        "lsr (0)", "lsr (1)", "lsr (4)", "lsr (7)", "lsr (8)", "lsr (64)",
        "lsr2 (0)", "lsr2 (1)", "lsr2 (4)", "lsr2 (8)", "lsr2 (15)", "lsr2 (16)", "lsr2 (64)",
        "lsr4 (0)", "lsr4 (1)", "lsr4 (4)", "lsr4 (8)", "lsr4 (16)", "lsr4 (24)", "lsr4 (31)", "lsr4 (32)", "lsr4 (64)",
        "asr (0)", "asr (1)", "asr (4)", "asr (7)", "asr (8)", "asr (64)",
        "asr2 (0)", "asr2 (1)", "asr2 (4)", "asr2 (8)", "asr2 (15)", "asr2 (16)", "asr2 (64)",
        "asr4 (0)", "asr4 (1)", "asr4 (4)", "asr4 (8)", "asr4 (16)", "asr4 (24)", "asr4 (31)", "asr4 (32)", "asr4 (64)",
        "and", "and2", "and4",
        "or", "or2", "or4",
        "xor", "xor2", "xor4",
        "comp", "comp2", "comp4",
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
        "cfeq (3.5, 3.4)", "cfeq (3.5, 3.4e8)",
        "cflt (3.5, 3.4)", "cflt (3.5, 3.4e8)",
        "not (0)", "not (1)",
        "fadd (3.5, 3.4)", "fadd (3.5, 3.4e8)",
        "fsub (3.5, -3.4)", "fsub (3.5, -3.4e8)",
        "fmul (3.5, 3.4)", "fmul (3.5, 3.4e8)",
        "fdiv (3.5, 3.4)", "fdiv (3.5, 3.4e8)",
        "f2i (3.4)", "f2i (3.4e8)",
        "f2u (3.4)", "f2u (3.4e8)",
        "i2f (3)", "i2f (340000000)",
        "u2f (3)", "u2f (340000000)",
        "bz (not taken)", "bz (taken)",
        "bz1 (not taken)", "bz1 (taken)",
        "bnz (not taken)", "bnz (taken)",
        "bnz1 (not taken)", "bnz1 (taken)",
        "bzp (not taken)", "bzp (taken)",
        "bzp1 (not taken)", "bzp1 (taken)",
        "bnzp (not taken)", "bnzp (taken)",
        "bnzp1 (not taken)", "bnzp1 (taken)",
        "jmp", "jmp1",
        "call", "call1", "icall", "ret",
        "$get_pixel",
        "$draw_pixel",
        "$draw_hline (0, 0, 1)",
        "$draw_hline (0, 0, 128)",
        "$draw_vline (0, 0, 1)",
        "$draw_vline (0, 0, 64)",
        "$draw_sprite (1x8 unmasked)",
        "$draw_sprite (8x8 unmasked)",
        "$draw_sprite (8x8 masked)",
        "$draw_sprite (16x16 unmasked)",
        "$draw_sprite (16x16 masked)",
        "$draw_sprite (32x32 unmasked)",
        "$draw_sprite (32x32 masked)",
        "$draw_sprite (x=0, y=+1, 32x32 unmasked)",
        "$draw_sprite (x=0, y=+1, 32x32 masked)",
        "$draw_sprite (x=0, y=-1, 32x32 unmasked)",
        "$draw_sprite (x=0, y=-1, 32x32 masked)",
        "$draw_sprite (x=0, y=33, 32x32 unmasked)",
        "$draw_sprite (x=0, y=33, 32x32 masked)",
        "$draw_sprite (x=-1, y=0, 32x32 unmasked)",
        "$draw_sprite (x=-1, y=0, 32x32 masked)",
        "$draw_sprite (128x64 unmasked)",
        "$draw_sprite (128x64 masked)",
        "$draw_filled_rect (0, 0, 4, 4)",
        "$draw_filled_rect (0, 6, 4, 4)",
        "$draw_filled_rect (0, 0, 8, 8)",
        "$draw_filled_rect (0, 4, 8, 8)",
        "$draw_filled_rect (0, 0, 16, 16)",
        "$draw_filled_rect (0, 4, 16, 16)",
        "$draw_filled_rect (0, 0, 32, 32)",
        "$draw_filled_rect (0, 4, 32, 32)",
        "$draw_filled_rect (0, 0, 64, 64)",
        "$draw_filled_rect (0, 4, 64, 64)",
        "$draw_filled_rect (0, 0, 128, 64)",
        "$draw_filled_circle (64, 32, 8)",
        "$draw_filled_circle (64, 32, 16)",
        "$draw_filled_circle (64, 32, 32)",
        "$draw_filled_circle (64, 32, 64)",
        "$draw_circle (64, 32, 8)",
        "$draw_circle (64, 32, 16)",
        "$draw_circle (64, 32, 32)",
        "$draw_circle (64, 32, 64)",
    };
    (void)measure();
    uint64_t bn = measure();
    {
        std::vector<std::tuple<char const*, uint64_t>> timings;
        for(auto const* i : INSTRS)
            timings.push_back({ i, uint64_t(measure() - bn) });
        for(auto const& t : timings)
            fprintf(fout, "%5" PRIu64 "   %s\n", std::get<1>(t), std::get<0>(t));
    }

    fclose(fout);

    fout = fopen(BENCHMARKS_DIR "/cycles_code.txt", "w");
    if(!fout) return 1;
    {
        auto d = compile(BENCHMARKS_DIR "/_cycles_code/main.abc");

        {
            std::ifstream vmhex(VMHEX_FILE);
            auto t = arduboy->load_file("vm.hex", vmhex);
            assert(t.empty());
        }
        {
            std::istrstream ss((char const*)d.data(), (int)d.size());
            auto t = arduboy->load_file("fxdata.bin", ss);
            assert(t.empty());
        }

        std::vector<uint64_t> timings;
        for(;;)
        {
            uint64_t t = measure(true);
            if(t == 0)
                break;
            t -= bn;
            timings.push_back(t);
        }

        std::vector<std::string> lines;
        {
            std::istrstream ss(
                (char const*)arduboy->cpu.serial_bytes.data(),
                (int)arduboy->cpu.serial_bytes.size());
            std::string line;
            while(std::getline(ss, line))
                lines.push_back(line);
        }
        assert(lines.size() == timings.size());

        for(size_t i = 0; i < std::min(lines.size(), timings.size()); ++i)
            fprintf(fout, "%7" PRIu64 "    %s\n", timings[i], lines[i].c_str());
    }
    fclose(fout);

    return 0;
}
