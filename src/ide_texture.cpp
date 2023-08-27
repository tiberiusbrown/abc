#include "ide_common.hpp"

#include <sokol_gfx.h>

texture_t::texture_t(int w, int h)
    : w(w), h(h)
{
    sg_image_desc desc{};
    desc.width = w;
    desc.height = h;
    desc.pixel_format = SG_PIXELFORMAT_RGBA8;
    desc.min_filter = SG_FILTER_LINEAR;
    desc.mag_filter = SG_FILTER_NEAREST;
    desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
    desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
    desc.usage = SG_USAGE_STREAM;
    id = sg_make_image(&desc).id;
}

texture_t::~texture_t()
{
    sg_destroy_image({ (uint32_t)id });
}

void texture_t::update(std::vector<uint8_t> const& rgba)
{
    sg_image_data idata{};
    size_t n = w * h * 4;
    if(rgba.size() == n)
        idata.subimage[0][0] = { rgba.data(), n };
    else
    {
        std::vector<uint8_t> t = rgba;
        t.resize(size_t(w * h * 4));
        idata.subimage[0][0] = { t.data(), t.size() };
    }
    sg_update_image({ (uint32_t)id }, &idata);
}
