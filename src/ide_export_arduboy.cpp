#include <miniz.h>
#include <miniz_zip.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

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

extern const unsigned char VM_HEX_ARDUBOYFX[];
extern const size_t VM_HEX_ARDUBOYFX_SIZE;
#include <vm_hex_arduboyfx.hpp>

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

void export_arduboy(
    std::string const& filename,
    std::vector<uint8_t> const& binary, bool has_save,
    std::unordered_map<std::string, std::string> const& fd)
{
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
                std::istrstream ss(
                    (char const*)VM_HEX_ARDUBOYFX,
                    (int)VM_HEX_ARDUBOYFX_SIZE);
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
        w.StartObject();
        if(auto it = fd.find("title"); it != fd.end())
        {
            w.Key("title");
            w.String(it->second.c_str());
        }
        w.Key("filename");
        w.String("interp.hex");
        w.Key("flashdata");
        w.String("game.bin");
        if(has_save)
        {
            w.Key("flashsave");
            w.String("save.bin");
        }
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
