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
#include "font_blackletter.hpp"
#include "font_blockkie.hpp"
#include "font_br4.hpp"
#include "font_br5.hpp"
#include "font_br5d.hpp"
#include "font_br5n.hpp"
#include "font_bserif.hpp"
#include "font_chunkysans6.hpp"
#include "font_chunkysans8.hpp"
#include "font_dungeon.hpp"
#include "font_faithless.hpp"
#include "font_glasgow.hpp"
#include "font_gothicpixels2.hpp"
#include "font_gothicpixels.hpp"
#include "font_laika.hpp"
#include "font_m5x7.hpp"
#include "font_monogram.hpp"
#include "font_monogramitalic.hpp"
#include "font_oldstyle.hpp"
#include "font_pixcon.hpp"
#include "font_pixdor.hpp"
#include "font_pixel5.hpp"
#include "font_pixelgeorgia.hpp"
#include "font_pixelgeorgiabold.hpp"
#include "font_pixeloidsans.hpp"
#include "font_pixeloidsansbold.hpp"
#include "font_pixeltimes.hpp"
#include "font_pixeltimesbold.hpp"
#include "font_pixolletta.hpp"
#include "font_rainyhearts.hpp"
#include "font_retron2000.hpp"
#include "font_scimono.hpp"
#include "font_scrawl.hpp"
#include "font_signature.hpp"
#include "font_simplipixa.hpp"
#include "font_slabserif.hpp"
#include "font_thirddimension.hpp"
#include "font_tubeofcorn.hpp"
#include "font_uibold.hpp"
#include "font_uicondensed.hpp"
#include "font_upheaval.hpp"
#include "font_venice.hpp"
#include "font_volter.hpp"
static font_info_t const ALL_FONTS[] = {
    { "FONT_ADAFRUIT", 7, FONT_ADAFRUIT, sizeof(FONT_ADAFRUIT) },
    { "FONT_BANGALOR", 6, FONT_BANGALOR, sizeof(FONT_BANGALOR) },
    { "FONT_BERKELIUM1541", 5, FONT_BERKELIUM1541, sizeof(FONT_BERKELIUM1541) },
    { "FONT_BERKELIUM64", 8, FONT_BERKELIUM64, sizeof(FONT_BERKELIUM64) },
    { "FONT_BIRCHLEAF", 12, FONT_BIRCHLEAF, sizeof(FONT_BIRCHLEAF) },
    { "FONT_BLACKLETTER", 9, FONT_BLACKLETTER, sizeof(FONT_BLACKLETTER) },
    { "FONT_BLOCKKIE", 16, FONT_BLOCKKIE, sizeof(FONT_BLOCKKIE) },
    { "FONT_BR4", 4, FONT_BR4, sizeof(FONT_BR4) },
    { "FONT_BR5", 5, FONT_BR5, sizeof(FONT_BR5) },
    { "FONT_BR5D", 5, FONT_BR5D, sizeof(FONT_BR5D) },
    { "FONT_BR5N", 5, FONT_BR5N, sizeof(FONT_BR5N) },
    { "FONT_BSERIF", 7, FONT_BSERIF, sizeof(FONT_BSERIF) },
    { "FONT_CHUNKYSANS6", 5, FONT_CHUNKYSANS6, sizeof(FONT_CHUNKYSANS6) },
    { "FONT_CHUNKYSANS8", 6, FONT_CHUNKYSANS8, sizeof(FONT_CHUNKYSANS8) },
    { "FONT_DUNGEON", 10, FONT_DUNGEON, sizeof(FONT_DUNGEON) },
    { "FONT_FAITHLESS", 7, FONT_FAITHLESS, sizeof(FONT_FAITHLESS) },
    { "FONT_GLASGOW", 10, FONT_GLASGOW, sizeof(FONT_GLASGOW) },
    { "FONT_GOTHICPIXELS2", 15, FONT_GOTHICPIXELS2, sizeof(FONT_GOTHICPIXELS2) },
    { "FONT_GOTHICPIXELS", 12, FONT_GOTHICPIXELS, sizeof(FONT_GOTHICPIXELS) },
    { "FONT_LAIKA", 8, FONT_LAIKA, sizeof(FONT_LAIKA) },
    { "FONT_M5X7", 7, FONT_M5X7, sizeof(FONT_M5X7) },
    { "FONT_MONOGRAM", 7, FONT_MONOGRAM, sizeof(FONT_MONOGRAM) },
    { "FONT_MONOGRAMITALIC", 7, FONT_MONOGRAMITALIC, sizeof(FONT_MONOGRAMITALIC) },
    { "FONT_OLDSTYLE", 8, FONT_OLDSTYLE, sizeof(FONT_OLDSTYLE) },
    { "FONT_PIXCON", 10, FONT_PIXCON, sizeof(FONT_PIXCON) },
    { "FONT_PIXDOR", 12, FONT_PIXDOR, sizeof(FONT_PIXDOR) },
    { "FONT_PIXEL5", 4, FONT_PIXEL5, sizeof(FONT_PIXEL5) },
    { "FONT_PIXELGEORGIA", 8, FONT_PIXELGEORGIA, sizeof(FONT_PIXELGEORGIA) },
    { "FONT_PIXELGEORGIABOLD", 8, FONT_PIXELGEORGIABOLD, sizeof(FONT_PIXELGEORGIABOLD) },
    { "FONT_PIXELOIDSANS", 8, FONT_PIXELOIDSANS, sizeof(FONT_PIXELOIDSANS) },
    { "FONT_PIXELOIDSANSBOLD", 8, FONT_PIXELOIDSANSBOLD, sizeof(FONT_PIXELOIDSANSBOLD) },
    { "FONT_PIXELTIMES", 8, FONT_PIXELTIMES, sizeof(FONT_PIXELTIMES) },
    { "FONT_PIXELTIMESBOLD", 8, FONT_PIXELTIMESBOLD, sizeof(FONT_PIXELTIMESBOLD) },
    { "FONT_PIXOLLETTA", 8, FONT_PIXOLLETTA, sizeof(FONT_PIXOLLETTA) },
    { "FONT_RAINYHEARTS", 9, FONT_RAINYHEARTS, sizeof(FONT_RAINYHEARTS) },
    { "FONT_RETRON2000", 21, FONT_RETRON2000, sizeof(FONT_RETRON2000) },
    { "FONT_SCIMONO", 8, FONT_SCIMONO, sizeof(FONT_SCIMONO) },
    { "FONT_SCRAWL", 7, FONT_SCRAWL, sizeof(FONT_SCRAWL) },
    { "FONT_SIGNATURE", 11, FONT_SIGNATURE, sizeof(FONT_SIGNATURE) },
    { "FONT_SIMPLIPIXA", 4, FONT_SIMPLIPIXA, sizeof(FONT_SIMPLIPIXA) },
    { "FONT_SLABSERIF", 8, FONT_SLABSERIF, sizeof(FONT_SLABSERIF) },
    { "FONT_THIRDDIMENSION", 11, FONT_THIRDDIMENSION, sizeof(FONT_THIRDDIMENSION) },
    { "FONT_TUBEOFCORN", 9, FONT_TUBEOFCORN, sizeof(FONT_TUBEOFCORN) },
    { "FONT_UIBOLD", 9, FONT_UIBOLD, sizeof(FONT_UIBOLD) },
    { "FONT_UICONDENSED", 10, FONT_UICONDENSED, sizeof(FONT_UICONDENSED) },
    { "FONT_UPHEAVAL", 10, FONT_UPHEAVAL, sizeof(FONT_UPHEAVAL) },
    { "FONT_VENICE", 10, FONT_VENICE, sizeof(FONT_VENICE) },
    { "FONT_VOLTER", 8, FONT_VOLTER, sizeof(FONT_VOLTER) },
};
