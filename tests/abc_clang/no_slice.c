typedef unsigned short u16;
typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern char *abc_strncpy_P(char *dst, const prog_char *src, u16 n) asm("$strncpy_P");

static char text[8];
static const prog_char lit_text[] = "ok";

int main(void)
{
    abc_strncpy_P(text, lit_text, sizeof(text));
    abc_debug_break();
    return 0;
}
