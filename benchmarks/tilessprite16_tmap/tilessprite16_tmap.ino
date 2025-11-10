#include <stdint.h>
#include <Arduboy2.h>

using u8 = uint8_t;

inline void debug_break() { asm volatile("break\n"); }

static constexpr uint8_t SPRITE[] PROGMEM = {
    16, 16,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
    for(u8 y = 0; y < 4; y = y + 1)
        for(u8 x = 0; x < 8; x = x + 1)
            Sprites::drawOverwrite(x * 16 + offx, y * 16 + offy, SPRITE, 0);

    debug_break();
}
