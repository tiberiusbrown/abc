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

s16 x;

void increment_x()
{
    x = x + 2 + 1;
}

void main()
{
    set_frame_rate(30);

    while(1)
    {
        while(!next_frame())
            ;
        draw_filled_rect(x, 0, 16, 16, 1);
        increment_x();
        display();
    }
}

)";
#endif

    std::string so;
    std::istringstream fi(si);
    std::ostringstream fo(so);

    c.compile(fi, fo);
    for(auto const& e : c.errors())
    {
        std::cerr << "Compiler Error" << std::endl;
        std::cerr << argv[2] << ":" << e.line_info.first << ":" << e.line_info.second << std::endl;
        std::cerr << e.msg << std::endl;
    }
    if(!c.errors().empty())
        return 1;

    return 0;
}
