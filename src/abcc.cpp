#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <cstring>

#include <argparse/argparse.hpp>

#define PROFILE_COUNT 1

#ifndef NDEBUG
#undef PROFILE_COUNT
#define PROFILE_COUNT 1
#endif

#define PROFILING (PROFILE_COUNT > 1)
#if PROFILING
#include <ctime>
#endif

void export_arduboy(
    std::string const& filename,
    std::vector<uint8_t> const& binary, bool has_save, bool mini,
    std::unordered_map<std::string, std::string> const& fd);
void export_interpreter_hex(
    std::string const& filename, bool mini);

static void usage(char const* argv0)
{
    //std::cout << "Current Path: " << std::filesystem::current_path() << std::endl;
    std::cout << "Usage: " << argv0 << " <main.abc> [fxdata.bin] [-a game.arduboy]" << std::endl;
}

int main(int argc, char** argv)
{
    std::filesystem::path psrc;
    std::filesystem::path pbin;
    std::filesystem::path pdata;
    std::filesystem::path psave;
    std::filesystem::path parduboy;
    std::filesystem::path pasm;
    std::filesystem::path pinterp;

    argparse::ArgumentParser args("abcc", ABC_VERSION);
    args.add_argument("<main.abc>")
        .help("path to top-level ABC source file")
        .action([&](std::string const& v) { psrc = v; });
    args.add_argument("-m", "--mini")
        .help("build for Arduboy Mini")
        .flag();
    args.add_argument("-i", "--interp")
        .help("path to interpreter .hex output file")
        .metavar("PATH")
        .action([&](std::string const& v) { pinterp = v; });
    args.add_argument("-b", "--bin")
        .help("path to development FX data output file")
        .metavar("PATH")
        .action([&](std::string const& v) { pbin = v; });
    args.add_argument("-d", "--data")
        .help("path to FX data output file")
        .metavar("PATH")
        .action([&](std::string const& v) { pdata = v; });
    args.add_argument("-s", "--save")
        .help("path to FX save output file (if there are no saved variables, no file is written)")
        .metavar("PATH")
        .action([&](std::string const& v) { psave = v; });
    args.add_argument("-a", "--arduboy")
        .help("path to .arduboy output file")
        .metavar("PATH")
        .action([&](std::string const& v) { parduboy = v; });
    args.add_argument("-S", "--asm")
        .help("path to .asm output file")
        .metavar("PATH")
        .action([&](std::string const& v) { pasm = v; });

    try {
        args.parse_args(argc, argv);
    }
    catch(const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << args;
#ifdef NDEBUG
        std::exit(1);
#endif
    }

    if(!psrc.empty() &&
        pbin.empty() &&
        pdata.empty() &&
        psave.empty() &&
        parduboy.empty() &&
        pasm.empty() &&
        pinterp.empty())
        parduboy = psrc.parent_path() / "game.arduboy";

    bool show_asm = false;
#ifndef NDEBUG
    show_asm = true;
#endif

#if PROFILING
    psrc = "C:/Users/Brown/Documents/GitHub/abc/examples/platformer/main.abc";
#endif

#ifndef NDEBUG
    psrc = "C:/Users/Brown/Documents/GitHub/abc/examples/test/main.abc";
    //psrc = "C:/Users/Brown/Documents/GitHub/abc/examples/platformer/main.abc";
    //psrc = "C:/Users/Brown/Documents/GitHub/abc/benchmarks/bubble3/bubble3.abc";
    //psrc = "C:/Users/Brown/Documents/GitHub/abc/tests/tests/memcpy.abc";
    //pbin = "C:/Users/Brown/Documents/GitHub/abc/examples/test/blah.bin";
#endif

    if(psrc.empty())
    {
        usage(argv[0]);
        return 1;
    }
    if(pbin.empty() && parduboy.empty())
        show_asm = true;

    psrc = std::filesystem::current_path() / psrc;

    ards::compiler_t c;
    ards::assembler_t a;

#if PROFILING
    auto ta = clock();

    for(int prof = 0; prof < PROFILE_COUNT; ++prof)
    {
#endif

    c = {};
    a = {};

    std::stringstream fasm;

    c.compile(psrc.parent_path().generic_string(), psrc.stem().generic_string(), fasm);
    for(auto const& e : c.errors())
    {
        //std::cerr << "Compiler Error" << std::endl;
        //std::cerr << /* argv[2] << ":" << */ e.line_info.first << ":" << e.line_info.second << std::endl;
        //std::cerr << e.msg << std::endl;
        std::cerr
            << c.error_file()
            << ":"
            << e.line_info.first
            << ":"
            << e.line_info.second
            << ": error: "
            << e.msg
            << std::endl;
    }
    if(!c.errors().empty())
        return 1;

#if !PROFILING
    if(show_asm)
        std::cout << fasm.str() << std::endl;
#endif

    if(!pasm.empty())
    {
        std::ofstream f(pasm);
        if(!f)
        {
            std::cerr << "Unable to open file: \"" << pasm.generic_string() << "\"" << std::endl;
            return 1;
        }
        f << fasm.str();
    }

    {
        auto e = a.assemble(fasm);
        if(!e.msg.empty())
        {
            std::cerr << "Assembler Error" << std::endl;
            std::cerr << e.msg << std::endl;
            return 1;
        }
    }

    {
        auto e = a.link();
        if(!e.msg.empty())
        {
            std::cerr << "Linker Error" << std::endl;
            std::cerr << e.msg << std::endl;
            return 1;
        }
    }

#if PROFILING
    }

    auto tb = clock();

    printf("Time: %f\n", double(tb - ta) / CLOCKS_PER_SEC);
#endif

    printf("Compilation Succeeded!\n");
    printf("======================\n");
    printf("Dev Bin: %7d bytes  %6.1f KB\n", (int)a.data().size(), (double)a.data().size() / 1024);
    printf("======================\n");
    printf("Data:    %7d bytes  %6.1f KB\n", (int)a.data_size(), (double)a.data_size() / 1024);
    printf("Code:    %7d bytes  %6.1f KB\n", (int)a.code_size(), (double)a.code_size() / 1024);
    printf("Debug:   %7d bytes  %6.1f KB\n", (int)a.debug_size(), (double)a.debug_size() / 1024);
    printf("======================\n");
    printf("Globals: %7d bytes\n", (int)a.globals_size());
    printf("Save:    %7d bytes\n", (int)a.save_size());
    printf("======================\n");

    if(!pbin.empty())
    {
        std::ofstream fbin(pbin.generic_string(), std::ios::out | std::ios::binary);
        if(!fbin)
        {
            std::cerr << "Unable to open file: \"" << pbin.generic_string() << "\"" << std::endl;
            return 1;
        }
        fbin.write((char const*)a.data().data(), a.data().size());
    }

    if(!pdata.empty())
    {
        std::ofstream f(pdata.generic_string(), std::ios::out | std::ios::binary);
        if(!f)
        {
            std::cerr << "Unable to open file: \"" << pdata.generic_string() << "\"" << std::endl;
            return 1;
        }
        size_t save_bytes = a.has_save() ? 4096 : 0;
        f.write((char const*)a.data().data(), a.data().size() - save_bytes);
    }

    if(!psave.empty() && a.has_save())
    {
        std::ofstream f(psave.generic_string(), std::ios::out | std::ios::binary);
        if(!f)
        {
            std::cerr << "Unable to open file: \"" << psave.generic_string() << "\"" << std::endl;
            return 1;
        }
        f.write((char const*)a.data().data() + a.data().size() - 4096, 4096);
    }

    if(!pinterp.empty())
    {
        export_interpreter_hex(pinterp.generic_string(), args["--mini"] == true);
    }

    if(!parduboy.empty())
    {
        export_arduboy(
            parduboy.generic_string(),
            a.data(), a.has_save(), args["--mini"] == true,
            c.arduboy_directives());
    }

    return 0;
}
