typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");
extern char *abc_strncpy_P(char *dst, const prog_char *src, unsigned short n) asm("$strncpy_P");

static char text[4];
static const prog_char text_init[] = "ram";
static const prog_char fmt_output[] = "ram:%s";

int main(void)
{
    abc_strncpy_P(text, text_init, sizeof(text));
    abc_debug_printf(fmt_output, text);
    abc_debug_break();
    return 0;
}
