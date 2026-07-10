typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");
extern char *abc_strncpy_P(char *dst, const prog_char *src, unsigned short n) asm("$strncpy_P");

static char ram_text[4];
static const prog_char prog_text[] = "prog";
static const prog_char fmt_output[] = "strings:%s|%S";
static const prog_char ram_text_init[] = "ram";

int main(void)
{
    abc_strncpy_P(ram_text, ram_text_init, sizeof(ram_text));
    abc_debug_printf(fmt_output, ram_text, prog_text);
    abc_debug_break();
    return 0;
}
