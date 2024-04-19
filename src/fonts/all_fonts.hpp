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
#include "font_br5d.hpp"
#include "font_br5n.hpp"
#include "font_gothicpixels.hpp"
#include "font_pixcon.hpp"
#include "font_pixdor.hpp"
#include "font_pixelgeorgia.hpp"
#include "font_pixelgeorgiabold.hpp"
#include "font_pixeloidsans.hpp"
#include "font_pixeloidsansbold.hpp"
#include "font_pixeltimes.hpp"
#include "font_pixeltimesbold.hpp"
#include "font_pixolletta.hpp"
#include "font_rainyhearts.hpp"
#include "font_retron2000.hpp"
#include "font_simplipixa.hpp"
#include "font_tubeofcorn.hpp"
#include "font_venice.hpp"
#include "font_volter.hpp"
static font_info_t const ALL_FONTS[] = {
    { "FONT_ADAFRUIT", 7, FONT_ADAFRUIT, sizeof(FONT_ADAFRUIT) },
    { "FONT_BANGALOR", 6, FONT_BANGALOR, sizeof(FONT_BANGALOR) },
    { "FONT_BERKELIUM1541", 5, FONT_BERKELIUM1541, sizeof(FONT_BERKELIUM1541) },
    { "FONT_BERKELIUM64", 8, FONT_BERKELIUM64, sizeof(FONT_BERKELIUM64) },
    { "FONT_BIRCHLEAF", 12, FONT_BIRCHLEAF, sizeof(FONT_BIRCHLEAF) },
    { "FONT_BLOCKKIE", 16, FONT_BLOCKKIE, sizeof(FONT_BLOCKKIE) },
    { "FONT_BR4", 4, FONT_BR4, sizeof(FONT_BR4) },
    { "FONT_BR5", 5, FONT_BR5, sizeof(FONT_BR5) },
    { "FONT_BR5D", 5, FONT_BR5D, sizeof(FONT_BR5D) },
    { "FONT_BR5N", 5, FONT_BR5N, sizeof(FONT_BR5N) },
    { "FONT_GOTHICPIXELS", 12, FONT_GOTHICPIXELS, sizeof(FONT_GOTHICPIXELS) },
    { "FONT_PIXCON", 10, FONT_PIXCON, sizeof(FONT_PIXCON) },
    { "FONT_PIXDOR", 12, FONT_PIXDOR, sizeof(FONT_PIXDOR) },
    { "FONT_PIXELGEORGIA", 8, FONT_PIXELGEORGIA, sizeof(FONT_PIXELGEORGIA) },
    { "FONT_PIXELGEORGIABOLD", 8, FONT_PIXELGEORGIABOLD, sizeof(FONT_PIXELGEORGIABOLD) },
    { "FONT_PIXELOIDSANS", 8, FONT_PIXELOIDSANS, sizeof(FONT_PIXELOIDSANS) },
    { "FONT_PIXELOIDSANSBOLD", 8, FONT_PIXELOIDSANSBOLD, sizeof(FONT_PIXELOIDSANSBOLD) },
    { "FONT_PIXELTIMES", 8, FONT_PIXELTIMES, sizeof(FONT_PIXELTIMES) },
    { "FONT_PIXELTIMESBOLD", 8, FONT_PIXELTIMESBOLD, sizeof(FONT_PIXELTIMESBOLD) },
    { "FONT_PIXOLLETTA", 8, FONT_PIXOLLETTA, sizeof(FONT_PIXOLLETTA) },
    { "FONT_RAINYHEARTS", 9, FONT_RAINYHEARTS, sizeof(FONT_RAINYHEARTS) },
    { "FONT_RETRON2000", 21, FONT_RETRON2000, sizeof(FONT_RETRON2000) },
    { "FONT_SIMPLIPIXA", 4, FONT_SIMPLIPIXA, sizeof(FONT_SIMPLIPIXA) },
    { "FONT_TUBEOFCORN", 9, FONT_TUBEOFCORN, sizeof(FONT_TUBEOFCORN) },
    { "FONT_VENICE", 10, FONT_VENICE, sizeof(FONT_VENICE) },
    { "FONT_VOLTER", 8, FONT_VOLTER, sizeof(FONT_VOLTER) },
};
