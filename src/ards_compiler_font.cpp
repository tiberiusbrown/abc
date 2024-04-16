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
lsb/adv/width     (512 bytes)
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

    float scale = stbtt_ScaleForPixelHeight(&info, (float)pixel_height);

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    int line_height = (int)roundf(scale * (ascent - descent + line_gap));

    int fw = 0;
    int fh = 0;

    int min_lsb = 0;
    int min_y = 1000;
    int max_y = -1000;

    for(int i = 1; i < 256; ++i)
    {
        int adv = 0;
        int lsb = 0;
        stbtt_GetCodepointHMetrics(&info, i, &adv, &lsb);
        min_lsb = std::min(min_lsb, lsb);

        bool alnum = isalnum(i);

        int x0 = 0;
        int y0 = 0;
        int x1 = 0;
        int y1 = 0;
        stbtt_GetCodepointBitmapBox(&info, i, scale, scale, &x0, &y0, &x1, &y1);
        if(alnum)
            min_y = std::min(min_y, y0);
        max_y = std::max(max_y, y1);

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

    fh = max_y - min_y;

    fh = (fh + 7) / 8 * 8;

    std::vector<uint8_t> idata;
    size_t iw = size_t(fw * 256);
    size_t ih = size_t(fh);
    idata.resize(iw * ih);

    //std::vector<uint8_t> edata;
    std::vector<uint8_t> bdata;
    bdata.resize(fw * fh);

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

        int width = x1 - x0;
        int y = y0 - min_y;
        y = std::max(y, 0);

        stbtt_MakeCodepointBitmap(
            &info,
            bdata.data(),
            width, fh,
            fw,
            scale, scale,
            i);

        //edata.clear();
        //encode_sprites_image(edata, n, width, fh, width, fh, false, bdata);
        //memcpy(
        //    idata.data() + i * fw,
        //    edata.data() + 5,
        //    edata.size() - 5);

        // blech i hate this code so much
        int rdst = 0;
        int cdst = 0;
        int rsrc = 0;
        for(int rb = 0; rb < (int)fh; rb += 8)
        {
            for(int ri = 0; ri < 8; ++ri)
            {
                if(rsrc + ri < y) continue;
                int rtmp = rdst + ri;
                int ctmp = cdst;
                for(int ci = 0; ci < width; ++ci)
                {
                    int isrc = (rsrc + ri - y) * fw + ci;
                    int idst = rtmp * (int)iw + i * fw + ctmp;
                    if((size_t)idst >= idata.size()) break;
                    if((size_t)isrc >= bdata.size()) break;
                    idata[idst] = bdata[isrc];
                    if(++ctmp >= fw)
                        ctmp = 0, rtmp += 8;
                }
            }
            rsrc += 8;
            cdst += width;
            while(cdst >= fw)
                cdst -= fw, rdst += 8;
        }

        data.push_back((uint8_t)std::clamp(lsb, -128, 127));
        data.push_back((uint8_t)std::clamp(adv, 0, 255));
        data.push_back((uint8_t)std::clamp(width, 1, fw));
    }
    for(auto& b : idata)
        b = (b >= 128 ? 2 : 1);

    // line height
    data.push_back(uint8_t(std::clamp(line_height, 1, 255)));

    encode_sprites_image(data, n, iw, ih, (size_t)fw, (size_t)fh, false, idata);
}

}
