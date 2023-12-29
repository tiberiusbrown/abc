#include <ards_assembler.hpp>
#include <ards_compiler.hpp>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>

#include <all_fonts.hpp>
#include <stb_truetype.h>

#include <stdio.h>

int main()
{
    std::map<std::string, ards::sysfunc_t> const sys_names(
        ards::sys_names.begin(), ards::sys_names.end());

    FILE* f;
    
    f = fopen(DOCS_DIR "/system.md", "w");
    if(!f) return 1;

    fprintf(f, "# Predefined Constants\n\n```c\n");
    for(auto const& c : ards::builtin_constexprs)
    {
        fprintf(f, "%s %s;\n", ards::type_name(c.type).c_str(), c.name.c_str());
    }
    fprintf(f, "```\n\n");

    fprintf(f, "# System Calls\n\n```c\n");
    for(auto const& [k, v] : sys_names)
    {
        auto it = ards::sysfunc_decls.find(v);
        if(it == ards::sysfunc_decls.end()) continue;
        auto const& decl = it->second;
        fprintf(f, "%-5s $%s(", ards::type_name(decl.return_type).c_str(), k.c_str());
        for(size_t i = 0; i < decl.arg_types.size(); ++i)
        {
            if(i != 0) fprintf(f, ", ");
            fprintf(f, "%s %s", ards::type_name(decl.arg_types[i]).c_str(), decl.arg_names[i].c_str());
        }
        if(ards::sysfunc_is_format(v))
            fprintf(f, ", ...");
        fprintf(f, ");\n");
    }
    fprintf(f, "```\n\n");

    fclose(f);

    f = fopen(DOCS_DIR "/builtin_fonts.md", "w");
    if(!f) return 1;

    fprintf(f, "# Built-in Font Assets\n");
    fprintf(f, "| Predefined Variable | Line Height |\n");
    fprintf(f, "|---|---|\n");

    std::vector<std::pair<int, std::string>> fonts;

    for(auto const& font : ALL_FONTS)
    {
        std::vector<uint8_t> data;
        ards::ast_node_t dummy{};
        ards::ast_node_t pixels{};
        pixels.type = ards::AST::INT_CONST;
        pixels.value = font.pixels;
        dummy.children.push_back(pixels);
        ards::compiler_t{}.encode_font_ttf(data, dummy, font.data, font.size);
        fonts.push_back({ (int)data[512], font.name });
    }

    std::sort(fonts.begin(), fonts.end());

    for(auto const& font : fonts)
        fprintf(f, "| `%s` | %d |\n", font.second.c_str(), font.first);

    fclose(f);

    return 0;
}
