#include "ide_common.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <abc_assembler.hpp>
#include <abc_compiler.hpp>

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if(ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static std::filesystem::path find_main(std::filesystem::path const& dir)
{
    if(!std::filesystem::is_directory(dir))
        return {};
    auto m = dir / "main.abc";
    if(std::filesystem::exists(m))
        return m;
    for(auto const& child : std::filesystem::directory_iterator{ dir })
    {
        m = find_main(child);
        if(!m.empty())
            return m;
    }
    return {};
}

bool compile_all()
{
    abc::compiler_t c{};
    abc::assembler_t a{};

    std::vector<std::string> asms;

    project.errors.clear();

    for(auto& [n, f] : open_files)
        f->save();

    auto fpath = find_main(project.root.path);

    do
    {
        if(!std::filesystem::exists(fpath))
        {
            project.errors["<Project>"].push_back({ "No main.abc found" });
            break;
        };
        std::stringstream sout;
        c.compile(fpath.parent_path().generic_string(), "main", sout);
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
