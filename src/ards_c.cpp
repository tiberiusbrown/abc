#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{

    ards::compiler_t c;
    ards::assembler_t a;

    std::string si = R"(
i16[100] A;
i16 total;

void swap(i16& a, i16& b)
{
    i16 t = a;
    a = b;
    b = t;
}

void main()
{
    for(u8 i = 0; i < 100; i = i + 1)
        A[i] = i + 1;

    $debug_break();
    
    for(u8 i = 0; i < 100; i = i + 1)
        total = total + A[i];

    $debug_break();
}
)";

    std::istringstream fi(si);
    std::stringstream fasm;

    c.compile(fi, fasm);
    for(auto const& e : c.errors())
    {
        std::cerr << "Compiler Error" << std::endl;
        std::cerr << /* argv[2] << ":" << */ e.line_info.first << ":" << e.line_info.second << std::endl;
        std::cerr << e.msg << std::endl;
    }
    if(!c.errors().empty())
        return 1;

    std::cout << fasm.str() << std::endl;

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

    if(argc > 1)
    {
        std::ofstream fbin(argv[1], std::ios::out | std::ios::binary);
        if(!fbin)
        {
            std::cerr << "Unable to open file: \"" << argv[1] << "\"" << std::endl;
            return 1;
        }
        fbin.write((char const*)a.data().data(), a.data().size());
    }

    return 0;
}
