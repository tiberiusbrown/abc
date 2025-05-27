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
