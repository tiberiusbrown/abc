## Benchmarks

<details><summary>bubble1: 43.05x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 34714</td><td>Cycles: 1494431</td></tr>
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

<details><summary>bubble2: 30.56x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 52858</td><td>Cycles: 1615391</td></tr>
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

<details><summary>bubble3: 27.18x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 71002</td><td>Cycles: 1929887</td></tr>
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

<details><summary>bubble4: 23.01x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 89146</td><td>Cycles: 2050847</td></tr>
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

<details><summary>mat3rotation: 1.06x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 13138</td><td>Cycles: 13909</td></tr>
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

<details><summary>fibonacci: 18.18x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 128131</td><td>Cycles: 2329780</td></tr>
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

<details><summary>format: 0.38x slowdown (2.64x speedup)</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 14057</td><td>Cycles: 5319</td></tr>
<tr>
<td>

```c
#include <Arduboy2.h>
#include <stdio.h>

Arduboy2 a;

void setup()
{
    a.boot();
}

void loop()
{
    char buf[16];
    
    asm volatile("break\n");

    snprintf(buf, sizeof(buf), PSTR("%u"), 1234567890u);

    asm volatile("break\n");
}

```

</td>
<td>

```c
void main()
{
    char[16] buf;

    $debug_break();
    
    $format(buf, "%u", 1234567890u);
    
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>sieve: 38.64x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 14198</td><td>Cycles: 548572</td></tr>
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
    
    for(u8 i = 2; i < SQRT_N; ++i)
    {
        if(!A[i])
            for(u16 j = i * i; j < N; j += i)
                A[j] = true;
    }

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>text: 0.31x slowdown (3.26x speedup)</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 207402</td><td>Cycles: 63653</td></tr>
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
void main()
{
    $debug_break();
    
    $set_text_font(FONT_ADAFRUIT);
    $draw_textf(0, 8, "Running: %u seconds", $millis() / 1000);
    $draw_text_P(0, 17, "the quick brown fox\njumps over the lazy\ndog");
    $draw_text_P(0, 44, "THE QUICK BROWN FOX\nJUMPS OVER THE LAZY\nDOG");
    
    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilessprite: 1.33x slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 90616</td><td>Cycles: 120345</td></tr>
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

int offx = 3;
int offy = 2;

int main()
{
    // for accurate comparison, prevent inlining with extra call here
    // presumably real games would not have a single call to drawOverwrite
    Sprites::drawOverwrite(0, 0, SPRITE, 0);

    debug_break();
    
    bool color = false;
    for(u8 y = 0; y < 8; y = y + 1)
        for(u8 x = 0; x < 16; x = x + 1)
            Sprites::drawOverwrite(x * 8 + offx, y * 8 + offy, SPRITE, 0);

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

int offx = 3;
int offy = 2;

void main()
{
    
    $debug_break();
    
    for(u8 y = 0; y < 8; ++y)
        for(u8 x = 0; x < 16; ++x)
            $draw_sprite(x * 8 + offx, y * 8 + offy, SPRITE, 0);

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilessprite16: 0.65x slowdown (1.54x speedup)</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 65088</td><td>Cycles: 42150</td></tr>
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

int offx = 3;
int offy = 2;

void main()
{
    $debug_break();
    
    for(u8 y = 0; y < 4; ++y)
        for(u8 x = 0; x < 8; ++x)
            $draw_sprite(x * 16 + offx, y * 16 + offy, SPRITE, 0);

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>tilesrect: 0.23x slowdown (4.43x speedup)</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 257725</td><td>Cycles: 58138</td></tr>
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

