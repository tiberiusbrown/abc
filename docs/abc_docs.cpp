#include <ards_assembler.hpp>
#include <ards_compiler.hpp>

#include <algorithm>
#include <map>
#include <sstream>
#include <string>
#include <tuple>

#include <all_fonts.hpp>
#include <stb_truetype.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <stdio.h>

static void draw_str(
    std::vector<uint8_t> const& font,
    std::vector<uint8_t>& buf,
    int w, int h,
    int x, int y,
    char const* str)
{
    int sw = font[513];
    int sh = font[514];
    int sb = (sh + 7) / 8 * sw;
    for(uint8_t cc; (cc = (uint8_t)*str) != '\0'; ++str)
    {
        for(int r = 0; r < sh; ++r)
        {
            int tr = r + y;
            int tp = r / 8;
            if(tr >= h) break;
            for(int c = 0; c < sw; ++c)
            {
                int tc = c + x + (int8_t)font[int(cc) * 2 + 0];
                if(tc >= w)
                    break;
                uint8_t& t = buf[tr * w + tc];
                if((font[518 + sb * cc + sw * tp + c] >> (r & 7)) & 1)
                    t = 0xff;
                else
                    t = 0x00;
            }
        }
        x += font[int(cc) * 2 + 1];
    }
}

int main()
{
    std::map<std::string, ards::sysfunc_t> const sys_names(
        ards::sys_names.begin(), ards::sys_names.end());

    FILE* f;
    
    f = fopen(DOCS_DIR "/system.md", "w");
    if(!f) return 1;

    fprintf(f, "# Predefined Constants\n\n```c\n");
    for(auto const& c : ards::builtin_constexprs)
    {
        fprintf(f, "%s %s;\n", ards::type_name(c.type).c_str(), c.name.c_str());
    }
    fprintf(f, "```\n\n");

    fprintf(f, "# System Calls\n\n```c\n");
    for(auto const& [k, v] : sys_names)
    {
        auto it = ards::sysfunc_decls.find(v);
        if(it == ards::sysfunc_decls.end()) continue;
        auto const& decl = it->second;
        fprintf(f, "%-5s $%s(", ards::type_name(decl.return_type).c_str(), k.c_str());
        for(size_t i = 0; i < decl.arg_types.size(); ++i)
        {
            if(i != 0) fprintf(f, ", ");
            fprintf(f, "%s %s", ards::type_name(decl.arg_types[i]).c_str(), decl.arg_names[i].c_str());
        }
        if(ards::sysfunc_is_format(v))
            fprintf(f, ", ...");
        fprintf(f, ");\n");
    }
    fprintf(f, "```\n\n");

    fclose(f);

    f = fopen(DOCS_DIR "/builtin_fonts.md", "w");
    if(!f) return 1;

    fprintf(f, "# Built-in Font Assets\n");
    fprintf(f, "| Predefined Variable | Line Height | Preview |\n");
    fprintf(f, "|---|---|---|\n");

    std::vector<std::tuple<int, std::string, std::vector<uint8_t>>> fonts;

    for(auto const& font : ALL_FONTS)
    {
        std::vector<uint8_t> data;
        ards::ast_node_t dummy{};
        ards::ast_node_t pixels{};
        pixels.type = ards::AST::INT_CONST;
        pixels.value = font.pixels;
        dummy.children.push_back(pixels);
        ards::compiler_t{}.encode_font_ttf(data, dummy, font.data, font.size);
        fonts.push_back({ data[512], font.name, data });
    }

    std::sort(fonts.begin(), fonts.end());

    for(auto const& font : fonts)
    {
        fprintf(f, "| `%s` | %d |", std::get<1>(font).c_str(), std::get<0>(font));
        static char const STR_LOWER[] = "the quick brown fox jumps over the lazy dog";
        static char const STR_UPPER[] = "THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";
        auto const& data = std::get<2>(font);
        int h = data[514] + data[512] + 2;
        int w = 2;
        for(char c : STR_UPPER)
            w += data[int(uint8_t(c)) * 2 + 1];
        if(w > 512)
            w = 512;
        std::vector<uint8_t> buf;
        buf.resize(w * h);
        draw_str(data, buf, w, h, 1, 1, STR_UPPER);
        draw_str(data, buf, w, h, 1, data[512] + 1, STR_LOWER);
        stbi_write_png(
            (std::string(DOCS_DIR) + "/font_images/" + std::get<1>(font) + ".png").c_str(),
            w, h, 1, buf.data(), w);
        fprintf(f, " ![%s](font_images/%s.png) |\n", std::get<1>(font).c_str(), std::get<1>(font).c_str());
    }

    fclose(f);

    return 0;
}
