#include <stdint.h>

using u8 = uint8_t;
using i16 = int16_t;

inline void debug_break() { asm volatile("break\n"); }

i16 A[200];

void swap(i16& a, i16& b)
{
    i16 t = a;
    a = b;
    b = t;
}

int main()
{
    for(u8 i = 0; i < 200; i = i + 1)
        A[i] = 199 - i;

    debug_break();
    
    u8 n = 200;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            if(A[i - 1] > A[i])
            {
                swap(A[i - 1], A[i]);
                n2 = i;
            }
        }
        n = n2;
    }

    debug_break();
}
