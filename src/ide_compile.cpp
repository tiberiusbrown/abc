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
        auto fpath = project.root.path / "main.abc";
        if(!std::filesystem::exists(fpath))
        {
            project.errors["<Project>"].push_back({ "No main.abc found" });
            break;
        };
        std::stringstream sout;
        c.compile(project.root.path.generic_string(), "main", sout);
        std::string t = sout.str();
        asm_editor.SetText(t);
        asms.push_back(t);
        std::string ef;
        if(!c.errors().empty())
        {
            auto cef = std::filesystem::path(c.error_file());
            ef = cef.lexically_relative(project.root.path).generic_string();
        }
        for(auto const& e : c.errors())
            project.errors[ef].push_back(e);
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

    project.data_size = a.data_size();
    project.code_size = a.code_size();
    project.debug_size = a.debug_size();
    project.globals_size = a.globals_size();
    project.save_size = a.save_size();
    project.shades = a.num_shades();

    project.arduboy_directives = c.arduboy_directives();
    return true;
}
