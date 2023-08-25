#include "ide_common.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <ards_assembler.hpp>
#include <ards_compiler.hpp>

void compile_all()
{
    ards::compiler_t c{};
    ards::assembler_t a{};

    std::vector<std::string> asms;
    bool any_errors = false;

    for(auto& [k, v] : editors)
    {
        std::stringstream ss(v.editor.GetText());
        std::stringstream sout;
        c.compile(ss, sout);
        asms.push_back(sout.str());
        TextEditor::ErrorMarkers errors;
        for(auto const& e : c.errors())
        {
            int line = (int)e.line_info.first;
            line = std::min(line, v.editor.GetTotalLines());
            errors.insert(std::make_pair(line, e.msg));
            any_errors = true;
        }
        v.editor.SetErrorMarkers(errors);
    }

    if(any_errors)
        return;

    for(auto const& s : asms)
    {
        std::stringstream ss(s);
        auto e = a.assemble(ss);
        if(!e.msg.empty())
            return;
    }

    {
        auto e = a.link();
        if(!e.msg.empty())
            return;
    }

    // a.data();
}
