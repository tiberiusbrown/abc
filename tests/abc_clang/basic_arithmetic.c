__attribute__((noinline)) int basic_sum(int x, int y) { return x + y; }
__attribute__((noinline)) int basic_min(int x, int y) { return x < y ? x : y; }
__attribute__((noinline)) int basic_max(int x, int y) { return x < y ? y : x; }
__attribute__((noinline)) int basic_clamp(int x, int a, int b) { return x < a ? a : b < x ? b : x; }

typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");

static const prog_char fmt_output[] = "arith:%d|%d|%d|%d";

__attribute__((noinline)) int array_sum(int* x, int n)
{
    int t = 0;
    for(int i = 0; i < n; ++i)
        t += x[i];
    return t;
}

__attribute__((noinline)) int array_sum_prog(int __prog* x, int n)
{
    int t = 0;
    for(int i = 0; i < n; ++i)
    {
        t += x[i];
    }
    return t;
}

int main()
{
    abc_debug_printf(fmt_output,
        basic_sum(2, 3),
        basic_min(7, 3),
        basic_max(7, 3),
        basic_clamp(5, 1, 4));
    abc_debug_break();
    return 0;
}
