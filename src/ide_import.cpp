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

#if 0
static void process_arduboy_file(std::vector<uint8_t> const& data)
{
    mz_zip_archive zip{};

    if(data.empty())
    {
        printf("Import .arduboy file: no data found\n");
        goto error;
    }

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
    if(project.files.count(main_name))
        open_files[main_name] = create_code_file(main_name);

    return;

error_close:
    mz_zip_reader_end(&zip);
error:
    // TODO
    return;
}
#endif

#if 0
#ifdef __EMSCRIPTEN__
static void web_upload_handler(
    std::string const& filename,
    std::string const& mime_type,
    std::string_view buffer,
    void* user)
{
    (void)filename;
    (void)mime_type;
    (void)user;
    auto data = std::vector<uint8_t>(buffer.begin(), buffer.end());
    process_arduboy_file(data);
}
#endif

static void import_arduboy_file()
{
    {
#ifdef __EMSCRIPTEN__
        emscripten_browser_file::upload(".arduboy", web_upload_handler, nullptr);
#else
        NFD::UniquePath path;
        nfdfilteritem_t filterItem[1] = { { "Arduboy Game", "arduboy" } };
        auto result = NFD::OpenDialog(path, filterItem, 1, nullptr);
        if(result != NFD_OKAY)
            return;
        std::ifstream f(path.get(), std::ios::in | std::ios::binary);
        std::vector<uint8_t> data = std::vector<uint8_t>(
            (std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());
        process_arduboy_file(data);
#endif
    }
}
#endif

static void try_open_main_abc()
{
    for(auto const& child : project.root.children)
    {
        if(child.is_dir) continue;
        if(child.path.filename() == "main.abc")
        {
            open_file(child.path);
            break;
        }
    }
}

#ifndef __EMSCRIPTEN__
static void open_directory()
{
    NFD::UniquePath path;
    auto result = NFD::PickFolder(path);
    if(result != NFD_OKAY) return;
    project = {};
    project.root.is_dir = true;
    project.root.path = std::filesystem::path(path.get()).lexically_normal();
    update_cached_files();
    open_files.clear();
    try_open_main_abc();
}
#endif

#ifdef __EMSCRIPTEN__
static void process_zip_file(std::vector<uint8_t> const& data)
{
    std::error_code ec;
    for(auto const& e : std::filesystem::directory_iterator(project.root.path))
        std::filesystem::remove_all(e.path(), ec);

    mz_zip_archive zip{};
    if(!mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0))
        return;

    mz_uint n = mz_zip_reader_get_num_files(&zip);
    for(mz_uint i = 0; i < n; ++i)
    {
        mz_zip_archive_file_stat stat{};
        if(!mz_zip_reader_file_stat(&zip, i, &stat))
            continue;
        auto path = project.root.path / stat.m_filename;
        if(stat.m_is_directory)
            std::filesystem::create_directories(path, ec);
        else
        {
            size_t size = 0;
            if(path.has_parent_path())
                std::filesystem::create_directories(path.parent_path(), ec);
            void* p = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
            if(!p) continue;
            std::ofstream f(path, std::ios::binary);
            f.write((char const*)p, size);
        }
    }

    mz_zip_reader_end(&zip);

    update_cached_files();
    dirty_save();

    open_files.clear();
    try_open_main_abc();
}
#endif

#ifdef __EMSCRIPTEN__
static void web_upload_handler(
    std::string const& filename,
    std::string const& mime_type,
    std::string_view buffer,
    void* user)
{
    (void)filename;
    (void)mime_type;
    (void)user;
    auto data = std::vector<uint8_t>(buffer.begin(), buffer.end());
    process_zip_file(data);
}

static void import_zip()
{
    emscripten_browser_file::upload(".zip", web_upload_handler, nullptr);
}
#endif

void import_menu_item()
{
    using namespace ImGui;
#ifndef __EMSCRIPTEN__
    if(MenuItem("Open directory..."))
        open_directory();
#else
    if(MenuItem("Import project (.zip)..."))
        import_zip();
#endif
#if 0
    if(MenuItem("Import .arduboy file..."))
        import_arduboy_file();
#endif
    Separator();
}
