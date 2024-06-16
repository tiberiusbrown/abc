#include "ide_common.hpp"

#include "ards_assembler.hpp"
#include "ards_compiler.hpp"

void ide_system_reference()
{
    using namespace ImGui;
    static std::map<std::string, ards::sysfunc_t> const sys_names(
        ards::sys_names.begin(), ards::sys_names.end());

#if 0
    constexpr ImGuiTableFlags tf =
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        0;
    constexpr ImGuiTableColumnFlags cf =
        //ImGuiTableColumnFlags_WidthFixed |
        0;
#endif

    float const indent_width = CalcTextSize("    ").x;

    constexpr auto TYPE_COLOR = IM_COL32(20, 120, 200, 255);

    PushFont(font_h2);
    TextUnformatted("Predefined Constants");
    PopFont();
    Separator();
    Indent(indent_width);
    for(auto const& c : ards::builtin_constexprs)
    {
        PushStyleColor(ImGuiCol_Text, TYPE_COLOR);
        TextUnformatted(ards::type_name(c.type).c_str());
        PopStyleColor();
        SameLine();
        Text("%s;", c.name.c_str());
    }
    Unindent(indent_width);

#if 0
    if(BeginTable("##globals_usage", 2, tf, { -1, 0 }))
    {
        TableSetupColumn("Type", cf);
        TableSetupColumn("Name", cf);
        TableHeadersRow();
        for(auto const& c : ards::builtin_constexprs)
        {
            TableNextRow();
            TableSetColumnIndex(0);
            PushStyleColor(ImGuiCol_Text, TYPE_COLOR);
            TextUnformatted(ards::type_name(c.type).c_str());
            PopStyleColor();
            TableSetColumnIndex(1);
            TextUnformatted(c.name.c_str());
        }
        EndTable();
    }
#endif

    PushFont(font_h2);
    TextUnformatted("System Functions");
    PopFont();
    Separator();
    Indent(indent_width);
    for(auto const& [k, v] : sys_names)
    {
        auto it = ards::sysfunc_decls.find(v);
        if(it == ards::sysfunc_decls.end()) continue;
        auto const& decl = it->second.decl;
        PushStyleColor(ImGuiCol_Text, TYPE_COLOR);
        Text("%-4s", ards::type_name(decl.return_type).c_str());
        PopStyleColor();
        SameLine();
        Text("$%s(", k.c_str());
        SameLine(0.f, 0.f);
        for(size_t i = 0; i < decl.arg_types.size(); ++i)
        {
            if(i != 0)
            {
                SameLine(0.f, 0.f);
                TextUnformatted(",");
                SameLine();
            }
            PushStyleColor(ImGuiCol_Text, TYPE_COLOR);
            TextUnformatted(ards::type_name(decl.arg_types[i]).c_str());
            PopStyleColor();
            SameLine();
            TextUnformatted(decl.arg_names[i].c_str());
            if(i + 1 == decl.arg_types.size() && ards::sysfunc_is_format(v))
            {
                SameLine(0.f, 0.f);
                TextUnformatted(", ...");
            }
        }
        SameLine(0.f, 0.f);
        TextUnformatted(");");
    }
    Unindent(indent_width);
#if 0
    if(BeginTable("##globals_usage", 3, tf, { -1, 0 }))
    {
        TableSetupColumn("Type", cf);
        TableSetupColumn("Name", cf, CalcTextSize("$draw_filled_circle  ").x);
        TableSetupColumn("Arguments", ImGuiTableColumnFlags_WidthStretch);
        TableHeadersRow();
        for(auto const& [k, v] : sys_names)
        {
            auto it = ards::sysfunc_decls.find(v);
            if(it == ards::sysfunc_decls.end()) continue;
            auto const& decl = it->second;
            TableNextRow();
            TableSetColumnIndex(0);
            PushStyleColor(ImGuiCol_Text, TYPE_COLOR);
            TextUnformatted(ards::type_name(decl.return_type).c_str());
            PopStyleColor();
            TableSetColumnIndex(1);
            Text("$%s", k.c_str());
            TableSetColumnIndex(2);
            for(size_t i = 0; i < decl.arg_types.size(); ++i)
            {
                if(i != 0)
                {
                    SameLine(0.f, 0.f);
                    TextUnformatted(",");
                    SameLine();
                }
                PushStyleColor(ImGuiCol_Text, TYPE_COLOR);
                TextUnformatted(ards::type_name(decl.arg_types[i]).c_str());
                PopStyleColor();
                SameLine();
                TextUnformatted(decl.arg_names[i].c_str());
            }
        }
        EndTable();
    }
#endif

}
