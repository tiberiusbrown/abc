#include "ards_compiler.hpp"

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
    int64_t w = n.children[0].value;
    int64_t h = n.children[1].value;
    if(w < 1 || w > 248 || h < 1 || h > 248)
    {
        errs.push_back({
            "Sprite width and height must be in the range [1, 248]",
            n.line_info });
        return;
    }
    data.push_back(uint8_t(uint16_t(w) >> 0));
    //data.push_back(uint8_t(uint16_t(w) >> 8));
    data.push_back(uint8_t(uint16_t(h) >> 0));
    //data.push_back(uint8_t(uint16_t(h) >> 8));

    size_t num_rows = n.children.size() - 2;
    if(num_rows % h != 0)
    {
        errs.push_back({
            "Sprite set total number of rows must be a multiple of sprite height",
            n.line_info });
        return;
    }
    for(size_t i = 2; i < n.children.size(); ++i)
    {
        if(n.children[i].data.size() == (size_t)w)
            continue;
        errs.push_back({ "Row length must match sprite width", n.children[i].line_info });
        return;
    }

    size_t num_sprites = num_rows / h;
    size_t actual_h = size_t(h + 7) & 0xfff8;
    std::vector<std::vector<uint8_t>> sdata(num_sprites);
    size_t ichild = 2;
    bool masked = (actual_h != (size_t)h);
    for(size_t si = 0; si < num_sprites; ++si)
    {
        auto& d = sdata[si];
        d.resize(size_t(w * actual_h));
        for(size_t sr = 0, di = 0; sr < (size_t)h; ++sr, ++ichild)
        {
            auto const& child = n.children[ichild];
            assert(child.type == AST::TOKEN);
            assert(child.data.size() == (size_t)w);
            for(size_t ci = 0; ci < child.data.size(); ++ci, ++di)
            {
                char c = child.data[ci];
                if     (c == '.') d[di] = 1;
                else if(c == 'X') d[di] = 2;
                else if(c == '-') d[di] = 0, masked = true;
                else assert(false);
            }
        }
    }

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
