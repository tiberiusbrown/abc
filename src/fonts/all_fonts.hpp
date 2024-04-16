#pragma once
#include <stdint.h>
struct font_info_t {
    char const* name;
    size_t pixels;
    unsigned char const* data;
    size_t size;
};
#include "font_adafruit.hpp"
#include "font_bangalor.hpp"
#include "font_berkelium1541.hpp"
#include "font_berkelium64.hpp"
#include "font_birchleaf.hpp"
#include "font_blockkie.hpp"
#include "font_br4.hpp"
#include "font_br5.hpp"
#include "font_br5n.hpp"
#include "font_br6.hpp"
#include "font_calamity.hpp"
#include "font_gothicpixels.hpp"
#include "font_lexis.hpp"
#include "font_nokiafc.hpp"
#include "font_pixcon.hpp"
#include "font_pixdor.hpp"
#include "font_pixelgeorgia.hpp"
#include "font_pixelgeorgiabold.hpp"
#include "font_pixeltimes.hpp"
#include "font_pixeltimesbold.hpp"
#include "font_rainyhearts.hpp"
#include "font_retron2000.hpp"
#include "font_simplipixa.hpp"
#include "font_tubeofcorn.hpp"
#include "font_volter.hpp"
static font_info_t const ALL_FONTS[] = {
    { "FONT_ADAFRUIT", 8, FONT_ADAFRUIT, sizeof(FONT_ADAFRUIT) },
    { "FONT_BANGALOR", 8, FONT_BANGALOR, sizeof(FONT_BANGALOR) },
    { "FONT_BERKELIUM1541", 6, FONT_BERKELIUM1541, sizeof(FONT_BERKELIUM1541) },
    { "FONT_BERKELIUM64", 12, FONT_BERKELIUM64, sizeof(FONT_BERKELIUM64) },
    { "FONT_BIRCHLEAF", 17, FONT_BIRCHLEAF, sizeof(FONT_BIRCHLEAF) },
    { "FONT_BLOCKKIE", 26, FONT_BLOCKKIE, sizeof(FONT_BLOCKKIE) },
    { "FONT_BR4", 4, FONT_BR4, sizeof(FONT_BR4) },
    { "FONT_BR5", 5, FONT_BR5, sizeof(FONT_BR5) },
    { "FONT_BR5N", 5, FONT_BR5N, sizeof(FONT_BR5N) },
    { "FONT_BR6", 6, FONT_BR6, sizeof(FONT_BR6) },
    { "FONT_CALAMITY", 16, FONT_CALAMITY, sizeof(FONT_CALAMITY) },
    { "FONT_GOTHICPIXELS", 16, FONT_GOTHICPIXELS, sizeof(FONT_GOTHICPIXELS) },
    { "FONT_LEXIS", 8, FONT_LEXIS, sizeof(FONT_LEXIS) },
    { "FONT_NOKIAFC", 10, FONT_NOKIAFC, sizeof(FONT_NOKIAFC) },
    { "FONT_PIXCON", 18, FONT_PIXCON, sizeof(FONT_PIXCON) },
    { "FONT_PIXDOR", 16, FONT_PIXDOR, sizeof(FONT_PIXDOR) },
    { "FONT_PIXELGEORGIA", 13, FONT_PIXELGEORGIA, sizeof(FONT_PIXELGEORGIA) },
    { "FONT_PIXELGEORGIABOLD", 13, FONT_PIXELGEORGIABOLD, sizeof(FONT_PIXELGEORGIABOLD) },
    { "FONT_PIXELTIMES", 13, FONT_PIXELTIMES, sizeof(FONT_PIXELTIMES) },
    { "FONT_PIXELTIMESBOLD", 13, FONT_PIXELTIMESBOLD, sizeof(FONT_PIXELTIMESBOLD) },
    { "FONT_RAINYHEARTS", 15, FONT_RAINYHEARTS, sizeof(FONT_RAINYHEARTS) },
    { "FONT_RETRON2000", 36, FONT_RETRON2000, sizeof(FONT_RETRON2000) },
    { "FONT_SIMPLIPIXA", 6, FONT_SIMPLIPIXA, sizeof(FONT_SIMPLIPIXA) },
    { "FONT_TUBEOFCORN", 14, FONT_TUBEOFCORN, sizeof(FONT_TUBEOFCORN) },
    { "FONT_VOLTER", 10, FONT_VOLTER, sizeof(FONT_VOLTER) },
};
