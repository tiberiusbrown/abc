## Benchmarks

<details><summary>bubble1: 46.67x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 34714</td><td>Cycles: 1620067</td></tr>
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
    for(u8 i = 0; i < len(A); ++i)
        A[i] = len(A) - i;

    $debug_break();
    
    u8 n = len(A);
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; ++i)
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

<details><summary>bubble2: 33.01x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 52858</td><td>Cycles: 1745059</td></tr>
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
    for(u8 i = 0; i < len(A); ++i)
        A[i] = len(A) - i;

    $debug_break();
    
    u8 n = len(A);
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; ++i)
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

<details><summary>bubble3: 29.04x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 71002</td><td>Cycles: 2061571</td></tr>
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
    for(u8 i = 0; i < len(A); ++i)
        A[i] = len(A) - i;

    $debug_break();
    
    u8 n = len(A);
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; ++i)
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

<details><summary>bubble4: 24.48x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 89146</td><td>Cycles: 2182531</td></tr>
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
    for(u8 i = 0; i < len(A); ++i)
        A[i] = len(A) - i;

    $debug_break();
    
    u8 n = len(A);
    while(n > 1)
    {
        u8 n2 = 0;
        for(u8 i = 1; i < n; ++i)
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

<details><summary>mat3rotation: 1.28x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 13138</td><td>Cycles: 16862</td></tr>
<tr>
<td>

```c
#include <stdint.h>
#include <stddef.h>
#include <math.h>

struct mat3 { float d[9]; };

// a: yaw
// b: pitch
// c: roll
__attribute__((noinline))
mat3 rot(float a, float b, float c)
{
    mat3 m;
    
    float sa = sinf(a);
    float ca = cosf(a);
    float sb = sinf(b);
    float cb = cosf(b);
    float sc = sinf(c);
    float cc = cosf(c);
    float sasb = sa * sb;
    float casb = ca * sb;

    m.d[0] = cb * cc;
    m.d[1] = cb * sc;
    m.d[2] = -sb;

    m.d[3] = sasb * cc - ca * sc;
    m.d[4] = sasb * sc + ca * cc;
    m.d[5] = sa * cb;

    m.d[6] = casb * cc + sa * sc;
    m.d[7] = casb * sc - sa * cc;
    m.d[8] = ca * cb;

    return m;
}

volatile mat3 r;

int main()
{
    r = rot(0, 0, 0);
    
    asm volatile("break\n");
    
    r = rot(1.23, 4.56, 0.789);
    
    asm volatile("break\n");
}

```

</td>
<td>

```c
struct mat3 { float[9] d; };

// a: yaw
// b: pitch
// c: roll
mat3 rot(float a, float b, float c)
{
    mat3 m;
    
    float sa = $sin(a);
    float ca = $cos(a);
    float sb = $sin(b);
    float cb = $cos(b);
    float sc = $sin(c);
    float cc = $cos(c);
    float sasb = sa * sb;
    float casb = ca * sb;

    m.d[0] = cb * cc;
    m.d[1] = cb * sc;
    m.d[2] = -sb;

    m.d[3] = sasb * cc - ca * sc;
    m.d[4] = sasb * sc + ca * cc;
    m.d[5] = sa * cb;

    m.d[6] = casb * cc + sa * sc;
    m.d[7] = casb * sc - sa * cc;
    m.d[8] = ca * cb;

    return m;
}

mat3 r;

void main()
{
    $debug_break();
    
    r = rot(1.23, 4.56, 0.789);
    
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>fibonacci: 18.36x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 128131</td><td>Cycles: 2352985</td></tr>
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
    for(uint8_t i = 0; i < 10; ++i)
        f = fib(12);
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
    for(u8 i = 0; i < 10; ++i)
        f = fib(12);
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>sieve: 53.78x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 20499</td><td>Cycles: 1102487</td></tr>
<tr>
<td>

```c
#include <stdint.h>

using u8 = uint8_t;

inline void debug_break() { asm volatile("break\n"); }

constexpr u8 SQRT_N = 30;
constexpr u16 N = SQRT_N * SQRT_N;
bool A[N];

int main()
{
    debug_break();
    
    for(u16 i = 0; i < N; ++i)
        A[i] = true;
    for(u8 i = 2; i < SQRT_N; ++i)
    {
        if(A[i])
            for(u16 j = i * i; j < N; j += i)
                A[j] = false;
    }
    
    debug_break();
}

```

</td>
<td>

```c
constexpr u8 SQRT_N = 30;
constexpr u16 N = SQRT_N * SQRT_N;
bool[N] A;

void main()
{
    $debug_break();
    
    for(u16 i = 0; i < N; ++i)
        A[i] = true;
    for(u8 i = 2; i < SQRT_N; ++i)
    {
        if(A[i])
            for(u16 j = i * i; j < N; j += i)
                A[j] = false;
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>text: 0.33x slowdown (3.02x speedup)</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 207402</td><td>Cycles: 68753</td></tr>
<tr>
<td>

```c
#include <Arduboy2.h>

Arduboy2 a;

void setup()
{
    a.boot();
}

void loop()
{
    asm volatile("break\n");

    a.setCursor(0, 0);
    a.print(F("Running: "));
    a.print(millis() / 1000);
    a.print(F(" seconds"));
    a.setCursor(0, 9);
    a.print(F("the quick brown fox\njumps over the lazy\ndog"));
    a.setCursor(0, 36);
    a.print(F("THE QUICK BROWN FOX\nJUMPS OVER THE LAZY\nDOG"));

    asm volatile("break\n");
}

```

</td>
<td>

```c
constexpr font f = font{ 8 "font6x8.ttf" };

void main()
{
    $debug_break();
    
    $draw_textf(0, 0, f, "Running: %u seconds", $millis() / 1000);
    $draw_text_P(0, 9, f, "the quick brown fox\njumps over the lazy\ndog");
    $draw_text_P(0, 36, f, "THE QUICK BROWN FOX\nJUMPS OVER THE LAZY\nDOG");
    
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilessprite: 3.31x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 44482</td><td>Cycles: 147129</td></tr>
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
    
    for(u8 y = 0; y < 8; ++y)
    {
        for(u8 x = 0; x < 16; ++x)
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

<details><summary>tilessprite16: 1.96x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 26978</td><td>Cycles: 52821</td></tr>
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
    
    for(u8 y = 0; y < 4; ++y)
    {
        for(u8 x = 0; x < 8; ++x)
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

<details><summary>tilesrect: 0.36x slowdown (2.80x speedup)</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 257725</td><td>Cycles: 91929</td></tr>
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
    for(u8 y = 0; y < 8; y += 1)
    {
        for(u8 x = 0; x < 16; x += 1)
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

