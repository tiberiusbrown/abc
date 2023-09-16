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
