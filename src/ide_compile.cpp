#include "ide_common.hpp"

#include <algorithm>
#include <sstream>

#include <ards_assembler.hpp>
#include <ards_compiler.hpp>

void compile_all()
{
    ards::compiler_t c{};

    for(auto& [k, v] : editors)
    {
        std::stringstream ss(v.editor.GetText());
        std::stringstream sout;
        c.compile(ss, sout);
        TextEditor::ErrorMarkers errors;
        for(auto const& e : c.errors())
        {
            int line = (int)e.line_info.first;
            line = std::min(line, v.editor.GetTotalLines());
            errors.insert(std::make_pair(line, e.msg));
        }
        v.editor.SetErrorMarkers(errors);
    }
}
