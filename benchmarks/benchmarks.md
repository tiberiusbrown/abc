## Benchmarks

<details><summary>bubble1: 66.04% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 84844</td><td>Cycles: 5602837</td></tr>
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

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>bubble2: 45.07% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 129394</td><td>Cycles: 5831867</td></tr>
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

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>bubble3: 37.94% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 173944</td><td>Cycles: 6598632</td></tr>
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

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>bubble4: 31.43% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 218494</td><td>Cycles: 6867452</td></tr>
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

    $debug_break();
}

```

</td>
</tr>
</table>
</details>

<details><summary>fibonacci: 21.14% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 229951</td><td>Cycles: 4860955</td></tr>
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

<details><summary>tilesrect: 0.51% slowdown</summary>
<table>
<tr><th>Native</th><th>ABC</th></tr>
<tr><td>Cycles: 257725</td><td>Cycles: 130191</td></tr>
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
            $draw_filled_rect(x * 8, y * 8, 8, 8, color);
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

