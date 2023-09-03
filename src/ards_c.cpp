#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{

    ards::compiler_t c;
    ards::assembler_t a;

    std::string si = R"(
void main()
{
    $debug_break();
    
    $assert(1);
    $assert(-1);
    $assert(2);
    $assert(2u);
    $assert(0x10000000);
    $assert(!0);
    $assert(!false);
    $assert(!!true);
    $assert(!!!false);
    
    $assert(0 == 0);
    $assert(1 == 1);
    $assert(1 != 0);
    $assert(1u == 1);
    $assert(0xff == -1);
    $assert(1 + 2 == 3);
    $assert(1 + 2 == 2 + 1);
    
    $assert(1 < 2);
    $assert(2 > 1);
    $assert(1 <= 2);
    $assert(2 >= 1);
    $assert(1 >= 1);
    $assert(1 <= 1);
    $assert(-1 <= -1);
    $assert(-1 < 0);
    
    $assert((1 == 1) == true);
    $assert((1 == 0) == false);
    
    // integer promotion / implicit conversion
    $assert(100 + 28 != 128);
    $assert(100 + 28 == -128);
    $assert(100 + 29 == -127);
    $assert(100u + 28u == 128);
    
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
