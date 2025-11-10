#include <stdint.h>
#include <Arduboy2.h>

using u8 = uint8_t;

inline void debug_break() { asm volatile("break\n"); }

static constexpr uint8_t SPRITE[] PROGMEM = {
    8, 8,
    0x3c, 0x7e, 0xdb, 0xbf, 0xbf, 0xdb, 0x7e, 0x3c
};

int offx = 3;
int offy = 2;

int main()
{
    // for accurate comparison, prevent inlining with extra call here
    // presumably real games would not have a single call to drawOverwrite
    Sprites::drawOverwrite(0, 0, SPRITE, 0);

    debug_break();
    
    bool color = false;
    for(u8 y = 0; y < 8; y = y + 1)
        for(u8 x = 0; x < 16; x = x + 1)
            Sprites::drawOverwrite(x * 8 + offx, y * 8 + offy, SPRITE, 0);

    debug_break();
}
