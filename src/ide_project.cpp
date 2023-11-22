#include "ide_common.hpp"

#include <cassert>

#include <imgui.h>
#include <imgui_internal.h>

#include "font_icons.hpp"

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if(ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

// new file type
static enum
{
    NF_NONE,
    NF_CODE,
    NF_SPRITES,
    NF_TEXT,
    NF_MUSIC,
    NF_MAX,
} nf_type;
static char nf_buf[128]{};

static void new_file_modal()
{
    using namespace ImGui;

    assert(nf_type > NF_NONE && nf_type < NF_MAX);

    TextUnformatted("Create a new file?");

    static char const* const EXTS[NF_MAX] = {
        nullptr, ".abc", ".png", ".txt", ".mid"
    };

    AlignTextToFramePadding();
    TextUnformatted("Name:");
    SameLine();
    InputText("##filename", nf_buf, sizeof(nf_buf));
    std::string filename = nf_buf;
    if(nf_type > NF_NONE && nf_type < NF_MAX)
    {
        SameLine();
        TextUnformatted(EXTS[nf_type]);
        filename += EXTS[nf_type];
    }

    NewLine();
    ImVec2 button_size = { pixel_ratio * 100.f, 0.f };
    if(Button("OK", button_size))
    {
        auto f = project.files[filename] = std::make_shared<project_file_t>();
        f->filename = filename;
        nf_type = NF_NONE;
        CloseCurrentPopup();
        open_file(filename);
    }
    SameLine();
    if(Button("Cancel", button_size))
    {
        nf_type = NF_NONE;
        CloseCurrentPopup();
    }
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
        if(n == INFO_FILENAME || !f->asset_info.empty())
            continue;
        if(!Selectable((ICON_FA_FILE_CODE " " + n).c_str()))
            continue;
        if(open_files.count(n) == 0)
        {
            if(ends_with(n, ".abc"))
                open_files[n] = create_code_file(n);
        }
        make_tab_visible(open_files[n]->window_id());
    }
    Separator();

    if(BeginMenu(ICON_FA_FILE " New file..."))
    {
        nf_buf[0] = '\0';
        if(MenuItem(ICON_FA_FILE_CODE  " ABC Code     (.abc)")) nf_type = NF_CODE;
        MenuItem(ICON_FA_FILE_IMAGE " Sprite Set   (.png)");
        MenuItem(ICON_FA_FILE_ALT   " Text Data    (.txt)");
        MenuItem(ICON_FA_FILE_AUDIO " Music        (.mid)");
        
        EndMenu();
    }

    if(nf_type != NF_NONE && !IsPopupOpen("New File"))
        OpenPopup("New File");
    if(BeginPopupModal("New File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        new_file_modal();
        EndPopup();
    }
}
