#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include "ide_common.hpp"

#include <strstream>

#include <imgui.h>
#include <imgui_internal.h>

#ifndef ABC_VERSION
#define ABC_VERSION "(unknown version)"
#endif

char const* const abc_version = ABC_VERSION;

std::unique_ptr<absim::arduboy_t> arduboy;
float pixel_ratio;
project_t project;
std::unordered_map<std::string, std::unique_ptr<open_file_t>> open_files;

static ImGuiStyle default_style;
ImGuiID dockspace_id;
ImGuiID dockid_project;

extern unsigned char const ProggyVector[198188];

#include "font_icons.hpp"

std::string project_file_t::content_as_string() const
{
    return std::string(content.begin(), content.end());
}

void project_file_t::set_content(std::string const& s)
{
    content = std::vector<uint8_t>(s.begin(), s.end());
}

void open_file_t::save()
{
    if(!dirty) return;
    save_impl();
    dirty = false;
}

void open_file_t::window()
{
    using namespace ImGui;
    if(!open) return;
    ImGui::SetNextWindowSize(
        { 400 * pixel_ratio, 400 * pixel_ratio },
        ImGuiCond_FirstUseEver);
    if(Begin(window_id().c_str(), &open))
    {
        window_contents();
    }
    End();
}

std::string open_file_t::filename() const
{
    if(auto f = file.lock())
        return f->filename;
    return "<deleted>";
}

std::string open_file_t::window_title()
{
    std::string title = filename();
    if(dirty) title += "*";
    return title;
}

std::string open_file_t::window_id()
{
    return window_title() + "###file_" + filename();
}

std::shared_ptr<project_file_t> project_t::get_file(std::string const& filename)
{
    if(auto it = files.find(filename); it != files.end())
        return it->second;
    return nullptr;
}

void frame_logic()
{
    if(ImGui::IsKeyPressed(ImGuiKey_F5, false))
    {
        player_run();
    }
}

void imgui_content()
{
    using namespace ImGui;

    uint64_t dt = platform_get_ms_dt();

    if(BeginMainMenuBar())
    {
        if(BeginMenu("File"))
        {
            export_menu_items();
            EndMenu();
        }
        {
            float w = ImGui::CalcTextSize(abc_version, NULL, true).x;
            w += ImGui::GetStyle().ItemSpacing.x;
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - w);
            ImGui::MenuItem(ABC_VERSION "##version", nullptr, nullptr, false);
        }
        EndMainMenuBar();
    }

    dockspace_id = DockSpaceOverViewport(GetMainViewport());
    {
        ImGuiDockNode* node = ImGui::DockBuilderGetCentralNode(dockspace_id);
        ImGuiWindowClass centralAlways = {};
        centralAlways.DockNodeFlagsOverrideSet |= ImGuiDockNodeFlags_NoTabBar | ImGuiDockNodeFlags_NoDockingOverMe;
        SetNextWindowClass(&centralAlways);
        SetNextWindowDockID(node->ID, ImGuiCond_Always);
        Begin("Display");
        player_window_contents(dt);
        End();
    }

    static bool firstinit = false;
    if(!firstinit)
    {
        new_project();
        firstinit = true;
    }

    //ShowDemoWindow();

    SetNextWindowDockID(dockid_project, ImGuiCond_Always);
    if(Begin("Project"))
    {
        project_window_contents();
    }
    End();

    for(auto& [k, v] : open_files)
    {
        v->window();
    }
    for(auto it = open_files.begin(); it != open_files.end();)
    {
        if(!it->second->open)
            it = open_files.erase(it);
        else
            ++it;
    }
}

bool update_pixel_ratio()
{
    float ratio = platform_pixel_ratio();
    bool changed = (ratio != pixel_ratio);
    pixel_ratio = ratio;
    return changed;
}

void define_font()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg;
    float font_size = DEFAULT_FONT_SIZE * pixel_ratio;
    cfg.FontDataOwnedByAtlas = false;
    cfg.RasterizerMultiply = 1.5f;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    io.Fonts->Clear();
    io.Fonts->AddFontFromMemoryTTF(
        (void*)ProggyVector, sizeof(ProggyVector), font_size, &cfg);
    cfg.MergeMode = true;
    cfg.GlyphOffset = { 0, 1.5f * pixel_ratio };
    cfg.GlyphMinAdvanceX = font_size;
    cfg.RasterizerMultiply = 1.2f;
    static ImWchar const icon_ranges[] =
    {
        0xf004, 0xf35b,
        0
    };
    io.Fonts->AddFontFromMemoryTTF(
        (void*)fa_regular_400, sizeof(fa_regular_400), font_size, &cfg, icon_ranges);
}

void rebuild_fonts()
{
    platform_destroy_fonts_texture();
    define_font();
    platform_create_fonts_texture();
}

void rescale_style()
{
    auto& style = ImGui::GetStyle();
    style = default_style;
    style.ScaleAllSizes(pixel_ratio);
}

void shutdown()
{
    player_shutdown();
}

void init()
{
    printf("ABC IDE " ABC_VERSION " by Peter Brown\n");

    //init_settings();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
#if defined(__EMSCRIPTEN__)
    //io.IniFilename = nullptr;
#else
    //ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    //settings_loaded = true;
#endif

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.Colors[ImGuiCol_PopupBg].w = 1.0f;

    default_style = style;

    arduboy = std::make_unique<absim::arduboy_t>();
    {
        std::istrstream ss((char const*)VM_HEX_ARDUBOYFX, (int)VM_HEX_ARDUBOYFX_SIZE);
        auto t = arduboy->load_file("vm.hex", ss);
        assert(t.empty());
    }
    arduboy->fx.erase_all_data();
    arduboy->reset();
    arduboy->paused = true;
    arduboy->fx.min_page = 0xffff;
    arduboy->fx.max_page = 0xffff;
}

void make_tab_visible(std::string const& window_name)
{
    //ImGuiWindow* window = ImGui::FindWindowByName(window_name.c_str());
    //if(window == NULL || window->DockNode == NULL || window->DockNode->TabBar == NULL)
    //    return;
    //window->DockNode->TabBar->NextSelectedTabId = window->TabId;
    ImGui::SetWindowFocus(window_name.c_str());
}
