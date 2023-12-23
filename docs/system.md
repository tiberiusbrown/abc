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
bool $any_pressed(u8 buttons);
void $assert(bool cond);
void $debug_break();
void $display();
void $draw_circle(i16 x, i16 y, u8 r, u8 color);
void $draw_filled_circle(i16 x, i16 y, u8 r, u8 color);
void $draw_filled_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
void $draw_hline(i16 x, i16 y, u8 w, u8 color);
void $draw_line(i16 x0, i16 y0, i16 x1, i16 y1, u8 color);
void $draw_pixel(i16 x, i16 y, u8 color);
void $draw_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
void $draw_sprite(i16 x, i16 y, sprites s, u16 frame);
void $draw_text(i16 x, i16 y, font f, char[]& str);
void $draw_text_P(i16 x, i16 y, font f, char[] prog& str);
void $draw_textf(i16 x, i16 y, font f, char[] prog& fmt, ...);
void $draw_vline(i16 x, i16 y, u8 h, u8 color);
void $format(char[]& dst, char[] prog& fmt, ...);
void $idle();
bool $just_pressed(u8 button);
bool $just_released(u8 button);
u32  $millis();
bool $next_frame();
bool $not_pressed(u8 buttons);
void $poll_buttons();
bool $pressed(u8 buttons);
void $set_frame_rate(u8 fps);
i8   $strcmp(char[]& str0, char[]& str1);
i8   $strcmp_P(char[]& str0, char[] prog& str1);
void $strcpy(char[]& dst, char[]& src);
void $strcpy_P(char[]& dst, char[] prog& src);
u16  $strlen(char[]& str);
u24  $strlen_P(char[] prog& str);
u16  $text_width(font f, char[]& str);
u16  $text_width_P(font f, char[] prog& str);
void $tones_play(tones song);
```

