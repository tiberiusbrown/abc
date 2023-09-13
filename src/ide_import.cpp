#include "ide_common.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten_browser_file.h>
#else
#include <nfd.hpp>
#endif

#include <fstream>
#include <cstdio>

#include <miniz.h>
#include <miniz_zip.h>
#include <rapidjson/document.h>

#ifdef __EMSCRIPTEN__
static void web_upload_handler(
    std::string const& filename,
    std::string const& mime_type,
    std::string_view buffer,
    void* data)
{
    (void)filename;
    (void)mime_type;
    *(std::vector<uint8_t>*)data = std::vector<uint8_t>(buffer.begin(), buffer.end());
}
#endif

static void import_arduboy_file()
{
    std::vector<uint8_t> data;
    {
#ifdef __EMSCRIPTEN__
        emscripten_browser_file::upload(".arduboy", web_upload_handler, &data);
#else
        NFD::UniquePath path;
        nfdfilteritem_t filterItem[1] = { { "Arduboy Game", "arduboy" } };
        auto result = NFD::OpenDialog(path, filterItem, 1, nullptr);
        if(result != NFD_OKAY)
            return;
        std::ifstream f(path.get(), std::ios::in | std::ios::binary);
        data = std::vector<uint8_t>(
            (std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
#endif
    }
    if(data.empty()) return;

    std::map<std::string, std::shared_ptr<project_file_t>> files;

    mz_zip_archive zip{};
    if(MZ_FALSE == mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0))
    {
        printf("ERROR: could not open zip file of size %u\n", (unsigned)data.size());
        goto error;
    }

    {
        auto n = mz_zip_reader_get_num_files(&zip);
        for(mz_uint f = 0; f < n; ++f)
        {
            mz_zip_archive_file_stat fstat{};
            if(MZ_FALSE == mz_zip_reader_file_stat(&zip, f, &fstat))
            {
                printf("ERROR: could not stat file index %u\n", (unsigned)f);
                goto error_close;
            }
            size_t fnamelen = strlen(fstat.m_filename);
            if(fstat.m_filename[fnamelen - 1] == '/')
                continue;
            if(0 != strncmp(fstat.m_filename, "abc/", 4))
                continue;
            std::string filename = &fstat.m_filename[4];
            auto pfile = std::make_shared<project_file_t>();
            pfile->filename = filename;
            pfile->content.resize((size_t)fstat.m_uncomp_size);
            if(MZ_FALSE == mz_zip_reader_extract_to_mem(
                &zip, f, pfile->content.data(), pfile->content.size(), 0))
            {
                printf("ERROR: could not extract file \"%s\"\n", fstat.m_filename);
                goto error_close;
            }
            files[filename] = std::move(pfile);
        }
    }

    mz_zip_reader_end(&zip);

    open_files.clear();
    project.files.swap(files);
    if(!project.files.count(INFO_FILENAME))
        create_default_info_file();
    {
        auto f = create_project_info_file(INFO_FILENAME);
        f->dirty = true;
        f->save();
    }
    if(project.files.count(main_name))
        open_files[main_name] = create_code_file(main_name);

    return;

error_close:
    mz_zip_reader_end(&zip);
error:
    // TODO
    return;
}

void import_menu_item()
{
    using namespace ImGui;
    if(MenuItem("Import .arduboy file..."))
        import_arduboy_file();
}
