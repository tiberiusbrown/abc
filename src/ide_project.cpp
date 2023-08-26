#include "ide_common.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "font_icons.hpp"

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if(ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void project_window_contents()
{
    using namespace ImGui;

    if(Selectable(ICON_FA_LIST_ALT " Project Info"))
    {
        if(open_files.count(INFO_FILENAME) == 0)
            open_files[INFO_FILENAME] = create_project_info_file(INFO_FILENAME);
        make_tab_visible(open_files[INFO_FILENAME]->window_id());
    }

    Separator();

    for(auto const& [n, f] : project.files)
    {
        if(n == INFO_FILENAME)
            continue;
        if(!Selectable((ICON_FA_FILE " " + n).c_str()))
            continue;
        if(open_files.count(n) == 0)
        {
            if(ends_with(n, ".abc"))
                open_files[n] = create_code_file(n);
        }
        make_tab_visible(open_files[n]->window_id());
    }

}
