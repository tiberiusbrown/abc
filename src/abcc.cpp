#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#define PROFILE_COUNT 1
#define PROFILING (PROFILE_COUNT > 1)

#if PROFILING
#include <ctime>
#endif

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    ards::compiler_t c;
    ards::assembler_t a;

    std::stringstream fasm;

    std::filesystem::path psrc = "C:/Users/Brown/Documents/GitHub/abc/examples/test/main.abc";
    //std::filesystem::path psrc = "C:/Users/Brown/Documents/GitHub/abc/tests/tests/save_load.abc";

#if PROFILING
    auto ta = clock();

    for(int prof = 0; prof < PROFILE_COUNT; ++prof)
    {
#endif

    c.compile(psrc.parent_path().generic_string(), psrc.stem().generic_string(), fasm);
    for(auto const& e : c.errors())
    {
        std::cerr << "Compiler Error" << std::endl;
        std::cerr << /* argv[2] << ":" << */ e.line_info.first << ":" << e.line_info.second << std::endl;
        std::cerr << e.msg << std::endl;
    }
    if(!c.errors().empty())
        return 1;

#if !PROFILING
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

    {
        auto fxdata = psrc.parent_path() / "fxdata.bin";
        std::ofstream fbin(fxdata.generic_string(), std::ios::out | std::ios::binary);
        if(!fbin)
        {
            std::cerr << "Unable to open file: \"" << fxdata.generic_string() << "\"" << std::endl;
            return 1;
        }
        fbin.write((char const*)a.data().data(), a.data().size());
    }

    return 0;
}
