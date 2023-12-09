## Benchmarks

<details><summary>bubble1: 56.30% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 34714</td><td>Cycles: 1954329</td></tr>
<tr>
<td>

```c
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

```

</td>
<td>

```c
i8[64] A;

void main()
{
    for(u8 i = 0; i < 64; i = i + 1)
        A[i] = 64 - i;

    $debug_break();
    
    u8 n = 64;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            i8& a = A[u8(i - 1)];
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

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>bubble2: 38.24% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 52858</td><td>Cycles: 2021237</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;
using i16 = int16_t;

inline void debug_break() { asm volatile("break\n"); }

i16 A[64];

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
            i16& a = A[i - 1];
            i16& b = A[i];
            if(a > b)
            {
                i16 t = a;
                a = b;
                b = t;
                n2 = i;
            }
        }
        n = n2;
    }

    debug_break();
}

```

</td>
<td>

```c
i16[64] A;

void main()
{
    for(u8 i = 0; i < 64; i = i + 1)
        A[i] = 64 - i;

    $debug_break();
    
    u8 n = 64;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            i16& a = A[u8(i - 1)];
            i16& b = A[i];
            if(a > b)
            {
                i16 t = a;
                a = b;
                b = t;
                n2 = i;
            }
        }
        n = n2;
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>bubble3: 33.89% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 71002</td><td>Cycles: 2406557</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;
using i24 = __int24;

inline void debug_break() { asm volatile("break\n"); }

i24 A[64];

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
            i24& a = A[i - 1];
            i24& b = A[i];
            if(a > b)
            {
                i24 t = a;
                a = b;
                b = t;
                n2 = i;
            }
        }
        n = n2;
    }

    debug_break();
}

```

</td>
<td>

```c
i24[64] A;

void main()
{
    for(u8 i = 0; i < 64; i = i + 1)
        A[i] = 64 - i;

    $debug_break();
    
    u8 n = 64;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            i24& a = A[u8(i - 1)];
            i24& b = A[i];
            if(a > b)
            {
                i24 t = a;
                a = b;
                b = t;
                n2 = i;
            }
        }
        n = n2;
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>bubble4: 28.86% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 89146</td><td>Cycles: 2572819</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;
using i32 = int32_t;

inline void debug_break() { asm volatile("break\n"); }

i32 A[64];

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

```

</td>
<td>

```c
i32[64] A;

void main()
{
    for(u8 i = 0; i < 64; i = i + 1)
        A[i] = 64 - i;

    $debug_break();
    
    u8 n = 64;
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; i = i + 1)
        {
            i32& a = A[u8(i - 1)];
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

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>fibonacci: 20.72% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 87831</td><td>Cycles: 1819477</td></tr>
<tr>
<td>

```c
#include <stdint.h>

uint16_t fib(uint8_t n)
{
    if(n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

volatile uint16_t f;

int main()
{
    asm volatile("break\n");
    f = fib(16);
    asm volatile("break\n");
}

```

</td>
<td>

```c
u16 fib(u8 n)
{
    if(n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

u16 f;

void main()
{
    $debug_break();
    f = fib(16);
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilessprite: 4.56% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 44482</td><td>Cycles: 202643</td></tr>
<tr>
<td>

```c
#include <stdint.h>
#include <Arduboy2.h>

using u8 = uint8_t;

inline void debug_break() { asm volatile("break\n"); }

static constexpr uint8_t SPRITE[] PROGMEM = {
    8, 8,
    0x3c, 0x7e, 0xdb, 0xbf, 0xbf, 0xdb, 0x7e, 0x3c
};

int main()
{
    // for accurate comparison, prevent inlining with extra call here
    // presumably real games would not have a single call to drawOverwrite
    Sprites::drawOverwrite(0, 0, SPRITE, 0);

    debug_break();
    
    bool color = false;
    for(u8 y = 0; y < 8; y = y + 1)
    {
        for(u8 x = 0; x < 16; x = x + 1)
        {
            Sprites::drawOverwrite(x * 8, y * 8, SPRITE, 0);
        }
    }

    debug_break();
}

```

</td>
<td>

```c
constexpr sprites SPRITE = sprites{
    8x8
    ..XXXX..
    .XXXXXX.
    XX.XX.XX
    XXXXXXXX
    XXXXXXXX
    XX.XX.XX
    .XX..XX.
    ..XXXX..
};

void main()
{
    
    $debug_break();
    
    for(u8 y = 0; y < 8; y = y + 1)
    {
        for(u8 x = 0; x < 16; x = x + 1)
        {
            $draw_sprite(
                u8(x * 8), u8(y * 8), SPRITE, 0);
        }
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilessprite16: 2.50% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 26978</td><td>Cycles: 67456</td></tr>
<tr>
<td>

```c
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
    // for accurate comparison, prevent inlining with extra call here
    // presumably real games would not have a single call to drawOverwrite
    Sprites::drawOverwrite(0, 0, SPRITE, 0);

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

```

</td>
<td>

```c
constexpr sprites SPRITE = sprites{
    16x16
    XXXXXXXX........
    XXXXXXXX........
    XXXXXXXX........
    XXXXXXXX........
    XXXXXXXX........
    XXXXXXXX........
    XXXXXXXX........
    XXXXXXXX........
    ........XXXXXXXX
    ........XXXXXXXX
    ........XXXXXXXX
    ........XXXXXXXX
    ........XXXXXXXX
    ........XXXXXXXX
    ........XXXXXXXX
    ........XXXXXXXX
};

void main()
{
    $debug_break();
    
    for(u8 y = 0; y < 4; y = y + 1)
    {
        for(u8 x = 0; x < 8; x = x + 1)
        {
            $draw_sprite(
                u8(x * 16), u8(y * 16), SPRITE, 0);
        }
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilesrect: 0.44% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 257725</td><td>Cycles: 112406</td></tr>
<tr>
<td>

```c
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

```

</td>
<td>

```c
void main()
{
    
    $debug_break();
    
    bool color = false;
    for(u8 y = 0; y < 8; y = y + 1)
    {
        for(u8 x = 0; x < 16; x = x + 1)
        {
            $draw_filled_rect(
                u8(x * 8), u8(y * 8), 8, 8, color);
            color = !color;
        }
        color = !color;
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

