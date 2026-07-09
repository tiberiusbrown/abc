typedef unsigned char u8;
typedef unsigned short u16;
typedef const char __attribute__((address_space(1))) prog_char;

extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");
extern void *abc_memset(void *dst, int value, u16 n) asm("$memset");
extern void *abc_memcpy(void *dst, const void *src, u16 n) asm("$memcpy");
extern void *abc_memcpy_P(void *dst, const prog_char *src, u16 n) asm("$memcpy_P");
extern char *abc_strncpy_P(char *dst, const prog_char *src, u16 n) asm("$strncpy_P");
extern char *abc_strncat_P(char *dst, const prog_char *src, u16 n) asm("$strncat_P");
extern int abc_strncmp(const char *lhs, const char *rhs, u16 n) asm("$strncmp");
extern u16 abc_strnlen(const char *src, u16 n) asm("$strnlen");
extern void abc_format(char *dst, u16 capacity, const prog_char *fmt, ...) asm("$format");

static char text_a[32];
static char text_b[32];
static char pad[8];
static char trunc_buf[5];
static char embedded_buf[8];

static const prog_char fmt_output[] = "A:%s|L:%u|C:%d|M:%s|P:%s|T:%s|E:%s\n";
static const prog_char fmt_string[] = "%s";
static const prog_char lit_head[] = "cat";
static const prog_char lit_tail[] = "erpillar";
static const prog_char lit_exact[] = "cater";
static const prog_char lit_embedded[] = {'o', 'k', 0, 'x', 'x', 0};

int main(void)
{
    u16 len;
    int cmp;

    abc_memset(text_a, 0, sizeof(text_a));
    abc_memset(text_b, 0, sizeof(text_b));
    abc_memset(pad, 0, sizeof(pad));
    abc_memset(trunc_buf, 0, sizeof(trunc_buf));
    abc_memset(embedded_buf, 0, sizeof(embedded_buf));

    abc_strncpy_P(text_a, lit_head, sizeof(text_a));
    abc_strncat_P(text_a, lit_tail, 2);
    abc_strncpy_P(text_b, lit_exact, sizeof(text_b));
    len = abc_strnlen(text_a, sizeof(text_a));
    cmp = abc_strncmp(text_a, text_b, len);

    abc_memcpy(text_b, text_a, 6);
    abc_memset(pad, 'X', 3);
    abc_format(trunc_buf, sizeof(trunc_buf), fmt_string, text_a);
    abc_memcpy_P(embedded_buf, lit_embedded, sizeof(lit_embedded));

    abc_debug_printf(
        fmt_output,
        text_a,
        len,
        cmp,
        text_b,
        pad,
        trunc_buf,
        embedded_buf);
    abc_debug_break();
    return 0;
}
