# Predefined Constants

```c
u8 WHITE;
u8 LIGHT_GRAY;
u8 LIGHT_GREY;
u8 GRAY;
u8 GREY;
u8 DARK_GRAY;
u8 DARK_GREY;
u8 BLACK;
u8 A_BUTTON;
u8 B_BUTTON;
u8 UP_BUTTON;
u8 DOWN_BUTTON;
u8 LEFT_BUTTON;
u8 RIGHT_BUTTON;
float PI;
```

# System Calls

```c
bool  $any_pressed(u8 buttons);
void  $assert(bool cond);
float $atan2(float y, float x);
bool  $audio_enabled();
bool  $audio_playing();
void  $audio_stop();
void  $audio_toggle();
float $ceil(float x);
float $cos(float angle);
void  $debug_break();
void  $debug_printf(char[] prog& fmt, ...);
void  $display();
void  $display_noclear();
void  $draw_circle(i16 x, i16 y, u8 r, u8 color);
void  $draw_filled_circle(i16 x, i16 y, u8 r, u8 color);
void  $draw_filled_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
void  $draw_hline(i16 x, i16 y, u8 w, u8 color);
void  $draw_line(i16 x0, i16 y0, i16 x1, i16 y1, u8 color);
void  $draw_pixel(i16 x, i16 y, u8 color);
void  $draw_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
void  $draw_sprite(i16 x, i16 y, sprites s, u16 frame);
void  $draw_sprite_selfmask(i16 x, i16 y, sprites s, u16 frame);
void  $draw_text(i16 x, i16 y, char[]& str);
void  $draw_text_P(i16 x, i16 y, char[] prog& str);
void  $draw_textf(i16 x, i16 y, char[] prog& fmt, ...);
void  $draw_vline(i16 x, i16 y, u8 h, u8 color);
float $floor(float x);
void  $format(char[]& dst, char[] prog& fmt, ...);
u32   $generate_random_seed();
u8    $get_pixel(u8 x, u8 y);
void  $idle();
void  $init_random_seed();
bool  $just_pressed(u8 button);
bool  $just_released(u8 button);
bool  $load();
void  $memcpy(byte[]& dst, byte[]& src);
void  $memcpy_P(byte[]& dst, byte[] prog& src);
u32   $millis();
float $mod(float x, float y);
void  $music_play(music song);
bool  $music_playing();
void  $music_stop();
bool  $next_frame();
bool  $not_pressed(u8 buttons);
void  $poll_buttons();
float $pow(float x, float y);
bool  $pressed(u8 buttons);
u32   $random();
u32   $random_range(u32 lo, u32 hi);
float $round(float x);
void  $save();
bool  $save_exists();
void  $set_frame_rate(u8 fps);
void  $set_text_color(u8 color);
void  $set_text_font(font f);
float $sin(float angle);
u16   $sprites_frames(sprites s);
u8    $sprites_height(sprites s);
u8    $sprites_width(sprites s);
float $sqrt(float x);
i8    $strcmp(char[]& str0, char[]& str1);
i8    $strcmp_P(char[]& str0, char[] prog& str1);
i8    $strcmp_PP(char[] prog& str0, char[] prog& str1);
char[]& $strcpy(char[]& dst, char[]& src);
char[]& $strcpy_P(char[]& dst, char[] prog& src);
u16   $strlen(char[]& str);
u24   $strlen_P(char[] prog& str);
float $tan(float angle);
u16   $text_width(char[]& str);
u16   $text_width_P(char[] prog& str);
void  $tones_play(tones sfx);
void  $tones_play_auto(tones sfx);
void  $tones_play_primary(tones sfx);
bool  $tones_playing();
void  $tones_stop();
void  $wrap_text(char[]& str, u8 w);
```

