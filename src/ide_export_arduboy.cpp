#include <miniz.h>
#include <miniz_zip.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <fstream>
#include <string>
#include <vector>

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
    std::vector<uint8_t> const& binary, bool has_save)
{

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
