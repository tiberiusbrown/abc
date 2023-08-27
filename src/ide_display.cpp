#include "ide_common.hpp"

static std::unique_ptr<texture_t> display_texture;

void display_contents()
{
    using namespace ImGui;

    if(!display_texture)
        display_texture = std::make_unique<texture_t>(128, 64);

    if(display_texture && arduboy && arduboy->cpu.decoded)
    {
        std::vector<uint8_t> pixels;
        pixels.resize(128 * 64 * 4);
        for(size_t i = 0; i < 128 * 64; ++i)
        {
            auto t = arduboy->display.filtered_pixels[i];
            pixels[i * 4 + 0] = t;
            pixels[i * 4 + 1] = t;
            pixels[i * 4 + 2] = t;
            pixels[i * 4 + 3] = 255;
        }
        display_texture->update(pixels);
        Image(display_texture->imgui_id(), { 128, 64 });
    }
}

void display_shutdown()
{
    display_texture.reset();
}
