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
