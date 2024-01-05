#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <cstring>

#define PROFILE_COUNT 1
#define PROFILING (PROFILE_COUNT > 1)

#if PROFILING
#include <ctime>
#endif

void export_arduboy(
    std::string const& filename,
    std::vector<uint8_t> const& binary, bool has_save);

static void usage(char const* argv0)
{
    //std::cout << "Current Path: " << std::filesystem::current_path() << std::endl;
    std::cout << "Usage: " << argv0 << " <main.abc> [fxdata.bin] [-a game.arduboy]" << std::endl;
}

int main(int argc, char** argv)
{
    std::filesystem::path psrc;
    std::filesystem::path pbin;
    std::filesystem::path parduboy;

    for(int i = 1; i < argc; ++i)
    {
        bool more = (i + 1 < argc);
        if(more && !strcmp(argv[i], "-a"))
            parduboy = argv[++i];
        else if(!strcmp(argv[i], "-h"))
            usage(argv[0]);
        else if(psrc.empty())
            psrc = argv[i];
        else if(pbin.empty())
            pbin = argv[i];
        else
        {
            usage(argv[0]);
            return 1;
        }
    }

    bool show_asm = false;
#ifndef NDEBUG
    show_asm = true;
#endif

#ifndef NDEBUG
    //psrc = "C:/Users/Brown/Documents/GitHub/abc/examples/test/main.abc";
    //psrc = "C:/Users/Brown/Documents/GitHub/abc/examples/platformer/main.abc";
    //psrc = "C:/Users/Brown/Documents/GitHub/abc/benchmarks/bubble3/bubble3.abc";
    psrc = "C:/Users/Brown/Documents/GitHub/abc/tests/tests/struct_ref_bug.abc";
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

    std::stringstream fasm;

#if PROFILING
    auto ta = clock();

    for(int prof = 0; prof < PROFILE_COUNT; ++prof)
    {
#endif

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

    printf("Compilation succeeded!\n");
    printf("======================\n");
    printf("Data:    %7d bytes\n", (int)a.data_size());
    printf("Code:    %7d bytes\n", (int)a.code_size());
    printf("Debug:   %7d bytes\n", (int)a.debug_size());
    printf("Globals: %7d bytes\n", (int)a.globals_size());
    printf("Save:    %7d bytes\n", (int)a.save_size());
    printf("======================\n");

    if(!pbin.empty())
    {
        auto fxdata = pbin;
        std::ofstream fbin(fxdata.generic_string(), std::ios::out | std::ios::binary);
        if(!fbin)
        {
            std::cerr << "Unable to open file: \"" << fxdata.generic_string() << "\"" << std::endl;
            return 1;
        }
        fbin.write((char const*)a.data().data(), a.data().size());
    }

    if(!parduboy.empty())
    {
        export_arduboy(parduboy.generic_string(), a.data(), a.has_save());
    }

    return 0;
}
