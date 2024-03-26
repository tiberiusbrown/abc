#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include "ide_common.hpp"

#include <fstream>
#include <sstream>
#include <strstream>

#include <imgui.h>
#include <imgui_internal.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

#include <basic_main.hpp>
#endif

#include <sokol_time.h>

#ifndef ABC_VERSION
#define ABC_VERSION "(unknown version)"
#endif

char const* const abc_version = ABC_VERSION;

std::unique_ptr<absim::arduboy_t> arduboy;
float pixel_ratio;
project_t project;
std::unordered_map<std::string, std::unique_ptr<open_file_t>> open_files;

static bool fs_ready = false;
static ImGuiStyle default_style;
ImGuiID dockspace_id;
ImGuiID dockid_project;
ImGuiID dockid_welcome;

ImFont* font_normal;
ImFont* font_h1;
ImFont* font_h2;
ImFont* font_h3;

static bool show_asm;
TextEditor asm_editor;

bool show_sys;

uint64_t save_tick;

extern unsigned char const ProggyVector[198188];

#include "font_icons.hpp"

open_file_t::open_file_t(std::string const& filename)
    : path(std::filesystem::path(filename).lexically_normal())
    , rel_path(path.lexically_relative(project.root.path))
    , dirty(false)
    , open(true)
{
}

void open_file_t::save()
{
    if(!dirty) return;
    save_impl();
    dirty = false;
    save_tick = stm_now() + 1'000'000'000ull * 500; // 500ms
}

void open_file_t::window()
{
    using namespace ImGui;
    if(!open) return;
    SetNextWindowSize(
        { 400 * pixel_ratio, 400 * pixel_ratio },
        ImGuiCond_FirstUseEver);
    dock_next_window_to_welcome();
    if(Begin(window_id().c_str(), &open))
    {
        window_contents();
    }
    End();
}

