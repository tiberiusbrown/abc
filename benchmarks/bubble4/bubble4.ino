#include <stdint.h>

using u8 = uint8_t;
using i32 = int32_t;

inline void debug_break() { asm volatile("break\n"); }

i32 A[100];

int main()
{
    for(u8 i = 0; i < 100; i = i + 1)
        A[i] = 99 - i;

    debug_break();
    
    u8 n = 100;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            i32& a = A[i - 1];
            i32& b = A[i];
            if(a > b)
            {
                i32 t = a;
                a = b;
                b = t;
                n2 = i;
            }
        }
        n = n2;
    }

    debug_break();
}
