#include "ide_common.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <strstream>

#include <imgui.h>

#include "font_icons.hpp"

static std::unique_ptr<texture_t> display_texture;

void player_run()
{
    if(!arduboy || project.binary.empty())
        return;
    std::istrstream ss(
        (char const*)project.binary.data(),
        project.binary.size());
    auto t = arduboy->load_file("fxdata.bin", ss);
    arduboy->paused = true;
    if(!t.empty())
        return;
    arduboy->paused = false;
}

void player_window_contents(uint64_t dt)
{
    using namespace ImGui;

    if(!display_texture)
        display_texture = std::make_unique<texture_t>(128, 64);

    if(arduboy && arduboy->cpu.decoded && !arduboy->paused)
    {
        arduboy->cpu.enabled_autobreaks.reset();
        arduboy->allow_nonstep_breakpoints = false;
        arduboy->display.enable_filter = true;

        uint8_t pinf = 0xf0;
        uint8_t pine = 0x40;
        uint8_t pinb = 0x10;

        std::array<ImGuiKey, 4> keys =
        {
            ImGuiKey_UpArrow,
            ImGuiKey_RightArrow,
            ImGuiKey_DownArrow,
            ImGuiKey_LeftArrow,
        };
        //std::rotate(keys.begin(), keys.begin() + settings.display_orientation, keys.end());
        if(IsKeyDown(keys[0])) pinf &= ~0x80;
        if(IsKeyDown(keys[1])) pinf &= ~0x40;
        if(IsKeyDown(keys[2])) pinf &= ~0x10;
        if(IsKeyDown(keys[3])) pinf &= ~0x20;

        if(IsKeyDown(ImGuiKey_A)) pine &= ~0x40;
        if(IsKeyDown(ImGuiKey_B) || ImGui::IsKeyDown(ImGuiKey_S)) pinb &= ~0x10;

        arduboy->cpu.data[0x23] = pinb;
        arduboy->cpu.data[0x2c] = pine;
        arduboy->cpu.data[0x2f] = pinf;
        arduboy->frame_bytes_total = 1024;

        constexpr uint64_t MS_TO_PS = 1000000000ull;
        uint64_t dtps = dt * MS_TO_PS;
        if(dtps > 0)
            arduboy->advance(dtps);
    }

    if(arduboy && !arduboy->cpu.sound_buffer.empty())
    {
        platform_send_sound();
        arduboy->cpu.sound_buffer.clear();
    }

    if(display_texture && arduboy && arduboy->cpu.decoded)
    {
        std::vector<uint8_t> pixels;
        pixels.resize(128 * 64 * 4);
        for(size_t i = 0; i < 128 * 64; ++i)
        {
            auto t = arduboy->display.filtered_pixels[i];
            pixels[i * 4 + 0] = t;
            pixels[i * 4 + 1] = t;
            pixels[i * 4 + 2] = t;
            pixels[i * 4 + 3] = 255;
        }
        display_texture->update(pixels);

        auto avail = GetContentRegionAvail();
        int z = 1;
        while(128 * z <= avail.x && 64 * z <= avail.y)
            ++z;
        --z;
        ImVec2 size = { 128.f * z, 64.f * z };
        float offset = std::round((avail.x - size.x) * 0.5f);
        constexpr float C = 0.5f;

        SetCursorPosX(GetCursorPosX() + offset);
        Image(
            display_texture->imgui_id(),
            size,
            { 0, 0 }, { 1, 1 },
            { 1, 1, 1, 1 },
            { C, C, C, 1 });
    }

    Button("Debug " ICON_FA_PLAY_CIRCLE);
    Button("Pause " ICON_FA_PAUSE_CIRCLE);
    Button("Stop " ICON_FA_STOP_CIRCLE);

}

void player_shutdown()
{
    display_texture.reset();
}
