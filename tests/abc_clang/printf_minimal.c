typedef unsigned short u16;
typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");
extern char *abc_strncpy_P(char *dst, const prog_char *src, u16 n) asm("$strncpy_P");

static char text[16];
static const prog_char fmt_output[] = "W:%04u|S:%S|%%\n";
static const prog_char lit_text[] = "zap";

int main(void)
{
    abc_strncpy_P(text, lit_text, sizeof(text));
    abc_debug_printf(fmt_output, 42, lit_text);
    abc_debug_break();
    return 0;
}
