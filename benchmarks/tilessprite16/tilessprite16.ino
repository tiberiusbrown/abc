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

int main()
{

    debug_break();
    
    bool color = false;
    for(u8 y = 0; y < 4; y = y + 1)
    {
        for(u8 x = 0; x < 8; x = x + 1)
        {
            Sprites::drawOverwrite(x * 16, y * 16, SPRITE, 0);
        }
    }

    debug_break();
}
