#include <stdint.h>

using u8 = uint8_t;

inline void debug_break() { asm volatile("break\n"); }

constexpr u8 SQRT_N = 30;
constexpr u16 N = SQRT_N * SQRT_N;
bool A[N];

int main()
{
    debug_break();
    
    for(u8 i = 2; i < SQRT_N; ++i)
    {
        if(!A[i])
            for(u16 j = i * i; j < N; j += i)
                A[j] = true;
    }
    
    debug_break();
    
    for(u16 i = 0; i < N; ++i)
    {
        volatile bool t = A[i];
    }
}
