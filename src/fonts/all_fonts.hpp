#pragma once
#include <stdint.h>
struct font_info_t {
    char const* name;
    size_t pixels;
    unsigned char const* data;
    size_t size;
};
#include "font_adafruit.hpp"
#include "font_alkhemikal.hpp"
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
#include "font_kiwisoda.hpp"
#include "font_lexis.hpp"
#include "font_mitochondria.hpp"
#include "font_monkey.hpp"
#include "font_nokiafc.hpp"
#include "font_piacevoli.hpp"
#include "font_pixcon.hpp"
#include "font_pixdor.hpp"
#include "font_pixelgeorgia.hpp"
#include "font_pixelgeorgiabold.hpp"
#include "font_pixellari.hpp"
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
    { "FONT_ALKHEMIKAL", 10, FONT_ALKHEMIKAL, sizeof(FONT_ALKHEMIKAL) },
    { "FONT_BANGALOR", 6, FONT_BANGALOR, sizeof(FONT_BANGALOR) },
    { "FONT_BERKELIUM1541", 5, FONT_BERKELIUM1541, sizeof(FONT_BERKELIUM1541) },
    { "FONT_BERKELIUM64", 8, FONT_BERKELIUM64, sizeof(FONT_BERKELIUM64) },
    { "FONT_BIRCHLEAF", 12, FONT_BIRCHLEAF, sizeof(FONT_BIRCHLEAF) },
    { "FONT_BLOCKKIE", 16, FONT_BLOCKKIE, sizeof(FONT_BLOCKKIE) },
    { "FONT_BR4", 4, FONT_BR4, sizeof(FONT_BR4) },
    { "FONT_BR5", 5, FONT_BR5, sizeof(FONT_BR5) },
    { "FONT_BR5N", 5, FONT_BR5N, sizeof(FONT_BR5N) },
    { "FONT_BR6", 5, FONT_BR6, sizeof(FONT_BR6) },
    { "FONT_CALAMITY", 11, FONT_CALAMITY, sizeof(FONT_CALAMITY) },
    { "FONT_GOTHICPIXELS", 12, FONT_GOTHICPIXELS, sizeof(FONT_GOTHICPIXELS) },
    { "FONT_KIWISODA", 10, FONT_KIWISODA, sizeof(FONT_KIWISODA) },
    { "FONT_LEXIS", 7, FONT_LEXIS, sizeof(FONT_LEXIS) },
    { "FONT_MITOCHONDRIA", 10, FONT_MITOCHONDRIA, sizeof(FONT_MITOCHONDRIA) },
    { "FONT_MONKEY", 10, FONT_MONKEY, sizeof(FONT_MONKEY) },
    { "FONT_NOKIAFC", 7, FONT_NOKIAFC, sizeof(FONT_NOKIAFC) },
    { "FONT_PIACEVOLI", 7, FONT_PIACEVOLI, sizeof(FONT_PIACEVOLI) },
    { "FONT_PIXCON", 10, FONT_PIXCON, sizeof(FONT_PIXCON) },
    { "FONT_PIXDOR", 12, FONT_PIXDOR, sizeof(FONT_PIXDOR) },
    { "FONT_PIXELGEORGIA", 8, FONT_PIXELGEORGIA, sizeof(FONT_PIXELGEORGIA) },
    { "FONT_PIXELGEORGIABOLD", 8, FONT_PIXELGEORGIABOLD, sizeof(FONT_PIXELGEORGIABOLD) },
    { "FONT_PIXELLARI", 11, FONT_PIXELLARI, sizeof(FONT_PIXELLARI) },
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