std::string open_file_t::read_as_string() const
{
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if(f.fail()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string open_file_t::filename() const
{
    return rel_path.generic_string();
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

void frame_logic()
{
    if(project.active() && ImGui::IsKeyPressed(ImGuiKey_F5, false))
    {
        player_run();
    }

    // save files if dirty
    if(stm_now() > save_tick)
    {
        save_tick = UINT64_MAX;
#ifdef __EMSCRIPTEN__
        EM_ASM(
            FS.syncfs(function(err) {});
        );
#endif
    }
}

void dock_next_window_to_welcome()
{
    using namespace ImGui;
    ImGuiWindow* w = FindWindowByName("Welcome");
    if(!w) return;
    SetNextWindowDockID(w->DockId, ImGuiCond_FirstUseEver);
}

void imgui_content()
{
    using namespace ImGui;

    uint64_t dt = platform_get_ms_dt();

    if(BeginMainMenuBar())
    {
        if(BeginMenu("Project"))
        {
            import_menu_item();
            if(!project.active())
                BeginDisabled();
            export_menu_items();
            if(!project.active())
                EndDisabled();
            EndMenu();
        }
        if(BeginMenu("Tools"))
        {
            if(MenuItem("System Reference", nullptr, show_sys))
                show_sys = !show_sys;
            if(MenuItem("Disassembly", nullptr, show_asm))
                show_asm = !show_asm;
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
        //new_project();
        firstinit = true;

        // set up docking
        using namespace ImGui;
        ImGuiID t;
        DockBuilderSplitNode(
            dockspace_id, ImGuiDir_Left, 0.20f, &dockid_project, &t);
        DockBuilderSplitNode(
            t, ImGuiDir_Left, 0.70f, &dockid_welcome, nullptr);
        ImGuiDockNode* p = ImGui::DockBuilderGetNode(dockid_project);
        p->LocalFlags |=
            ImGuiDockNodeFlags_NoTabBar |
            ImGuiDockNodeFlags_NoDockingOverMe |
            ImGuiDockNodeFlags_NoDockingSplitMe;
        ImGuiDockNode* root = ImGui::DockBuilderGetNode(dockspace_id);
        root->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplitMe;

        update_cached_files();
    }

    //ShowDemoWindow();

    SetNextWindowDockID(dockid_project, ImGuiCond_Always);
    if(Begin("Project"))
    {
        project_window_contents();
    }
    End();

    SetNextWindowDockID(dockid_welcome, ImGuiCond_FirstUseEver);
    if(Begin("Welcome"))
    {
        project_welcome();
    }
    End();

    if(show_asm)
    {
        dock_next_window_to_welcome();
        if(Begin("Disassembly", &show_asm))
        {
            asm_editor.Render("###asm");
        }
        End();
    }

    if(show_sys)
    {
        dock_next_window_to_welcome();
        if(Begin("System Reference", &show_sys))
        {
            ide_system_reference();
        }
        End();
    }

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
    cfg.OversampleH = 3;
    cfg.OversampleV = 1;
    io.Fonts->Clear();
    font_normal = io.Fonts->AddFontFromMemoryTTF(
        (void*)ProggyVector, sizeof(ProggyVector), font_size, &cfg);
    cfg.MergeMode = true;
    cfg.GlyphOffset = { 0, 1.5f * pixel_ratio };
    cfg.GlyphMinAdvanceX = font_size;
    cfg.RasterizerMultiply = 1.0f;
    static ImWchar const icon_ranges[] =
    {
        0xf004, 0xf35b,
        0
    };
    io.Fonts->AddFontFromMemoryTTF(
        (void*)fa_regular_400, sizeof(fa_regular_400), font_size, &cfg, icon_ranges);

    cfg.MergeMode = false;
    cfg.GlyphOffset = {};
    cfg.GlyphMinAdvanceX = 0;
    cfg.RasterizerMultiply = 1.5f;
    font_h3 = io.Fonts->AddFontFromMemoryTTF(
        (void*)ProggyVector, sizeof(ProggyVector), font_size * 1.25f, &cfg);
    font_h2 = io.Fonts->AddFontFromMemoryTTF(
        (void*)ProggyVector, sizeof(ProggyVector), font_size * 1.5f, &cfg);
    font_h1 = io.Fonts->AddFontFromMemoryTTF(
        (void*)ProggyVector, sizeof(ProggyVector), font_size * 2.f, &cfg);
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

#ifdef __EMSCRIPTEN__
extern "C" void postsyncfs()
{
    fs_ready = true;
    project = {};
    project.root.is_dir = true;
    project.root.path = std::filesystem::path("/offline/abc").lexically_normal();
    if(!std::filesystem::exists(project.root.path))
    {
        std::error_code ec;
        std::filesystem::create_directory(project.root.path);
        {
            std::ofstream f(project.root.path / "main.abc", std::ios::out | std::ios::binary);
            f.write((char const*)BASIC_MAIN, sizeof(BASIC_MAIN));
        }
    }
    update_cached_files();
}
#endif

void init()
{
    printf("ABC IDE " ABC_VERSION " by Peter Brown\n");

#ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.mkdir('/offline');
        FS.mount(IDBFS, {}, '/offline');
        FS.syncfs(true, function(err) { ccall('postsyncfs', 'v'); });
    );
#endif

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

    asm_editor.SetReadOnly(true);
    asm_editor.SetShowWhitespaces(false);
    asm_editor.SetColorizerEnable(false);
    auto p = asm_editor.GetPalette();
    p[(int)TextEditor::PaletteIndex::Default] = 0xffdddddd;
    asm_editor.SetPalette(p);
}

void make_tab_visible(std::string const& window_name)
{
    //ImGuiWindow* window = ImGui::FindWindowByName(window_name.c_str());
    //if(window == NULL || window->DockNode == NULL || window->DockNode->TabBar == NULL)
    //    return;
    //window->DockNode->TabBar->NextSelectedTabId = window->TabId;
    ImGui::SetWindowFocus(window_name.c_str());
}

static bool ends_with(std::string const& str, std::string const& suffix)
{
    if(str.size() < suffix.size()) return false;
    return std::equal(suffix.begin(), suffix.end(), str.end() - suffix.size());
}

bool open_file(std::filesystem::path const& p)
{
    std::string filename = p.lexically_normal().generic_string();
    if(open_files.count(filename) != 0)
    {
        make_tab_visible(open_files[filename]->window_id());
        return true;
    }
    if(ends_with(filename, ".abc"))
        open_files[filename] = create_code_file(filename);
    else
        return false;
    make_tab_visible(open_files[filename]->window_id());
    return true;
}

void close_file(std::filesystem::path const& p)
{
    std::string filename = p.lexically_normal().generic_string();
    if(open_files.count(filename) == 0)
        return;
    open_files.erase(filename);
}

static void refresh_cached_files_in_dir(cached_file_t& dir)
{
    assert(dir.is_dir);
    dir.children.clear();
    for(auto const& e : std::filesystem::directory_iterator(dir.path))
    {
        if(e.path().filename().c_str()[0] == '.')
            continue;
        cached_file_t f{};
        f.path = e.path();
        if(e.is_directory())
        {
            f.is_dir = true;
            refresh_cached_files_in_dir(f);
        }
        dir.children.emplace_back(std::move(f));
    }
    std::sort(dir.children.begin(), dir.children.end());
}

static void catalog_all_files(
    cached_file_t const& dir,
    std::unordered_set<std::string>& files)
{
    assert(dir.is_dir);
    for(auto const& child : dir.children)
    {
        if(child.is_dir)
            catalog_all_files(child, files);
        else
            files.insert(child.path.generic_string());
    }
}

void update_cached_files()
{
    if(!project.active()) return;
    refresh_cached_files_in_dir(project.root);

    std::unordered_set<std::string> files;
    catalog_all_files(project.root, files);
    std::vector<std::string> files_to_close;
    for(auto const& kv : open_files)
    {
        auto const& filename = kv.first;
        if(files.count(filename) == 0)
            files_to_close.push_back(filename);
    }

    // TODO: make file dirty instead of closing?
    for(auto const& f : files_to_close)
        close_file(f);
}
