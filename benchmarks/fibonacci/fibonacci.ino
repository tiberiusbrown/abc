#include <stdint.h>

uint16_t fib(uint8_t n)
{
    if(n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

volatile uint16_t fib20;

int main()
{
    asm volatile("break\n");
    fib20 = fib(20);
    asm volatile("break\n");
}
