#include <ards_compiler.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{

    ards::compiler_t c;

    std::string si = R"(

int x;

void main()
{
    int a;
    a = 1 + 2;
}

)";
    std::string so;
    std::istringstream fi(si);
    std::ostringstream fo(so);

    c.compile(fi, fo);
    for(auto const& e : c.errors())
    {
        std::cerr << "Compiler Error" << std::endl;
        std::cerr << argv[2] << ":" << e.line << ":" << e.column << std::endl;
        std::cerr << e.msg << std::endl;
    }
    if(!c.errors().empty())
        return 1;

    return 0;
}
