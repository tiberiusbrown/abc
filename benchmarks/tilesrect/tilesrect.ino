#include <stdint.h>
#include <Arduboy2.h>

using u8 = uint8_t;

inline void debug_break() { asm volatile("break\n"); }

int main()
{

    debug_break();
    
    bool color = false;
    for(u8 y = 0; y < 8; y = y + 1)
    {
        for(u8 x = 0; x < 16; x = x + 1)
        {
            Arduboy2::fillRect(x * 8, y * 8, 8, 8, color);
            color = !color;
        }
        color = !color;
    }

    debug_break();
}
