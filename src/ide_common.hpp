#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <absim.hpp>
#include <TextEditor.h>

constexpr float DEFAULT_FONT_SIZE = 16.f;

constexpr uint32_t AUDIO_FREQ = 16000000 / absim::atmega32u4_t::SOUND_CYCLES;

using texture_t = void*;

extern std::unique_ptr<absim::arduboy_t> arduboy;
extern float pixel_ratio;

void init();
void shutdown();
void frame_logic();
void imgui_content();
bool update_pixel_ratio();
void define_font();
void rebuild_fonts();
void rescale_style();

// ide_compile.cpp
void compile_all();

// platform-specific functionality
void platform_destroy_texture(texture_t t);
texture_t platform_create_texture(int w, int h);
void platform_update_texture(texture_t t, void const* data, size_t n);
void platform_texture_scale_linear(texture_t t);
void platform_texture_scale_nearest(texture_t t);
void platform_set_clipboard_text(char const* str);
void platform_send_sound();
uint64_t platform_get_ms_dt();
float platform_pixel_ratio();
void platform_destroy_fonts_texture();
void platform_create_fonts_texture();
void platform_open_url(char const* url);
void platform_toggle_fullscreen();

// editor
struct editor_t
{
    TextEditor editor;
    std::string title;
    bool open;

    explicit editor_t(std::string const& name);
    editor_t() : editor_t("<unknown>") {}
    void update();
};
extern std::unordered_map<std::string, editor_t> editors;
