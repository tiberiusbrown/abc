#include "ide_common.hpp"

#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

struct open_project_info_file_t : public open_file_t
{
    std::string name;
    std::string author;
    std::string desc;

    open_project_info_file_t();

    void load();

    std::string window_title() override
    {
        std::string title = "Project Info";
        if(dirty) title += "*";
        return title;
    }
    void save_impl() override;
    void window_contents() override;
};

std::unique_ptr<open_file_t> create_project_info_file(std::string const& filename)
{
    return std::make_unique<open_project_info_file_t>();
}

open_project_info_file_t::open_project_info_file_t()
    : open_file_t(INFO_FILENAME)
{
    load();
}

void open_project_info_file_t::load()
{
    name.clear();
    author.clear();
    desc.clear();

    auto f = file.lock();
    if(!f) return;
    rapidjson::Document doc;
    rapidjson::ParseResult ok = doc.Parse<
        rapidjson::kParseCommentsFlag |
        rapidjson::kParseTrailingCommasFlag |
        0>((char*)f->content.data(), f->content.size());
    if(!ok) return;

    if(doc.HasMember("name"))
    {
        auto const& j = doc["name"];
        if(j.IsString())
            name = j.GetString();
    }
    if(doc.HasMember("author"))
    {
        auto const& j = doc["author"];
        if(j.IsString())
            author = j.GetString();
    }
    if(doc.HasMember("desc"))
    {
        auto const& j = doc["desc"];
        if(j.IsString())
            desc = j.GetString();
    }
}

void open_project_info_file_t::window_contents()
{
    using namespace ImGui;

    PushItemWidth(-1.f);

    TextUnformatted("       Name:");
    SameLine();
    if(InputText("##Name", &name))
        dirty = true;

    TextUnformatted("     Author:");
    SameLine();
    if(InputText("##Author", &author))
        dirty = true;

    PopItemWidth();

    TextUnformatted("Description:");
    SameLine();
    if(InputTextMultiline("##Desc", &desc, { -1, 0 }))
        dirty = true;
}

void open_project_info_file_t::save_impl()
{
    auto f = file.lock();
    if(!f) return;

    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> w(s);

    w.StartObject();
    w.Key("name");
    w.String(name.c_str());
    w.Key("author");
    w.String(author.c_str());
    w.Key("desc");
    w.String(desc.c_str());
    w.EndObject();

    f->set_content(s.GetString());
}
