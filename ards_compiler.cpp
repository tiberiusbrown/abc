#include "ards_compiler.hpp"

namespace ards
{

static bool isspace(char c)
{
    switch(c)
    {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return true;
    default:
        return false;
    }
}

void compiler_t::compile(std::istream& fi, std::ostream& fo)
{
    parse(fi);
    if(!errs.empty()) return;

    // trim all token whitespace
    ast.recurse([](ast_node_t& n) {
        size_t size = n.data.size();
        size_t i;
        if(size == 0) return;
        for(i = 0; isspace(n.data[i]); ++i);
        n.data.remove_prefix(i);
        for(i = 0; isspace(n.data[size - i - 1]); ++i);
        n.data.remove_suffix(i);
    });

}

}
