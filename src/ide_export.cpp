#include "ide_common.hpp"

#include <fstream>
#include <cassert>

#include <imgui.h>

#ifdef __EMSCRIPTEN__
#include <emscripten_browser_file.h>
#else
#include <nfd.hpp>
#endif

#include <miniz.h>
#include <miniz_zip.h>

size_t zip_write_data(
    void* data, mz_uint64 file_ofs, const void* pBuf, size_t n);
void export_arduboy(
    std::string const& filename,
    std::vector<uint8_t> const& binary, bool has_save, bool mini,
    std::unordered_map<std::string, std::string> const& fd);
void export_interpreter_hex(
    std::string const& filename, bool mini);

static void export_compiled_fxdata(std::string const& filename)
{
#ifdef __EMSCRIPTEN__
    emscripten_browser_file::download(
        filename.c_str(), "application/octet-stream",
        project.binary.data(), project.binary.size());
#else
    std::ofstream f(filename.c_str(), std::ios::out | std::ios::binary);
    if(!f) return;
    f.write((char const*)project.binary.data(), project.binary.size());
#endif
}

static void export_fxdata()
{
    if(!compile_all())
        return;
    std::string filename;

#ifdef __EMSCRIPTEN__
    filename = "fxdata.bin";
#else
    NFD::UniquePath path;
    nfdfilteritem_t filterItem[1] = { { "FX Data", "bin" } };
    auto result = NFD::SaveDialog(path, filterItem, 1, nullptr, "fxdata.bin");
    if(result != NFD_OKAY)
        return;
    filename = path.get();
#endif

    export_compiled_fxdata(filename);
}

#ifdef __EMSCRIPTEN__
static void export_zip()
{
    constexpr mz_uint compression = (mz_uint)MZ_DEFAULT_COMPRESSION;
    std::vector<uint8_t> zipdata;
    mz_zip_archive zip{};
    zip.m_pWrite = zip_write_data;
    zip.m_pIO_opaque = &zipdata;

    mz_zip_writer_init(&zip, 0);

    for(auto const& e : std::filesystem::recursive_directory_iterator(project.root.path))
    {
        mz_zip_writer_add_file(
            &zip,
            (e.path().lexically_relative(project.root.path)).string().c_str(),
            e.path().string().c_str(),
            nullptr, 0, compression);
    }

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);

#ifdef __EMSCRIPTEN__
    emscripten_browser_file::download(
        "project.zip", "application/x-zip",
        zipdata.data(), zipdata.size());
#else
    std::ofstream f("project.zip", std::ios::out | std::ios::binary);
    if(!f.fail())
    {
        f.write((char const*)zipdata.data(), zipdata.size());
        f.close();
    }
#endif
}
#endif

static void export_arduboy_file_menu_clicked(bool mini)
{
    std::string filename;

#ifdef __EMSCRIPTEN__
    filename = "game.arduboy";
#else
    NFD::UniquePath path;
    nfdfilteritem_t filterItem[1] = { { "Arduboy Game", "arduboy" } };
    auto result = NFD::SaveDialog(path, filterItem, 1, nullptr, "game.arduboy");
    if(result != NFD_OKAY)
        return;
    filename = path.get();
#endif
    export_arduboy(
        filename, project.binary,
        project.has_save(), mini,
        project.arduboy_directives);
}

static void export_hex(bool mini)
{
    std::string filename;
    filename = mini ? "abc_interpreter_mini.hex" : "abc_interpreter.hex";

#ifndef __EMSCRIPTEN__
    NFD::UniquePath path;
    nfdfilteritem_t filterItem[1] = { { "Arduboy Game", "hex" } };
    auto result = NFD::SaveDialog(path, filterItem, 1, nullptr, filename.c_str());
    if(result != NFD_OKAY)
        return;
    filename = path.get();
#endif

    export_interpreter_hex(filename, mini);
}

void export_menu_items()
{
    using namespace ImGui;
#ifdef __EMSCRIPTEN__
    if(MenuItem("Download project (.zip)..."))
        export_zip();
    Separator();
#endif
    if(MenuItem("Export interpreter hex (FX)..."))
        export_hex(false);
    if(MenuItem("Export interpreter hex (Mini)..."))
        export_hex(true);
    if(MenuItem("Export development FX data..."))
        export_fxdata();
    Separator();
    if(MenuItem("Export .arduboy file (FX)...") && compile_all())
        export_arduboy_file_menu_clicked(false);
    if(MenuItem("Export .arduboy file (Mini)...") && compile_all())
        export_arduboy_file_menu_clicked(true);
}
