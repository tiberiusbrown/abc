#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{

    ards::compiler_t c;

#if 0
    std::string si = R"(
void main()
{
    set_frame_rate(30);
}
)";
#else
    std::string si = R"(
u8 width;
void main()
{
    u8 w;
    w = width;
    draw_filled_rect(0, 0, w, w, 1);
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
    std::ostringstream fo;

    c.compile(fi, fo);
    for(auto const& e : c.errors())
    {
        std::cerr << "Compiler Error" << std::endl;
        std::cerr << argv[2] << ":" << e.line_info.first << ":" << e.line_info.second << std::endl;
        std::cerr << e.msg << std::endl;
    }
    if(!c.errors().empty())
        return 1;

    std::cout << fo.str() << std::endl;

    return 0;
}
