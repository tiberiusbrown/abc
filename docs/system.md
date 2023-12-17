# Predefined Constants

```c
u8 WHITE;
u8 BLACK;
u8 A_BUTTON;
u8 B_BUTTON;
u8 UP_BUTTON;
u8 DOWN_BUTTON;
u8 LEFT_BUTTON;
u8 RIGHT_BUTTON;
```

# System Calls

```c
bool $any_pressed(u8);
void $assert(bool);
void $debug_break();
void $display();
void $draw_circle(i16, i16, u8, u8);
void $draw_filled_circle(i16, i16, u8, u8);
void $draw_filled_rect(i16, i16, u8, u8, u8);
void $draw_hline(i16, i16, u8, u8);
void $draw_line(i16, i16, i16, i16, u8);
void $draw_pixel(i16, i16, u8);
void $draw_rect(i16, i16, u8, u8, u8);
void $draw_sprite(i16, i16, sprites, u16);
void $draw_text(i16, i16, font, char[]&);
void $draw_text_P(i16, i16, font, char[] prog&);
void $draw_textf(i16, i16, font, char[] prog&, ...);
void $draw_vline(i16, i16, u8, u8);
void $format(char[]&, char[] prog&, ...);
void $idle();
bool $just_pressed(u8);
bool $just_released(u8);
u32  $millis();
bool $next_frame();
bool $not_pressed(u8);
void $poll_buttons();
bool $pressed(u8);
void $set_frame_rate(u8);
i8   $strcmp(char[]&, char[]&);
i8   $strcmp_P(char[]&, char[] prog&);
void $strcpy(char[]&, char[]&);
void $strcpy_P(char[]&, char[] prog&);
u16  $strlen(char[]&);
u24  $strlen_P(char[] prog&);
u16  $text_width(font, char[]&);
u16  $text_width_P(font, char[] prog&);
```

