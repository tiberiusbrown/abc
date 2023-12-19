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
