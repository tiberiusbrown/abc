#include "ide_common.hpp"

#include "ards_assembler.hpp"
#include "ards_compiler.hpp"

void ide_system_reference()
{
    using namespace ImGui;
    static std::map<std::string, ards::sysfunc_t> const sys_names(
        ards::sys_names.begin(), ards::sys_names.end());

    constexpr ImGuiTableFlags tf =
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_SizingFixedFit |
        0;
    constexpr ImGuiTableColumnFlags cf =
        //ImGuiTableColumnFlags_WidthFixed |
        0;

    PushFont(font_h2);
    TextUnformatted("Predefined Constants");
    PopFont();
    Separator();
    if(BeginTable("##globals_usage", 2, tf, { -1, 0 }))
    {
        TableSetupColumn("Type", cf);
        TableSetupColumn("Name", cf);
        TableHeadersRow();
        for(auto const& c : ards::builtin_constexprs)
        {
            TableNextRow();
            TableSetColumnIndex(0);
            TextUnformatted(ards::type_name(c.type).c_str());
            TableSetColumnIndex(1);
            TextUnformatted(c.name.c_str());
        }
        EndTable();
    }

    PushFont(font_h2);
    TextUnformatted("System Functions");
    PopFont();
    Separator();
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
            TextUnformatted(ards::type_name(decl.return_type).c_str());
            TableSetColumnIndex(1);
            Text("$%s", k.c_str());
            TableSetColumnIndex(2);
            std::string t;
            for(size_t i = 0; i < decl.arg_types.size(); ++i)
            {
                if(i != 0) t += ", ";
                t += ards::type_name(decl.arg_types[i]);
            }
            TextUnformatted(t.c_str());
        }
        EndTable();
    }

}
