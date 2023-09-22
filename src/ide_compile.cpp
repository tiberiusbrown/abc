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

    do
    {
        std::string n = "main.abc";
        if(project.files.count(n) == 0)
            break;
        auto const& f = project.files[n];
        std::stringstream ss(f->content_as_string());
        std::stringstream sout;
        auto loader = [&](std::string const& filename, std::vector<char>& t) -> bool {
            auto it = project.files.find(filename);
            if(it == project.files.end()) return false;
            auto const& c = it->second->content;
            t = std::vector<char>(c.begin(), c.end());
            return true;
        };
        c.compile("", n.substr(0, n.size() - 4), loader, sout);
        asms.push_back(sout.str());
        for(auto const& e : c.errors())
            project.errors[n].push_back(e);
    } while(0);

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
