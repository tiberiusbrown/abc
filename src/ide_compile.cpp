#include "ide_common.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <ards_assembler.hpp>
#include <ards_compiler.hpp>

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if(ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool compile_all()
{
    ards::compiler_t c{};
    ards::assembler_t a{};

    std::vector<std::string> asms;

    project.errors.clear();

    for(auto& [n, f] : open_files)
        f->save();

    for(auto& [n, f] : project.files)
    {
        if(!ends_with(n, ".abc"))
            continue;
        std::stringstream ss(f->content_as_string());
        std::stringstream sout;
        c.compile(ss, sout, n);
        asms.push_back(sout.str());
        for(auto const& e : c.errors())
            project.errors[n].push_back(e);
    }

    if(!project.errors.empty())
        return false;

    for(auto const& s : asms)
    {
        std::stringstream ss(s);
        auto e = a.assemble(ss);
        if(!e.msg.empty())
        {
            project.errors["<Assembler>"].push_back(e);
            return false;
        }
    }

    {
        auto e = a.link();
        if(!e.msg.empty())
        {
            project.errors["<Linker>"].push_back(e);
            return false;
        }
    }

    project.binary = a.data();
    return true;
}
