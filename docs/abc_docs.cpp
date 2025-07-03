#include <abc_assembler.hpp>
#include <abc_compiler.hpp>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <tuple>

#include <ctype.h>

#include <all_fonts.hpp>
#include <stb_truetype.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <stdio.h>

constexpr uint8_t FONT_HEADER_PER_CHAR = 7;
constexpr uint8_t FONT_HEADER_OFFSET = 1;
constexpr uint16_t FONT_HEADER_CHAR_BYTES =
    FONT_HEADER_PER_CHAR * 256;
constexpr uint16_t FONT_HEADER_BYTES =
    FONT_HEADER_CHAR_BYTES + FONT_HEADER_OFFSET;

static void draw_str(
    std::vector<uint8_t> const& font,
    std::vector<uint8_t>& buf,
    int w, int h,
    int x, int y,
    char const* str)
{
    // gray line
    for(int c = 0; c < w; ++c)
        buf[y * w + c] = 0x40;

    for(uint8_t cc; (cc = (uint8_t)*str) != '\0'; ++str)
    {
        int sw = font[int(cc) * FONT_HEADER_PER_CHAR + 1];
        int sh = font[int(cc) * FONT_HEADER_PER_CHAR + 6];
        int offset = font[int(cc) * FONT_HEADER_PER_CHAR + 4];
        offset += font[int(cc) * FONT_HEADER_PER_CHAR + 5] * 256;
        for(int r = 0; r < sh; ++r)
        {
            int tr = r + y;
            tr += (int8_t)font[int(cc) * FONT_HEADER_PER_CHAR + 3]; // yoff
            int tp = r / 8;
            if(tr < 0) continue;
            if(tr >= h) break;
            for(int c = 0; c < sw; ++c)
            {
                int tc = c + x;
                tc += (int8_t)font[int(cc) * FONT_HEADER_PER_CHAR + 2]; // xoff

                if(tc < 0)
                    continue;
                if(tc >= w)
                    break;
                uint8_t& t = buf[tr * w + tc];
                if((font[FONT_HEADER_BYTES + offset + sw * tp + c] >> (r & 7)) & 1)
                    t = 0xff;
            }
        }
        x += font[int(cc) * FONT_HEADER_PER_CHAR + 0];
    }
}

static void print_sysfunc_decl(
    FILE* f,
    std::string const& k,
    abc::sysfunc_t v,
    abc::sysfunc_info_t const& info)
{
    auto const& decl = info.decl;
    fprintf(f, "\n%s $%s(", abc::type_name(decl.return_type).c_str(), k.c_str());
    for(size_t i = 0; i < decl.arg_types.size(); ++i)
    {
        if(i != 0) fprintf(f, ", ");
        fprintf(f, "%s %s", abc::type_name(decl.arg_types[i]).c_str(), decl.arg_names[i].c_str());
    }
    if(abc::sysfunc_is_format(v))
        fprintf(f, ", ...");
    fprintf(f, ");");
}

static std::string tolabel(std::string const& t)
{
    std::string r(t);
    for(auto& c : r)
        c = char(isalnum(c) || c == '_' ? tolower(c) : '-');
    return r;
}

static void print_sysfunc(
    FILE* f,
    std::string const& k,
    abc::sysfunc_t v,
    abc::sysfunc_info_t const& info)
{
    auto const& decl = info.decl;
    fprintf(f, "## `$%s`", k.c_str());
    //fprintf(f, "\n\n### Declaration");
    fprintf(f, "\n\n```c");
    print_sysfunc_decl(f, k, v, info);
    if(auto it2 = abc::sys_overloads.find(k); it2 != abc::sys_overloads.end())
    {
        for(auto const& ov : it2->second)
        {
            auto jt = abc::sys_names.find(ov);
            auto kt = abc::sysfunc_decls.find(jt->second);
            print_sysfunc_decl(f, k, jt->second, kt->second);
        }
    }
    fprintf(f, "\n```");
    //fprintf(f, "\n\n### Description");
    fprintf(f, "\n\n%s", info.desc.c_str());
    if(!info.arg_descs.empty())
    {
        fprintf(f, "\n\n| Parameter | Description |");
        fprintf(f, "\n| :-- | :-- |");
        for(size_t i = 0; i < info.arg_descs.size(); ++i)
        {
            fprintf(f, "\n| **%s** | %s |",
                decl.arg_names[i].c_str(), info.arg_descs[i].c_str());
        }
    }
    if(!info.return_desc.empty())
    {
        fprintf(f, "\n\n**Returns:** %s", info.return_desc.c_str());
    }
    fprintf(f, "\n\n");
}

