#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{

    ards::compiler_t c;
    ards::assembler_t a;

#if 0
    std::string si = R"(
void main()
{
    set_frame_rate(30);
}
)";
#else
    std::string si = R"(
u8 x;
void increment_x()
{
    x = x + 1;
}
void main()
{
    i16 blah;
    increment_x();
    blah = x;
    draw_filled_rect(0, 0, x, blah, 1);
    display();
}
)";
//void main()
//{
//    set_frame_rate(30);
//
//    while(1)
//    {
//        while(!next_frame())
//            ;
//        draw_filled_rect(x, 0, 16, 16, 1);
//        increment_x();
//        display();
//    }
//}
#endif

    std::istringstream fi(si);
    std::stringstream fasm;

    c.compile(fi, fasm);
    for(auto const& e : c.errors())
    {
        std::cerr << "Compiler Error" << std::endl;
        std::cerr << argv[2] << ":" << e.line_info.first << ":" << e.line_info.second << std::endl;
        std::cerr << e.msg << std::endl;
    }
    if(!c.errors().empty())
        return 1;

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
