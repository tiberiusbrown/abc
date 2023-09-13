#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <absim.hpp>
#include <TextEditor.h>
#include <ards_error.hpp>

constexpr float DEFAULT_FONT_SIZE = 16.f;

constexpr uint32_t AUDIO_FREQ = 16000000 / absim::atmega32u4_t::SOUND_CYCLES;

const std::string INFO_FILENAME = "abc.json";
extern char const* const abc_version;
extern std::unique_ptr<absim::arduboy_t> arduboy;
extern float pixel_ratio;
extern ImGuiID dockspace_id;
extern ImGuiID dockid_project;
extern bool player_active;
extern std::string const main_name;

// ide_texture.cpp
struct texture_t
{
    int w, h;
    uintptr_t id;
    ImTextureID imgui_id() const { return reinterpret_cast<ImTextureID>(id); }
    void update(std::vector<uint8_t> const& rgba);
    texture_t(int w, int h);
    ~texture_t();
};

struct project_info_t
{
    std::string name;
    std::string author;
    std::string desc;
};

struct project_file_t
{
    std::string filename;
    std::vector<uint8_t> content;
    std::string content_as_string() const;
    void set_content(std::string const& s);
};
struct project_t
{
    project_info_t info;
    std::map<std::string, std::shared_ptr<project_file_t>> files;
    std::map<std::string, std::vector<ards::error_t>> errors;
    std::vector<uint8_t> binary;

    std::shared_ptr<project_file_t> get_file(std::string const& filename);
};
extern project_t project;

struct open_file_t
{
    std::weak_ptr<project_file_t> file;
    bool dirty;
    bool open;
    open_file_t(std::string const& filename)
        : file(project.get_file(filename))
        , dirty(false)
        , open(true)
    {}
    void save();
    void window();
    std::string filename() const;
    virtual std::string window_title();
    virtual std::string window_id();
    virtual ~open_file_t() {}
protected:
    virtual void save_impl() {}
    virtual void window_contents() = 0;
};
extern std::unordered_map<std::string, std::unique_ptr<open_file_t>> open_files;

// ide_common.cpp
void init();
void shutdown();
void frame_logic();
void imgui_content();
bool update_pixel_ratio();
void define_font();
void rebuild_fonts();
void rescale_style();
void make_tab_visible(std::string const& window_name);

// ide_compile.cpp
bool compile_all();

// ide_export.cpp
void export_menu_items();

// ide_import.cpp
void import_menu_item();

// ide_new_project.cpp
void create_default_info_file();
void new_project();

// ide_project.cpp
void project_window_contents();

// ide_player.cpp
void player_window_contents(uint64_t dt);
void player_shutdown();
void player_run();

// platform-specific functionality
void platform_set_clipboard_text(char const* str);
void platform_send_sound();
uint64_t platform_get_ms_dt();
float platform_pixel_ratio();
void platform_destroy_fonts_texture();
void platform_create_fonts_texture();
void platform_open_url(char const* url);
void platform_toggle_fullscreen();

// ide_editor.cpp
std::unique_ptr<open_file_t> create_code_file(std::string const& filename);

// ide_project_info.cpp
std::unique_ptr<open_file_t> create_project_info_file(std::string const& filename);

// ide_hexdata.cpp
extern const unsigned char VM_HEX_ARDUBOYFX[];
extern const size_t VM_HEX_ARDUBOYFX_SIZE;
