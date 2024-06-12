#pragma once

#include "ards_vm.hpp"

/*
Draw Command Encoding
===============================================

0   end of commands

1   filled rect
        1   x
        1   y
        1   w
        1   h
        1   color

*/

enum
{
    SHADES_CMD_END,
    SHADES_CMD_RECT,
    SHADES_CMD_FILLED_RECT,
    SHADES_CMD_SPRITE,
    SHADES_CMD_SPRITE_BATCH,
    SHADES_CMD_CHARS,
};

void shades_init();

void shades_swap();
void shades_display();

void shades_draw_rect(
    int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t c, bool filled);
void shades_draw_sprite(
    int16_t x, int16_t y, uint24_t img, uint16_t frame);

void shades_draw_chars_begin(int16_t x, int16_t y);
void shades_draw_char(char c);
void shades_draw_chars_end();
