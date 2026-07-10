typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");

static const prog_char fmt_output[] = "static";

int main(void)
{
    abc_debug_printf(fmt_output);
    abc_debug_break();
    return 0;
}
