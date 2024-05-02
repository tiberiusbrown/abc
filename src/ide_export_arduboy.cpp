#include <miniz.h>
#include <miniz_zip.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#ifdef _MSC_VER
// suppress strstream deprecation warnings
#pragma warning(disable:4996)
#endif

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <strstream>
#include <unordered_map>
#include <vector>

#include <absim.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#ifdef __EMSCRIPTEN__
#include <emscripten_browser_file.h>
#endif

extern const unsigned char INTERP_BUILDS_ZIP[];
extern const size_t INTERP_BUILDS_ZIP_SIZE;
#include <interp_builds.hpp>

size_t zip_write_data(
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

std::vector<uint8_t> extract_interp_build(char const* id)
{
    std::vector<uint8_t> r;
    mz_zip_archive zip{};
    if(!mz_zip_reader_init_mem(&zip, INTERP_BUILDS_ZIP, INTERP_BUILDS_ZIP_SIZE, 0))
        return {};
    std::string fname = std::string("interp_builds/build_") + id + ".hex";
    int f = mz_zip_reader_locate_file(&zip, fname.c_str(), nullptr, 0);
    if(f < 0) return {};
    mz_zip_archive_file_stat stat{};
    if(!mz_zip_reader_file_stat(&zip, (mz_uint)f, &stat))
        return {};
    r.resize(stat.m_uncomp_size);
    if(!mz_zip_reader_extract_to_mem(&zip, f, r.data(), r.size(), 0))
        return {};
    (void)mz_zip_reader_end(&zip);
    return r;
}

static std::vector<std::string> interp_build_ids()
{
    std::vector<std::string> r;
    mz_zip_archive zip{};
    if(!mz_zip_reader_init_mem(&zip, INTERP_BUILDS_ZIP, INTERP_BUILDS_ZIP_SIZE, 0))
        return {};
    auto n = mz_zip_reader_get_num_files(&zip);
    char fname[256];
    r.push_back("ArduboyFX");
    for(mz_uint f = 0; f < n; ++f)
    {
        if(mz_zip_reader_is_file_a_directory(&zip, f))
            continue;
        (void)mz_zip_reader_get_filename(&zip, f, fname, sizeof(fname));
        std::string fstr(fname);
        if(fstr.size() < 24)
            continue;
        fstr = fstr.substr(20, fstr.size() - 24);
        if(fstr != "ArduboyFX")
            r.push_back(fstr);
    }
    return r;
}

void export_interpreter_hex(std::string const& filename)
{
    auto r = extract_interp_build("ArduboyFX");
    auto const* data = r.data();
    size_t size = r.size();
#ifdef __EMSCRIPTEN__
    emscripten_browser_file::download(
        filename.c_str(), "application/octet-stream",
        data, size);
#else
    std::ofstream f(filename.c_str(), std::ios::out | std::ios::binary);
    if(!f) return;
    f.write((char const*)data, size);
#endif
}

void export_arduboy(
    std::string const& filename,
    std::vector<uint8_t> const& binary, bool has_save, bool universal,
    std::unordered_map<std::string, std::string> const& fd)
{
    std::vector<std::string> ids;
    if(universal)
        ids = interp_build_ids();
    else
        ids = { "ArduboyFX" };

    std::vector<uint8_t> screenshot_png;
    std::string info_json;
    {
        rapidjson::StringBuffer s;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> w(s);
        w.StartObject();

        w.Key("schemaVersion");
        w.String("3");
        w.Key("title");
        if(auto it = fd.find("title"); it != fd.end())
            w.String(it->second.c_str());
        else
            w.String("Untitled Arduboy Game");
        if(auto it = fd.find("description"); it != fd.end())
        {
            w.Key("description");
            w.String(it->second.c_str());
        }
        w.Key("author");
        if(auto it = fd.find("author"); it != fd.end())
            w.String(it->second.c_str());
        else
            w.String("Unknown Author");
        w.Key("version");
        if(auto it = fd.find("version"); it != fd.end())
            w.String(it->second.c_str());
        else
            w.String("1.0");
        for(char const* id : {
            "date", "genre", "publisher", "idea", "code", "art", "sound",
            "url", "sourceUrl", "email", "companion" })
        {
            if(auto it = fd.find(id); it != fd.end())
            {
                w.Key(id);
                w.String(it->second.c_str());
            }
        }

        // simulate game for 100 ms and extract screenshot
        do
        {
            auto a = std::make_unique<absim::arduboy_t>();
            {
                std::istrstream ss(
                    (char const*)binary.data(),
                    (int)binary.size());
                auto t = a->load_file("fxdata.bin", ss);
                if(!t.empty()) break;
            }
            {
                auto r = extract_interp_build("ArduboyFX");
                std::istrstream ss(
                    (char const*)r.data(),
                    (int)r.size());
                auto t = a->load_file("interp.hex", ss);
                if(!t.empty()) break;
            }
            a->display.enable_filter = false;
            constexpr uint64_t MS = 1000000000ull;
            a->advance(MS * 100);

            std::vector<uint8_t> idata;
            idata.resize(128 * 64 * 3);
            for(int i = 0, n = 0; i < 64; ++i)
                for(int j = 0; j < 128; ++j, ++n)
                {
                    idata[n * 3 + 0] = idata[n * 3 + 1] = idata[n * 3 + 2] =
                        (a->display.filtered_pixels[n] >= 128 ? 255 : 0);
                }

            int len = 0;
            unsigned char* pngd = stbi_write_png_to_mem(
                idata.data(),
                128 * 3, 128, 64, 3, &len);
            screenshot_png.resize((size_t)len);
            std::memcpy(screenshot_png.data(), pngd, screenshot_png.size());
            STBIW_FREE(pngd);
        } while(0);

        if(!screenshot_png.empty())
        {
            w.Key("screenshots");
            w.StartArray();
            w.StartObject();
            w.Key("filename");
            w.String("screenshot.png");
            w.EndObject();
            w.EndArray();
        }

        w.Key("binaries");
        w.StartArray();
        std::string title;
        if(auto it = fd.find("title"); it != fd.end())
            title = it->second;
        for(auto const& id : ids)
        {
            w.StartObject();
            w.Key("title");
            if(universal)
            {
                if(title.empty())
                    w.String(id.c_str());
                else
                    w.String((title + " (" + id + ")").c_str());
            }
            else
                w.String(title.c_str());
            w.Key("filename");
            w.String(("interp_" + id + ".hex").c_str());
            w.Key("flashdata");
            w.String("game.bin");
            if(has_save)
            {
                w.Key("flashsave");
                w.String("save.bin");
            }
            w.Key("device");
            w.String(id.c_str());
            w.EndObject();
        }
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

    for(auto const& id : ids)
    {
        auto r = extract_interp_build(id.c_str());
        mz_zip_writer_add_mem(
            &zip, ("interp_" + id + ".hex").c_str(),
            r.data(), r.size(),
            compression);
    }

    mz_zip_writer_add_mem(
        &zip, "game.bin",
        binary.data(), binary.size() - (has_save ? 4096 : 0),
        compression);

    if(!screenshot_png.empty())
    {
        mz_zip_writer_add_mem(
            &zip, "screenshot.png",
            screenshot_png.data(), screenshot_png.size(),
            compression);
    }

    if(has_save)
    {
        uint8_t byte = 0xff;
        mz_zip_writer_add_mem(
            &zip, "save.bin",
            &byte, 1,
            compression);
    }

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
