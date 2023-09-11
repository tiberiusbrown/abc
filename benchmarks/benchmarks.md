## Benchmarks

<details><summary>bubble1: 64.22% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 84844</td><td>Cycles: 5448730</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;
using i8 = int8_t;

inline void debug_break() { asm volatile("break\n"); }

i8 A[100];

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
i8[100] A;

void main()
{
    for(u8 i = 0; i < 100; i = i + 1)
        A[i] = 99 - i;

    $debug_break();
    
    u8 n = 100;
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

<details><summary>bubble2: 43.84% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 129394</td><td>Cycles: 5672810</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;
using i16 = int16_t;

inline void debug_break() { asm volatile("break\n"); }

i16 A[100];

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
i16[100] A;

void main()
{
    for(u8 i = 0; i < 100; i = i + 1)
        A[i] = 99 - i;

    $debug_break();
    
    u8 n = 100;
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

<details><summary>bubble3: 37.36% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 173944</td><td>Cycles: 6499165</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;
using i24 = __int24;

inline void debug_break() { asm volatile("break\n"); }

i24 A[100];

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
i24[100] A;

void main()
{
    for(u8 i = 0; i < 100; i = i + 1)
        A[i] = 99 - i;

    $debug_break();
    
    u8 n = 100;
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

<details><summary>bubble4: 30.98% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 218494</td><td>Cycles: 6768080</td></tr>
<tr>
<td>

```c
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

```

</td>
<td>

```c
i32[100] A;

void main()
{
    for(u8 i = 0; i < 100; i = i + 1)
        A[i] = 99 - i;

    $debug_break();
    
    u8 n = 100;
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

<details><summary>fibonacci: 21.30% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 229951</td><td>Cycles: 4898770</td></tr>
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
    f = fib(18);
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
    f = fib(18);
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilessprite: 6.86% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 29206</td><td>Cycles: 200252</td></tr>
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

<details><summary>tilessprite16: 2.95% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 22654</td><td>Cycles: 66859</td></tr>
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

<details><summary>tilesrect: 0.46% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 257725</td><td>Cycles: 118807</td></tr>
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

