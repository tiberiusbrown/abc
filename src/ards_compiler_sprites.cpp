#include "ards_compiler.hpp"

#include <cctype>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ards
{

/*
* 
Sprite Encoding
================================
Width             (1 byte)
Height            (1 byte)
Number of Sprites (2 bytes)
Masked            (1 byte)
Sprite Data

*/

void compiler_t::encode_sprites(std::vector<uint8_t>& data, ast_node_t const& n)
{
    assert(n.children.size() > 2);
    assert(n.children[0].type == AST::INT_CONST);
    assert(n.children[1].type == AST::INT_CONST);
    size_t w = (size_t)n.children[0].value;
    size_t h = (size_t)n.children[1].value;

    std::vector<uint8_t> idata;
    size_t iw = 0;
    size_t ih = 0;
    bool masked = false;

    if(n.children[2].type == AST::STRING_LITERAL)
    {
        std::vector<char> d;
        std::string image_path = current_path + "/";
        image_path += n.children[2].data;
        if(!file_loader || !file_loader(image_path, d))
        {
            errs.push_back({
                "Unable to open image file \"" + image_path + "\"",
                n.children[2].line_info });
            return;
        }
        int tx = 0, ty = 0, tc = 0;
        stbi_uc* loaded_data = stbi_load_from_memory(
            (stbi_uc*)d.data(), (int)d.size(), &tx, &ty, &tc, 2);
        if(!loaded_data || tx <= 0 || ty <= 0)
        {
            errs.push_back({
                "Unable to load image file \"" + image_path + "\"",
                n.children[2].line_info });
            return;
        }
        iw = (size_t)tx;
        ih = (size_t)ty;
        size_t loaded_pixels = iw * ih;
        idata.reserve(loaded_pixels);
        for(size_t i = 0; i < loaded_pixels; ++i)
        {
            uint8_t c = loaded_data[i * 2 + 0];
            uint8_t a = loaded_data[i * 2 + 1];
            if(a < 128) idata.push_back(0), masked = true;
            else if(c < 128) idata.push_back(1);
            else             idata.push_back(2);
        }
        stbi_image_free(loaded_data);
    }
    else
    {
        auto const& nd = n.children[2];
        for(size_t i = 0; i < nd.children.size(); ++i)
        {
            auto const& row = nd.children[i];
            assert(row.type == AST::TOKEN);
            for(char c : row.data)
            {
                if(isspace(c)) continue;
                if(c == '-') idata.push_back(0), masked = true;
                else if(c == '.') idata.push_back(1);
                else              idata.push_back(2);
            }
            if(iw == 0)
                iw = idata.size();
        }
        if(iw == 0)
        {
            errs.push_back({
                "No sprite data found",
                nd.line_info });
            return;
        }
        ih = idata.size() / iw;
    }

    encode_sprites_image(data, n, iw, ih, w, h, masked, idata);
}

void compiler_t::encode_sprites_image(
    std::vector<uint8_t>& data, ast_node_t const& n,
    size_t iw, size_t ih, size_t w, size_t h, bool masked,
    std::vector<uint8_t> const& idata)
{
    if(w == 0) w = iw;
    if(h == 0) h = ih;

    if(w > 248 || h > 248)
    {
        errs.push_back({
            "Sprite width and height must be in the range [0, 248]",
            n.line_info });
        return;
    }

    if(ih % h != 0 || iw % w != 0)
    {
        errs.push_back({
            "Sprites image width/height must be a multiple of sprite width/height",
            n.line_info });
        return;
    }

    size_t numw = iw / w;
    size_t numh = ih / h;
    size_t num_sprites = numw * numh;
    size_t actual_h = size_t(h + 7) & 0xfff8;
    std::vector<std::vector<uint8_t>> sdata(num_sprites);
    if(actual_h != (size_t)h)
        masked = true;
    for(size_t si = 0; si < num_sprites; ++si)
    {
        size_t ir = si / numw * h;
        size_t ic = si % numw * w;
        auto& d = sdata[si];
        d.resize(size_t(w * actual_h));

        for(size_t r = 0, di = 0; r < h; ++r)
        {
            for(size_t c = 0; c < w; ++c, ++di)
            {
                d[di] = idata[(ir + r) * iw + (ic + c)];
            }
        }
    }

    data.push_back(uint8_t(uint16_t(w) >> 0));
    //data.push_back(uint8_t(uint16_t(w) >> 8));
    data.push_back(uint8_t(uint16_t(actual_h) >> 0));
    //data.push_back(uint8_t(uint16_t(actual_h) >> 8));

    data.push_back(uint8_t(num_sprites >> 0));
    data.push_back(uint8_t(num_sprites >> 8));
    data.push_back(masked ? 1 : 0);

    size_t pages = actual_h / 8;
    size_t bytes_per_sprite = size_t(w * pages);
    if(masked) bytes_per_sprite *= 2;
    data.reserve(data.size() + num_sprites * bytes_per_sprite);

    for(size_t si = 0; si < num_sprites; ++si)
    {
        auto const& d = sdata[si];
        for(size_t p = 0; p < pages; ++p)
        {
            for(size_t x = 0; x < (size_t)w; ++x)
            {
                uint8_t byte = 0;
                for(size_t r = 0; r < 8; ++r)
                {
                    size_t y = p * 8 + r;
                    uint8_t pixel = d[y * w + x];
                    if(pixel == 2) byte |= (1 << r);
                }
                data.push_back(byte);
                if(!masked) continue;
                byte = 0;
                for(size_t r = 0; r < 8; ++r)
                {
                    size_t y = p * 8 + r;
                    uint8_t pixel = d[y * w + x];
                    if(pixel != 0) byte |= (1 << r);
                }
                data.push_back(byte);
            }
        }
    }
}

}
