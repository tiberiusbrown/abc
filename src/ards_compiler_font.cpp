#include "ards_compiler.hpp"

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
adv/lsb           (512 bytes)
Line Height       (1 byte)
---------  image data  ---------
Width             (1 byte)
Height            (1 byte)
Number of Sprites (2 bytes)  256
Masked            (1 byte)     0
Sprite Data
*/

void compiler_t::encode_font(std::vector<uint8_t>& data, ast_node_t const& n)
{
    stbtt_fontinfo info{};

    assert(n.children.size() == 2);
    assert(n.children[0].type == AST::INT_CONST);
    assert(n.children[1].type == AST::STRING_LITERAL);

    int pixel_height = (int)n.children[0].value;

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

    if(!stbtt_InitFont(&info, (unsigned char const*)d.data(), 0))
    {
        errs.push_back({ "Failed to load font data", n.line_info });
        return;
    }

    float scale = stbtt_ScaleForPixelHeight(&info, (float)pixel_height);

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    ascent = (int)ceilf(ascent * scale);
    descent = (int)roundf(descent * scale);

    int fw = 0;
    int fh = 0;

    int min_lsb = 0;

    for(int i = 0; i < 256; ++i)
    {
        int adv = 0;
        int lsb = 0;
        stbtt_GetCodepointHMetrics(&info, i, &adv, &lsb);
        min_lsb = std::min(min_lsb, lsb);

        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
        stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &x0, &y0, &x1, &y1);

        int w = x1 - x0;
        int h = y1 - y0;
        fw = std::max(fw, w);
        fh = std::max(fh, h);
    }

    if(fw <= 0 || fh <= 0)
    {
        errs.push_back({ "Failed to load font data: bad glyph size", n.line_info });
        return;
    }

    int line_height = fh + 1;

    fh = (fh + 7) / 8 * 8;

    std::vector<uint8_t> idata;
    size_t iw = size_t(fw * 256);
    size_t ih = size_t(fh);
    idata.resize(iw * ih * 2);

    for(int i = 0; i < 256; ++i)
    {
        int adv = 0;
        int lsb = 0;
        stbtt_GetCodepointHMetrics(&info, i, &adv, &lsb);
        //lsb -= min_lsb;
        adv = (int)roundf(adv * scale);
        lsb = (int)roundf(lsb * scale);

        data.push_back((uint8_t)std::min(lsb, 255));
        data.push_back((uint8_t)std::min(adv, 255));

        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
        stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &x0, &y0, &x1, &y1);

        int y = ascent + y0;
        y = std::max(y, 0);

        size_t byte_offset = size_t(fw * i + y * iw);
        stbtt_MakeCodepointBitmap(
            &info,
            idata.data() + byte_offset,
            fw, fh - y,
            (int)iw,
            scale, scale,
            i);
    }
    for(auto& b : idata)
        b = (b >= 128 ? 2 : 1);

    // line height
    data.push_back(uint8_t(std::min(255, line_height)));

    encode_sprites_image(data, n, iw, ih, (size_t)fw, (size_t)fh, false, idata);
}

}
