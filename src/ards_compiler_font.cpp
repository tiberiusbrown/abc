#include "ards_compiler.hpp"

#include <algorithm>

#include <cctype>
#include <cmath>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#include <stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ards
{

/*
Font Encoding
================================
glyphs            (1792 bytes)
yadv              (1 byte)
image data

Glyph Encoding (256 glyphs)
================================
xadv              (1 byte)
xoff              (1 byte)
yoff              (1 byte)
offset            (2 bytes)
w                 (1 byte)
h                 (1 byte)
*/

void compiler_t::encode_font(std::vector<uint8_t>& data, ast_node_t const& n)
{
    assert(n.children.size() == 2);
    assert(n.children[0].type == AST::INT_CONST);
    assert(n.children[1].type == AST::STRING_LITERAL);

    std::vector<char> d;
    {
        std::string font_path = current_path + "/";
        font_path += n.children[1].data;
        if(!file_loader || !file_loader(font_path, d))
        {
            errs.push_back({
                "Unable to open font file \"" + font_path + "\"",
                n.children[1].line_info });
            return;
        }
    }

    encode_font_ttf(data, n, (uint8_t const*)d.data(), d.size());
}

void compiler_t::encode_font_ttf(
        std::vector<uint8_t>& data, ast_node_t const& n,
        uint8_t const* ttf_data, size_t ttf_size)
{
    stbtt_fontinfo info{};
    (void)ttf_size;

    int pixel_height = (int)n.children[0].value;

    if(!stbtt_InitFont(&info, (unsigned char const*)ttf_data, 0))
    {
        errs.push_back({ "Failed to load font data", n.line_info });
        return;
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetricsOS2(&info, &ascent, &descent, &line_gap);

    float scale = (float)pixel_height / ascent;

    int line_height = (int)roundf(scale * (ascent - descent + line_gap));

    size_t start = data.size();

    // glyph table
    for(int i = 0; i < 256; ++i)
    {
        int adv = 0;
        int lsb = 0;
        stbtt_GetCodepointHMetrics(&info, i, &adv, &lsb);
        //lsb -= min_lsb;
        adv = (int)roundf(adv * scale);
        lsb = (int)roundf(lsb * scale);

        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
        stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &x0, &y0, &x1, &y1);

        // xadv
        data.push_back((uint8_t)std::clamp(adv, 0, 255));

        // xoff
        data.push_back((uint8_t)std::clamp(lsb, -128, 127));

        // yoff
        data.push_back((uint8_t)std::clamp(y0, -128, 127));

        // offset (defined later)
        data.push_back((uint8_t)0);
        data.push_back((uint8_t)0);

        // w
        data.push_back((uint8_t)std::clamp(x1 - x0, 1, 248));

        // h
        int h = std::clamp(y1 - y0, 1, 248);
        h = (h + 7) / 8 * 8;
        data.push_back((uint8_t)h);
    }

    // yadv
    data.push_back(uint8_t(std::clamp(line_height, 1, 255)));

    // image data
    size_t offset = 0;
    std::vector<uint8_t> idata;
    std::vector<uint8_t> sdata;
    for(int i = 0; i < 256; ++i)
    {
        size_t j = start + i * 7;
        
        int w = data[j + 5];
        int h = data[j + 6];

        if(offset >= 65536)
        {
            errs.push_back({ "Font glyph data too large", n.line_info });
            return;
        }
        
        data[j + 3] = uint8_t(offset >> 0);
        data[j + 4] = uint8_t(offset >> 8);

        idata.resize(w * h);
        
        stbtt_MakeCodepointBitmap(
            &info,
            idata.data(),
            w, h,
            w,
            scale, scale,
            i);

        for(auto& b : idata)
            b = (b >= 128 ? 2 : 1);

        sdata.clear();
        encode_sprites_image(
            sdata, n,
            (size_t)w, (size_t)h,
            (size_t)w, (size_t)h,
            false, idata);

        data.insert(data.end(), sdata.begin() + 5, sdata.end());
        offset += sdata.size() - 5;
    }
}

}
