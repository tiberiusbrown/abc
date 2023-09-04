#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{

    ards::compiler_t c;
    ards::assembler_t a;

    std::string si = R"(
u8 [4] a1;
u16[4] a2;
u24[4] a3;
u32[4] a4;

u32 g;

void main()
{
    $debug_break();
    
    $assert(a1[2] == 0);
    $assert(a2[2] == 0);
    $assert(a3[2] == 0);
    $assert(a4[2] == 0);
    
    for(u8 i = 0; i < 4; i = i + 1)
    {
        a1[i] = 0x42;
        a2[i] = 0x4200;
        a3[i] = 0x420000;
        a4[i] = 0x42000000;
    }
    
    $assert(a1[2] == 0x42);
    $assert(a2[2] == 0x4200);
    $assert(a3[2] == 0x420000);
    $assert(a4[2] == 0x42000000);
    
    {
        u32 loc = 0;
        u32& r = loc;
        $assert(loc == 0);
        r = 0xcafebabe;
        $assert(loc == 0xcafebabe);
        $assert(r   == 0xcafebabe);
        //$assert((loc & 0xff00ff00) == 0xca00ba00);
        $assert((r   & 0xff00ff00) == 0xca00ba00);
    }
    
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
