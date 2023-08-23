#include "ide_common.hpp"

#include <imgui.h>

#ifndef ABC_VERSION
#define ABC_VERSION "(unknown version)"
#endif

std::unique_ptr<absim::arduboy_t> arduboy;
float pixel_ratio;

static ImGuiStyle default_style;

extern unsigned char const ProggyVector[198188];

void frame_logic()
{
    if(ImGui::IsKeyPressed(ImGuiKey_F5, false))
        compile_all();
}

void imgui_content()
{
    using namespace ImGui;

    for(auto& [k, v] : editors)
    {
        v.update();
    }
    for(auto it = editors.begin(); it != editors.end();)
    {
        if(!it->second.open)
            it = editors.erase(it);
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
#ifndef ARDENS_NO_DEBUGGER
    ImPlot::DestroyContext();
#endif
}

void init()
{
    printf(
        "ABC IDE " ABC_VERSION " by Peter Brown\n");

#ifdef __EMSCRIPTEN__
    EM_ASM(
        FS.mkdir('/offline');
    FS.mount(IDBFS, {}, '/offline');
    FS.syncfs(true, function(err) { ccall('postsyncfs', 'v'); });
    );
#endif

    arduboy = std::make_unique<absim::arduboy_t>();

    //init_settings();

    ImGuiIO& io = ImGui::GetIO();
#if !defined(__EMSCRIPTEN__)
    ImGui::LoadIniSettingsFromDisk(io.IniFilename);
    //settings_loaded = true;
#endif

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

#if defined(__EMSCRIPTEN__)
    io.IniFilename = nullptr;
#endif

    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    style.Colors[ImGuiCol_PopupBg].w = 1.0f;

    default_style = style;

    arduboy->fx.erase_all_data();
    arduboy->reset();
    arduboy->fx.min_page = 0xffff;
    arduboy->fx.max_page = 0xffff;

    editors["test.abc"] = editor_t("test.abc");
}
