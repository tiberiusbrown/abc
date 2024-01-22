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
