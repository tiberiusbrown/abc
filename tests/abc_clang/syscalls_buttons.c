typedef unsigned char u8;
typedef unsigned char bool;
typedef const char __attribute__((address_space(1))) prog_char;

extern u8 abc_buttons(void) asm("$buttons");
extern bool abc_just_pressed(u8 button) asm("$just_pressed");
extern bool abc_just_released(u8 button) asm("$just_released");
extern bool abc_pressed(u8 buttons) asm("$pressed");
extern bool abc_any_pressed(u8 buttons) asm("$any_pressed");
extern bool abc_not_pressed(u8 buttons) asm("$not_pressed");
extern void abc_debug_break(void) asm("$debug_break");
extern void abc_debug_printf(const prog_char *fmt, ...) asm("$debug_printf");

static const prog_char fmt_output[] = "buttons:%u|%d|%d";

int main(void)
{
    abc_debug_printf(fmt_output,
        abc_buttons(),
        abc_just_pressed(0),
        abc_just_released(0));
    abc_debug_break();
    return 0;
}
