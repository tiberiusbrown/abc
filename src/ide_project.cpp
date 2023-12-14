#include "ide_common.hpp"

#include <fstream>
#include <cassert>

#include <imgui.h>
#include <imgui_internal.h>

#ifdef __EMSCRIPTEN__
#include <emscripten_browser_file.h>
#else
#include <nfd.hpp>
#endif

#include "font_icons.hpp"

static char const* PROJECT_MODAL_ID = "###projectmodal";

enum modal_type_t
{
    MT_NONE,
    MT_NEW_CODE,
    MT_DELETE,
    MT_RENAME,
} modal_type = MT_NONE;
static char nf_buf[128]{};
static char const* nf_ext = "";
static std::filesystem::path modal_path;

inline bool ends_with(std::string const& value, std::string const& ending)
{
    if(ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

static void modal()
{
    using namespace ImGui;

    if(modal_type == MT_NONE) return;

    if(!IsPopupOpen(PROJECT_MODAL_ID))
        OpenPopup(PROJECT_MODAL_ID);

    std::string title = "???";
    switch(modal_type)
    {
    case MT_NEW_CODE: title = "New ABC Code File"; break;
    case MT_DELETE: title = "Delete File"; break;
    case MT_RENAME: title = "Rename File"; break;
    default: break;
    }

    title += PROJECT_MODAL_ID;
    if(!BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    if(modal_type == MT_NEW_CODE)
    {
        AlignTextToFramePadding();
        TextUnformatted("Name:");
        SameLine();
        InputText("##filename", nf_buf, sizeof(nf_buf));
        SameLine();
        TextUnformatted(nf_ext);
    }
    else if(modal_type == MT_DELETE)
    {
        TextUnformatted("Delete the following file?");
        Separator();
        TextUnformatted(modal_path.lexically_relative(project.root.path).string().c_str());
    }

    NewLine();
    ImVec2 button_size = { pixel_ratio * 100.f, 0.f };
    if(Button("OK", button_size))
    {
        if(modal_type == MT_NEW_CODE)
        {
            std::string filename = nf_buf;
            filename += nf_ext;
            auto p = (modal_path / filename).lexically_normal();
            std::ofstream f(p, std::ios::out);
            f.close();
            update_cached_files();
            open_file(p);
        }
        else if(modal_type == MT_DELETE)
        {
            std::error_code ec;
            std::filesystem::remove(modal_path, ec);
            //close_file(modal_path);
            update_cached_files();
        }
        modal_type = MT_NONE;
        CloseCurrentPopup();
        update_cached_files();
    }
    SameLine();
    if(Button("Cancel", button_size))
    {
        modal_type = MT_NONE;
        CloseCurrentPopup();
    }

    EndPopup();
}

#ifdef __EMSCRIPTEN__
static void web_upload_handler(
    std::string const& filename,
    std::string const& mime_type,
    std::string_view buffer,
    void* user)
{
    (void)filename;
    (void)mime_type;
    (void)user;
    auto data = std::vector<uint8_t>(buffer.begin(), buffer.end());
    process_arduboy_file(data);
    std::filesystem::path orig_path = filename;
    std::filesystem::path new_path = modal_path / orig_path.filename();
    {
        std::ofstream f(new_path, std::ios::out | std::ios::binary);
        if(!f.fail())
            f.write(data.data(), data.size());
    }
    update_cached_files();
}
#endif

static void upload_file()
{
#ifdef __EMSCRIPTEN__
    emscripten_browser_file::upload("*", web_upload_handler, nullptr);
#else
    NFD::UniquePath path;
    nfdfilteritem_t filterItem[1] = { { "Arduboy Game", "arduboy" } };
    auto result = NFD::OpenDialog(path);
    if(result != NFD_OKAY)
        return;
    std::filesystem::path orig_path = path.get();
    std::filesystem::path new_path = modal_path / orig_path.filename();
    std::filesystem::copy(orig_path, new_path);
    update_cached_files();
#endif
}

static void project_window_contents_dir(cached_file_t const& dir)
{
    using namespace ImGui;
    assert(dir.is_dir);
    for(auto const& f : dir.children)
    {
        std::string name = f.path.filename().string();
        if(f.is_dir)
        {
            if(TreeNode(name.c_str()))
            {
                project_window_contents_dir(f);
                TreePop();
            }
        }
        else
        {
            if(Selectable((ICON_FA_FILE_CODE " " + name).c_str()))
            {
                open_file(f.path);
            }
        }
        if(BeginPopupContextItem())
        {
            if(MenuItem("Delete"))
            {
                modal_type = MT_DELETE;
                modal_path = f.path;
            }
            EndPopup();
        }
    }
    if(BeginMenu("   New..."))
    {
        if(MenuItem("Code File..."))
        {
            modal_type = MT_NEW_CODE;
            modal_path = dir.path;
            nf_ext = ".abc";
            nf_buf[0] = '\0';
        }
        if(MenuItem("Upload File..."))
        {
            modal_path = dir.path;
            upload_file();
        }
        EndMenu();
    }
}

void project_window_contents()
{
    using namespace ImGui;

    if(project.active())
        project_window_contents_dir(project.root);
    else
        TextDisabled("No project open");

    modal();
}