int abc_docs()
{
    std::map<std::string, abc::sysfunc_t> const sys_names(
        abc::sys_names.begin(), abc::sys_names.end());

    FILE* f;
    
    f = fopen(DOCS_DIR "/system.md", "w");
    if(!f) return 1;

#if 0
    fprintf(f, "# Predefined Constants\n\n```c\n");
    for(auto const& c : abc::builtin_constexprs)
    {
        fprintf(f, "%s %s;\n", abc::type_name(c.type).c_str(), c.name.c_str());
    }
    fprintf(f, "```\n\n");
#endif

    // table of contents
    for(auto const& cat : abc::sysfunc_cats)
    {
        fprintf(f, "- [%s](#%s)\n", cat.c_str(), tolabel(cat).c_str());
        for(auto const& [k, v] : sys_names)
        {
            bool skip = false;
            for(auto const& [k2, v2] : abc::sys_overloads)
                for(auto const& v3 : v2)
                    if(v3 == k) skip = true;
            if(skip) continue;
            auto it = abc::sysfunc_decls.find(v);
            if(it == abc::sysfunc_decls.end()) continue;
            if(it->second.category != cat) continue;
            fprintf(f, "  - [`$%s`](#%s)\n", k.c_str(), tolabel(k).c_str());
        }
    }
    fprintf(f, "\n");

    for(auto const& cat : abc::sysfunc_cats)
    {
        fprintf(f, "# %s\n\n", cat.c_str());
        for(auto const& [k, v] : sys_names)
        {
            bool skip = false;
            for(auto const& [k2, v2] : abc::sys_overloads)
                for(auto const& v3 : v2)
                    if(v3 == k) skip = true;
            if(skip) continue;
            auto it = abc::sysfunc_decls.find(v);
            if(it == abc::sysfunc_decls.end()) continue;
            if(it->second.category != cat) continue;
            print_sysfunc(f, k, v, it->second);
        }
        fprintf(f, "\n");
    }
    fprintf(f, "\n");

    fclose(f);

    f = fopen(DOCS_DIR "/builtin_fonts.md", "w");
    if(!f) return 1;

    fprintf(f, "# Built-in Font Assets\n");
    fprintf(f, "| Predefined Variable | Ascent | Line Height | Bytes | Preview |\n");
    fprintf(f, "|---|---|---|---|---|\n");

    std::vector<std::tuple<int, int, std::string, std::vector<uint8_t>>> fonts;

    for(auto const& font : ALL_FONTS)
    {
        std::vector<uint8_t> data;
        data.resize(font.size);
        memcpy(data.data(), font.data, data.size());
        fonts.push_back({ (int)font.pixels, data[FONT_HEADER_CHAR_BYTES + 0], font.name, data });
    }

    std::sort(fonts.begin(), fonts.end());

    for(auto const& font : fonts)
    {
        auto const& data = std::get<3>(font);
        fprintf(f, "| `%s` | %d | %d | %d |",
            std::get<2>(font).c_str(),
            std::get<0>(font),
            std::get<1>(font),
            (int)data.size());
        static char const STR_LOWER[] = "the quick brown fox jumps over the lazy dog";
        static char const STR_UPPER[] = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";
        static char const STR_SYM[] = "0123456789,.:?![/](\\){|}+-=<>@#$%^&*';\"";
        int h = data[FONT_HEADER_CHAR_BYTES + 0] * 14 / 4;
        int w = 2;
        for(char c : STR_UPPER)
            w += data[int(uint8_t(c)) * FONT_HEADER_PER_CHAR + 0];
        if(w > 512)
            w = 512;
        std::vector<uint8_t> buf;
        buf.resize(w * h);
        draw_str(data, buf, w, h, 1, data[FONT_HEADER_CHAR_BYTES + 0] * 1, STR_UPPER);
        draw_str(data, buf, w, h, 1, data[FONT_HEADER_CHAR_BYTES + 0] * 2, STR_LOWER);
        draw_str(data, buf, w, h, 1, data[FONT_HEADER_CHAR_BYTES + 0] * 3, STR_SYM);
        stbi_write_png(
            (std::string(DOCS_DIR) + "/font_images/" + std::get<2>(font) + ".png").c_str(),
            w, h, 1, buf.data(), w);
        fprintf(f, " ![%s](font_images/%s.png) |\n", std::get<2>(font).c_str(), std::get<2>(font).c_str());
    }

    fclose(f);

    return 0;
}
