#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include "ide_common.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <strstream>

#include <imgui.h>
#include <imgui_internal.h>

#include "font_icons.hpp"

static std::unique_ptr<texture_t> display_texture;
bool player_active;

void player_run()
{
    if(player_active)
    {
        arduboy->reset();
        arduboy->paused = true;
        player_active = false;
    }
    if(!compile_all())
        return;
    if(!arduboy || project.binary.empty())
        return;
    std::istrstream ss(
        (char const*)project.binary.data(),
        (int)project.binary.size());
    auto t = arduboy->load_file("fxdata.bin", ss);
    arduboy->paused = true;
    if(!t.empty())
        return;
    arduboy->paused = false;
    player_active = true;
}

void player_window_contents(uint64_t dt)
{
    using namespace ImGui;

    if(!display_texture)
        display_texture = std::make_unique<texture_t>(128, 64);

    if(arduboy && arduboy->cpu.decoded && !arduboy->paused && player_active)
    {
        arduboy->cpu.enabled_autobreaks.reset();
        arduboy->allow_nonstep_breakpoints = false;
        arduboy->display.enable_filter = true;
        arduboy->display.enable_current_limiting = false;

        uint8_t pinf = 0xf0;
        uint8_t pine = 0x40;
        uint8_t pinb = 0x10;
        if(!GetIO().WantCaptureKeyboard)
        {
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
        }
        arduboy->cpu.data[0x23] = pinb;
        arduboy->cpu.data[0x2c] = pine;
        arduboy->cpu.data[0x2f] = pinf;

        arduboy->frame_bytes_total = 1024;

        constexpr uint64_t MS_TO_PS = 1000000000ull;
        uint64_t tdt = std::min<uint64_t>(dt, 30);
        uint64_t dtps = tdt * MS_TO_PS;
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
        if(player_active)
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
        }

        auto avail = GetContentRegionAvail();
        avail.x -= 2.f;
        avail.y -= 2.f;
        int z = 1;
        while(128 * z <= avail.x && 64 * z <= avail.y)
            ++z;
        if(z > 1) --z;
        ImVec2 size = { 128.f * z, 64.f * z };
        float offset = std::round((avail.x - size.x) * 0.5f);
        constexpr float C = 0.5f;
        constexpr ImVec4 COLOR = { C, C, C, 1 };

        SetCursorPosX(GetCursorPosX() + offset);
        if(player_active)
        {
            Image(
                display_texture->imgui_id(),
                size,
                { 0, 0 }, { 1, 1 },
                { 1, 1, 1, 1 },
                COLOR);
        }
        else
        {
            auto* d = GetWindowDrawList();
            ImVec2 a = GetCursorScreenPos();
            ImVec2 b = { a.x + size.x + 2.f, a.y + size.y + 2.f };
            ImRect bb = { a, b };
            ItemSize(bb);
            ItemAdd(bb, 0);
            d->AddRect(a, b, GetColorU32(COLOR));
            d->AddRectFilled(
                { a.x + 1.f, a.y + 1.f },
                { b.x - 1.f, b.y - 1.f },
                IM_COL32(30, 30, 30, 255));
        }
    }

    if(!project.active())
        BeginDisabled();

    ImVec2 button_size = {
        150.f * pixel_ratio,
        30.f * pixel_ratio
    };
    SetCursorPosX((GetWindowSize().x - button_size.x) * 0.5f);
    if(player_active)
    {
        if(Button(ICON_FA_STOP_CIRCLE " Stop", button_size))
        {
            arduboy->reset();
            arduboy->paused = true;
            player_active = false;
        }
    }
    else
    {
        if(Button(ICON_FA_PLAY_CIRCLE " Debug (F5)", button_size))
            player_run();
    }

    if(project.errors.empty() && !project.binary.empty())
    {
        Separator();
        TextUnformatted("Compilation Succeeded!");
        TextUnformatted("======================");
        Text("Dev Bin: %7d bytes  %6.1f KB",
            (int)project.binary.size(), (double)project.binary.size() / 1024);
        Text("Data:    %7d bytes  %6.1f KB",
            (int)project.data_size, (double)project.data_size / 1024);
        Text("Code:    %7d bytes  %6.1f KB",
            (int)project.code_size, (double)project.code_size / 1024);
        Text("Debug:   %7d bytes  %6.1f KB",
            (int)project.debug_size, (double)project.debug_size / 1024);
        TextUnformatted("======================");
        Text("Globals: %7d bytes", (int)project.globals_size);
        Text("Save:    %7d bytes", (int)project.save_size);
        TextUnformatted("======================");
    }

    if(!project.errors.empty())
    {
        constexpr auto flags =
            ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_BordersInner |
            0;
        Separator();
        PushStyleColor(ImGuiCol_Text, IM_COL32(200, 0, 0, 255));
        TextUnformatted("Some errors occurred during compilation:");
        PopStyleColor();
        if(BeginTable("##errorstable", 3, flags))
        {
            TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed);
            TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed);
            TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
            //TableHeadersRow();
            for(auto const& [f, errs] : project.errors)
            {
                for(auto const& e : errs)
                {
                    TableNextRow();
                    TableSetColumnIndex(0);
                    TextUnformatted(f.c_str());
                    TableSetColumnIndex(1);
                    Text("%d", (int)e.line_info.first);
                    TableSetColumnIndex(2);
                    TextWrapped("%s", e.msg.c_str());
                }
            }
            EndTable();
        }
    }

    if(!project.active())
        EndDisabled();
}

void player_shutdown()
{
    display_texture.reset();
}
