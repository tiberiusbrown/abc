#define _SILENCE_CXX17_STRSTREAM_DEPRECATION_WARNING

#include "ide_common.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <numeric>
#include <strstream>
#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>

#include <stb_image_write.h>

#include "font_icons.hpp"

static std::unique_ptr<texture_t> display_texture;
bool player_active;
static bool gif_recording = false;

static std::vector<uint8_t> screenshot_png;

static void take_screenshot()
{
    std::vector<uint8_t> idata;
    idata.resize(128 * 64 * 3);
    for(int i = 0, n = 0; i < 64; ++i)
        for(int j = 0; j < 128; ++j, ++n)
        {
            idata[n * 3 + 0] = idata[n * 3 + 1] = idata[n * 3 + 2] =
                arduboy->display.filtered_pixels[n];
        }

    screenshot_png.clear();
    stbi_write_png_to_func(
        [](void* ctx, void* data, int size)
        {
            (void)ctx;
            screenshot_png.reserve(screenshot_png.size() + size);
            for(int i = 0; i < size; ++i)
                screenshot_png.push_back(((uint8_t*)data)[i]);
        },
        nullptr, 128, 64, 3, idata.data(), 128 * 3);
}

static void toggle_gif()
{
    gif_recording = !gif_recording;
}

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
    std::string err;
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "s%d_ArduboyFX", project.shades);
        auto r = extract_interp_build(buf);
        std::istrstream ss((char const*)r.data(), (int)r.size());
        err = arduboy->load_file("vm.hex", ss);
        assert(err.empty());
    }
    if(!err.empty())
        return;
    {
        std::istrstream ss(
            (char const*)project.binary.data(),
            (int)project.binary.size());
        err = arduboy->load_file("fxdata.bin", ss);
    }
    arduboy->paused = true;
    if(!err.empty())
        return;
    arduboy->paused = false;
    arduboy->profiler_enabled = true;
    player_active = true;
    if(gif_recording)
        toggle_gif();
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

    if(display_texture && arduboy)
    {
        if(player_active && arduboy->cpu.decoded)
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
        if(player_active && arduboy->cpu.decoded)
        {
            ImVec2 a = GetCursorScreenPos();
            Image(
                display_texture->imgui_id(),
                size,
                { 0, 0 }, { 1, 1 },
                { 1, 1, 1, 1 },
                COLOR);
            if(gif_recording)
            {
                auto* d = GetWindowDrawList();
                d->AddRectFilled(
                    { a.x +  5.f * z, a.y +  5.f * z },
                    { a.x + 15.f * z, a.y + 15.f * z },
                    IM_COL32(255, 0, 0, 128)
                );
            }
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
        200.f * pixel_ratio,
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
            if(gif_recording)
                toggle_gif();
        }
    }
    else
    {
        if(Button(ICON_FA_PLAY_CIRCLE " Debug (F5)", button_size))
            player_run();
    }

    if(0 && player_active)
    {
        SetCursorPosX((GetWindowSize().x - button_size.x) * 0.5f);
        if(Button(ICON_FA_IMAGE " Take Screenshot (F2)", button_size) ||
            ImGui::IsKeyPressed(ImGuiKey_F2))
            take_screenshot();
        SetCursorPosX((GetWindowSize().x - button_size.x) * 0.5f);
        if(!gif_recording && (
            Button(ICON_FA_IMAGES " Record GIF (F3)", button_size) ||
            ImGui::IsKeyPressed(ImGuiKey_F3)))
            toggle_gif();
        else if(gif_recording && (
            Button(ICON_FA_STOP_CIRCLE " Stop Recording (F3)", button_size) ||
            ImGui::IsKeyPressed(ImGuiKey_F3)))
            toggle_gif();
    }
    

    if(player_active)
    {
        Separator();

        float fps = 0.f;
        if(arduboy->prev_frame_cycles != 0)
        {
            static std::array<float, 16> fps_queue{};
            static size_t fps_i = 0;
            fps = 16e6f / float(arduboy->prev_frame_cycles);
            fps_queue[fps_i] = fps;
            fps_i = (fps_i + 1) % fps_queue.size();
            fps = std::accumulate(fps_queue.begin(), fps_queue.end(), 0.f) * (1.f / fps_queue.size());
        }
        ImGui::Text("FPS:       %d", int(fps + 0.5f));

        float usage = 0.f;
        size_t i;
        constexpr size_t n = 16;
        auto const& d = arduboy->frame_cpu_usage;
        for(i = 0; i < n; ++i)
        {
            if(i >= d.size())
                break;
            usage += d[d.size() - i - 1];
        }
        usage /= n;
        bool red = usage > 0.999f;
        if(red) ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
        ImGui::Text("CPU Usage: %5.1f%%", usage * 100);
        if(red) ImGui::PopStyleColor();
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
        TextUnformatted("Some errors occurred during compilation.");
        PopStyleColor();
        if(Button("Copy errors to clipboard"))
        {
            std::stringstream ss;
            for(auto const& [f, errs] : project.errors)
            {
                for(auto const& e : errs)
                {
                    ss << f << ":" << (int)e.line_info.first << "  " << e.msg << "\n";
                }
            }
            platform_set_clipboard_text(ss.str().c_str());
        }
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
