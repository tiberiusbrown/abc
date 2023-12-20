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
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

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

static size_t zip_write_data(
    void* data, mz_uint64 file_ofs, const void* pBuf, size_t n)
{
    auto& d = *(std::vector<uint8_t>*)data;
    assert(file_ofs == d.size());
    size_t bytes = file_ofs + n;
    if(bytes > d.size())
        d.resize(bytes);
    memcpy(d.data() + file_ofs, pBuf, n);
    return n;
}

static void export_arduboy()
{
    if(!compile_all())
        return;
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

    std::string info_json;
    {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> w(s);
        w.StartObject();

        w.Key("schemaVersion");
        w.String("3");
        w.Key("title");
        //w.String(project.info.name.c_str());
        w.String("TODO");
        w.Key("description");
        //w.String(project.info.desc.c_str());
        w.String("TODO");
        w.Key("author");
        //w.String(project.info.author.c_str());
        w.String("TODO");

        w.Key("binaries");
        w.StartArray();
        w.StartObject();
        w.Key("title");
        //w.String(project.info.name.c_str());
        w.String("TODO");
        w.Key("filename");
        w.String("interp.hex");
        w.Key("flashdata");
        w.String("game.bin");
        w.Key("device");
        w.String("ArduboyFX");
        w.EndObject();
        w.EndArray();

        w.EndObject();
        info_json = s.GetString();
    }

    std::vector<uint8_t> zipdata;
    mz_zip_archive zip{};
    zip.m_pWrite = zip_write_data;
    zip.m_pIO_opaque = &zipdata;

    constexpr mz_uint compression = (mz_uint)MZ_DEFAULT_COMPRESSION;

    mz_zip_writer_init(&zip, 0);

    mz_zip_writer_add_mem(
        &zip, "info.json",
        info_json.data(), info_json.size(),
        compression);

    mz_zip_writer_add_mem(
        &zip, "interp.hex",
        VM_HEX_ARDUBOYFX, VM_HEX_ARDUBOYFX_SIZE,
        compression);

    mz_zip_writer_add_mem(
        &zip, "game.bin",
        project.binary.data(), project.binary.size(),
        compression);

#if 0
    // add project files
    mz_zip_writer_add_mem(&zip, "abc/", nullptr, 0, compression);
    for(auto const& e : std::filesystem::recursive_directory_iterator(project.root.path))
    {
        mz_zip_writer_add_file(
            &zip,
            ("abc" / e.path().lexically_relative(project.root.path)).string().c_str(),
            e.path().string().c_str(),
            nullptr, 0, compression);
    }
    //for(auto const& [n, f] : project.files)
    //{
    //    mz_zip_writer_add_mem(
    //        &zip, ("abc/" + n).c_str(),
    //        f->content.data(), f->content.size(),
    //        compression);
    //}
#endif

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);
    
#ifdef __EMSCRIPTEN__
    emscripten_browser_file::download(
        filename.c_str(), "application/octet-stream",
        zipdata.data(), zipdata.size());
#else
    std::ofstream f(filename.c_str(), std::ios::out | std::ios::binary);
    if(!f) return;
    f.write((char const*)zipdata.data(), zipdata.size());
#endif
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

    mz_zip_writer_finalize_archive(&zip);
    mz_zip_writer_end(&zip);

    for(auto const& e : std::filesystem::recursive_directory_iterator(project.root.path))
    {
        mz_zip_writer_add_file(
            &zip,
            (e.path().lexically_relative(project.root.path)).string().c_str(),
            e.path().string().c_str(),
            nullptr, 0, compression);
    }

    emscripten_browser_file::download(
        "project.zip", "application/x-zip",
        zipdata.data(), zipdata.size());
}
#endif

void export_menu_items()
{
    using namespace ImGui;
#ifdef __EMSCRIPTEN__
    if(MenuItem("Download project (.zip)..."))
        export_zip();
    Separator();
#endif
    if(MenuItem("Export FX data..."))
        export_fxdata();
    if(MenuItem("Export .arduboy file..."))
        export_arduboy();
}
