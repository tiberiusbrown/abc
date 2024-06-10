#pragma once

#include <filesystem>
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
extern ImGuiID dockid_welcome;
extern ImFont* font_normal;
extern ImFont* font_h1;
extern ImFont* font_h2;
extern ImFont* font_h3;
extern bool player_active;
extern std::string const main_name;
extern TextEditor asm_editor;
extern bool show_sys;
extern uint64_t save_tick;

void dirty_save();

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

struct cached_file_t
{
    std::filesystem::path path;
    bool is_dir;
    std::vector<cached_file_t> children;
    bool operator<(cached_file_t const& f) const
    {
        return std::tie(is_dir, path) < std::tie(f.is_dir, f.path);
    }
};

struct project_t
{
    cached_file_t root;
    std::map<std::string, std::vector<ards::error_t>> errors;
    std::vector<uint8_t> binary;
    std::unordered_map<std::string, std::string> arduboy_directives;

    size_t data_size;
    size_t code_size;
    size_t debug_size;
    size_t globals_size;
    size_t save_size;
    int shades;
    bool has_save() const { return save_size != 0; };

    void update_cached_files();
    bool active() { return !root.path.empty(); }
};
extern project_t project;

struct open_file_t
{
    std::filesystem::path path;
    std::filesystem::path rel_path;
    bool dirty;
    bool open;
    bool confirming_close;
    open_file_t(std::string const& filename);
    void save();
    void window();
    std::string read_as_string() const;
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
bool open_file(std::filesystem::path const& p);
void close_file(std::filesystem::path const& p);
void update_cached_files();
void dock_next_window_to_welcome();

// ide_compile.cpp
bool compile_all();

// ide_export.cpp
extern const unsigned char INTERP_BUILDS_ZIP[];
extern const size_t INTERP_BUILDS_ZIP_SIZE;
void export_menu_items();
std::vector<uint8_t> extract_interp_build(char const* id);
void export_arduboy(
    std::string const& filename,
    std::vector<uint8_t> const& binary, bool has_save, bool universal, int shades,
    std::unordered_map<std::string, std::string> const& fd);

// ide_import.cpp
void import_menu_item();

// ide_welcome.cpp
void project_welcome();

// ide_project.cpp
void project_window_contents();

// ide_player.cpp
void player_window_contents(uint64_t dt);
void player_shutdown();
void player_run();

// ide_asm.cpp
void set_asm(std::string const& t);

// ide_sys.cpp
void ide_system_reference();

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
