#include <stdint.h>

using u8 = uint8_t;
using i8 = int8_t;

inline void debug_break() { asm volatile("break\n"); }

i8 A[64];

int main()
{
    for(u8 i = 0; i < 64; i = i + 1)
        A[i] = 64 - i;

    debug_break();
    
    u8 n = 64;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            i8& a = A[i - 1];
            i8& b = A[i];
            if(a > b)
            {
                i8 t = a;
                a = b;
                b = t;
                n2 = i;
            }
        }
        n = n2;
    }

    debug_break();
}
