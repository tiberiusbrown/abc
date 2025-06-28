- [Graphics](#graphics)
  - [`$display`](#_display)
  - [`$display_noclear`](#_display_noclear)
  - [`$draw_circle`](#_draw_circle)
  - [`$draw_filled_circle`](#_draw_filled_circle)
  - [`$draw_filled_rect`](#_draw_filled_rect)
  - [`$draw_hline`](#_draw_hline)
  - [`$draw_line`](#_draw_line)
  - [`$draw_pixel`](#_draw_pixel)
  - [`$draw_rect`](#_draw_rect)
  - [`$draw_sprite`](#_draw_sprite)
  - [`$draw_sprite_selfmask`](#_draw_sprite_selfmask)
  - [`$draw_text`](#_draw_text)
  - [`$draw_textf`](#_draw_textf)
  - [`$draw_tilemap`](#_draw_tilemap)
  - [`$draw_vline`](#_draw_vline)
  - [`$get_pixel`](#_get_pixel)
  - [`$set_frame_rate`](#_set_frame_rate)
  - [`$set_text_color`](#_set_text_color)
  - [`$set_text_font`](#_set_text_font)
  - [`$sprites_frames`](#_sprites_frames)
  - [`$sprites_height`](#_sprites_height)
  - [`$sprites_width`](#_sprites_width)
  - [`$text_width`](#_text_width)
  - [`$tilemap_get`](#_tilemap_get)
  - [`$tilemap_height`](#_tilemap_height)
  - [`$tilemap_width`](#_tilemap_width)
  - [`$wrap_text`](#_wrap_text)
- [Sound](#sound)
  - [`$audio_enabled`](#_audio_enabled)
  - [`$audio_playing`](#_audio_playing)
  - [`$audio_stop`](#_audio_stop)
  - [`$audio_toggle`](#_audio_toggle)
  - [`$music_play`](#_music_play)
  - [`$music_playing`](#_music_playing)
  - [`$music_stop`](#_music_stop)
  - [`$tones_play`](#_tones_play)
  - [`$tones_play_auto`](#_tones_play_auto)
  - [`$tones_play_primary`](#_tones_play_primary)
  - [`$tones_playing`](#_tones_playing)
  - [`$tones_stop`](#_tones_stop)
- [Buttons](#buttons)
  - [`$any_pressed`](#_any_pressed)
  - [`$buttons`](#_buttons)
  - [`$just_pressed`](#_just_pressed)
  - [`$just_released`](#_just_released)
  - [`$not_pressed`](#_not_pressed)
  - [`$pressed`](#_pressed)
- [Math](#math)
  - [`$atan2`](#_atan2)
  - [`$ceil`](#_ceil)
  - [`$cos`](#_cos)
  - [`$floor`](#_floor)
  - [`$mod`](#_mod)
  - [`$pow`](#_pow)
  - [`$round`](#_round)
  - [`$sin`](#_sin)
  - [`$sqrt`](#_sqrt)
  - [`$tan`](#_tan)
- [Strings](#strings)
  - [`$format`](#_format)
  - [`$strcmp`](#_strcmp)
  - [`$strcpy`](#_strcpy)
  - [`$strlen`](#_strlen)
- [Utility](#utility)
  - [`$assert`](#_assert)
  - [`$debug_break`](#_debug_break)
  - [`$debug_printf`](#_debug_printf)
  - [`$idle`](#_idle)
  - [`$memcpy`](#_memcpy)
  - [`$memset`](#_memset)
  - [`$millis`](#_millis)
- [Random](#random)
  - [`$generate_random_seed`](#_generate_random_seed)
  - [`$init_random_seed`](#_init_random_seed)
  - [`$random`](#_random)
  - [`$random_range`](#_random_range)
  - [`$set_random_seed`](#_set_random_seed)
- [Save/Load](#save_load)
  - [`$load`](#_load)
  - [`$save`](#_save)
  - [`$save_exists`](#_save_exists)

# Graphics

## `$display`

```c
void $display();
```

Call this function once at the end of each frame to update the display and inputs. This function does the following in sequence:
  1. Push the contents of the display buffer to the display.
  2. Clear the contents of the display buffer to all black pixels.
  3. Wait for frame timing (see `$set_frame_rate`).
  4. Poll button states.
## `$display_noclear`

```c
void $display_noclear();
```

When not using grayscale, this function behaves exactly like `$display` except that the contents of the display buffer are left unmodified after pushing to the display. In grayscale modes, this function behaves exactly like `$display`, clearing the display buffer to all `BLACK` pixels.
## `$draw_circle`

```c
void $draw_circle(i16 x, i16 y, u8 r, u8 color);
```

Draws a single pixel thick outline of a circle to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the center of the circle. |
| **y** | The y-coorindate of the center of the circle. |
| **r** | The radius of the circle in pixels. |
| **color** | The color of the circle. |
## `$draw_filled_circle`

```c
void $draw_filled_circle(i16 x, i16 y, u8 r, u8 color);
```

Draws a filled circle to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the center of the circle. |
| **y** | The y-coorindate of the center of the circle. |
| **r** | The radius of the circle in pixels. |
| **color** | The color of the circle. |
## `$draw_filled_rect`

```c
void $draw_filled_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
```

Draws a filled rectangle to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left side of the rectangle. |
| **y** | The y-coordinate of the top side of the rectangle. |
| **w** | The width of the rectangle in pixels. |
| **h** | The height of the rectangle in pixels. |
| **color** | The color of the rectangle. |
## `$draw_hline`

```c
void $draw_hline(i16 x, i16 y, u8 w, u8 color);
```

Draws a horizontal line of a given color to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the leftmost pixel of the line. |
| **y** | The y-coordinate of the line. |
| **w** | The width of the line in pixels. |
| **color** | The color (`BLACK` or `WHITE`) of the line. |
## `$draw_line`

```c
void $draw_line(i16 x0, i16 y0, i16 x1, i16 y1, u8 color);
```

Draws a line between two arbitrary points to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x0** | The x-coordinate of the start of the line. |
| **y0** | The y-coordinate of the start of the line. |
| **x1** | The x-coordinate of the end of the line. |
| **y1** | The y-coordinate of the end of the line. |
| **color** | The color (`BLACK` or `WHITE`) of the line. |
## `$draw_pixel`

```c
void $draw_pixel(i16 x, i16 y, u8 color);
```

Draws a single pixel of a given color to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the pixel. |
| **y** | The y-coorindate of the pixel.The color (`BLACK` or `WHITE`) of the pixel. |
## `$draw_rect`

```c
void $draw_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
```

Draws a single pixel thick outline of a rectangle to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left side of the rectangle. |
| **y** | The y-coordinate of the top side of the rectangle. |
| **w** | The width of the rectangle in pixels. |
| **h** | The height of the rectangle in pixels. |
| **color** | The color of the rectangle. |
## `$draw_sprite`

```c
void $draw_sprite(i16 x, i16 y, sprites s, u16 frame);
```

Draws a sprite to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coorindate of the top side of the sprite. |
| **y** | The y-coordinate of the left side of the sprite. |
| **s** | The sprite set for the sprite to draw. |
| **frame** | The frame of the sprite to draw form the sprite set. |
## `$draw_sprite_selfmask`

```c
void $draw_sprite_selfmask(i16 x, i16 y, sprites s, u16 frame);
```

Draws a sprite to the display buffer. If the sprite is unmasked, the sprite is masked by its own white pixels. If the sprite is masked, the sprite is masked as normal. In grayscale modes, this function behaves exactly as `$draw_sprite`.

| Parameter | Description |
| -- | -- |
| **x** | The x-coorindate of the top side of the sprite. |
| **y** | The y-coordinate of the left side of the sprite. |
| **s** | The sprite set for the sprite to draw. |
| **frame** | The frame of the sprite to draw form the sprite set. |
## `$draw_text`

```c
void $draw_text(i16 x, i16 y, char[]& text);
void $draw_text(i16 x, i16 y, char[] prog& text);
```

Draws some text to the display buffer. The font and color that were previously set by `$set_text_font` and `$set_text_color` are used. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left edge of the first character in the text. When drawing, newline characters will set the x-coordinate of the next character to this value. |
| **y** | The y-coordinate of the baseline of the first character in the text. |
| **text** | The text to draw. |
## `$draw_textf`

```c
void $draw_textf(i16 x, i16 y, char[] prog& fmt, ...);
```

Draws some formatted text to the display buffer. The formatting supports a limited subset of `printf`-style format strings.

| Specifier | Description |
| -- | -- |
| `%%` | A single '%' character. |
| `%d` | A signed decimal integer (`i32`). |
| `%u` | An unsigned decimal integer (`u32`). |
| `%x` | An unsigned hexadecimal integer (`u32`). |
| `%f` | A floating-point number (`float`). |
| `%c` | A single character (`char`). |
| `%s` | A text string (`char[]&` or `char[] prog&`). |

The `%d`, `%u`, and `%x` specifiers support zero-padding to a given width, up to 9; for example, `%04u` would print 42 as "0042". The `%f` specifier supports a precision modifier of up to 9 digits for decimal fractions; for example, `.3f` would print 3.14159 as "3.142". The font and color that were previously set by `$set_text_font` and `$set_text_color` are used. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left edge of the first character in the text. When drawing, newline characters will set the x-coordinate of the next character to this value. |
| **y** | The y-coordinate of the baseline of the first character in the text. |
| **fmt** | The format string to use for constructing the text to draw. |
## `$draw_tilemap`

```c
void $draw_tilemap(i16 x, i16 y, sprites s, tilemap tm);
```


## `$draw_vline`

```c
void $draw_vline(i16 x, i16 y, u8 h, u8 color);
```

Draws a vertical line of a given color to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the line. |
| **y** | The y-coordinate of topmost pixel of the line. |
| **h** | The height of the line in pixels. |
| **color** | The color (`BLACK` or `WHITE`) of the line. |
## `$get_pixel`

```c
u8 $get_pixel(u8 x, u8 y);
```

Retrieves the color of a single pixel from the display buffer. In grayscale modes, this function always returns `BLACK`.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the pixel to test. |
| **y** | The y-coordinate of the pixel to test. |

### Return Value

The color of the pixel: either `BLACK` or `WHITE`.
## `$set_frame_rate`

```c
void $set_frame_rate(u8 fps);
```

Set the target frame rate (see `$display`).

| Parameter | Description |
| -- | -- |
| **fps** | The frame rate in frames per second. |
## `$set_text_color`

```c
void $set_text_color(u8 color);
```

Set the color used by subsequence text drawing functions.

| Parameter | Description |
| -- | -- |
| **color** | The color to use in subsequent text functions. |
## `$set_text_font`

```c
void $set_text_font(font f);
```

Set the font used by subsequent text functions.

| Parameter | Description |
| -- | -- |
| **f** | The font to use in subsequent text functions. |
## `$sprites_frames`

```c
u16 $sprites_frames(sprites s);
```

Get the number of sprite frames in a sprite set.

| Parameter | Description |
| -- | -- |
| **s** | The sprite set. |

### Return Value

The number of sprite frames in the sprite set.
## `$sprites_height`

```c
u8 $sprites_height(sprites s);
```

Get the sprite height for a sprite set.

| Parameter | Description |
| -- | -- |
| **s** | The sprite set. |

### Return Value

The sprite height for the sprite set.
## `$sprites_width`

```c
u8 $sprites_width(sprites s);
```

Get the sprite width for a sprite set.

| Parameter | Description |
| -- | -- |
| **s** | The sprite set. |

### Return Value

The sprite width for the sprite set.
## `$text_width`

```c
u16 $text_width(char[]& text);
u16 $text_width(char[] prog& text);
```

Gets the width of some text in pixels. The font that was previously set by `$set_text_font` is used for character widths.

| Parameter | Description |
| -- | -- |
| **text** | The text to compute the width. |

### Return Value

The width of the text in pixels.
## `$tilemap_get`

```c
u16 $tilemap_get(tilemap tm, u16 x, u16 y);
```

Get the tile at the given coordinates in a tilemap. Tilemap coordinates are unsigned. The x-coordinate runs left-to-right and the y-coordinate runs top-to-bottom.

| Parameter | Description |
| -- | -- |
| **tm** | The x-coordinate of the tile to get. |
| **x** | The y-coordinate of the tile to get. |
## `$tilemap_height`

```c
u16 $tilemap_height(tilemap tm);
```

Get the height of a tilemap in tiles.

| Parameter | Description |
| -- | -- |
| **tm** | The tilemap. |

### Return Value

The height of the tilemap in tiles.
## `$tilemap_width`

```c
u16 $tilemap_width(tilemap tm);
```

Get the width of a tilemap in tiles.

| Parameter | Description |
| -- | -- |
| **tm** | The tilemap. |

### Return Value

The width of the tilemap in tiles.
## `$wrap_text`

```c
u16 $wrap_text(char[]& text, u8 w);
```

Word-wrap some text in-place by replacing space characters with newlines. The font that was previously set by `$set_text_font` is used for character widths.

| Parameter | Description |
| -- | -- |
| **text** | The text to word-wrap. |
| **w** | The wrap width in pixels. |

# Sound

## `$audio_enabled`

```c
bool $audio_enabled();
```

Get whether music and tones are able to play.

### Return Value

`true` if music and tones are able to play.
## `$audio_playing`

```c
bool $audio_playing();
```

Get whether any music or tones are playing on any channel.

### Return Value

`true` if any music or tones are playing on any channel.
## `$audio_stop`

```c
void $audio_stop();
```

Stop any music or tones that are currently playing on any channel.
## `$audio_toggle`

```c
void $audio_toggle();
```

Toggle whether music and tones are able to play.
## `$music_play`

```c
void $music_play(music song);
```

Start playing some music. Any previoously playing music will be stopped.

| Parameter | Description |
| -- | -- |
| **song** | The music to play. |
## `$music_playing`

```c
bool $music_playing();
```

Get whether any music is playing.

### Return Value

`true` if any music is playing.
## `$music_stop`

```c
void $music_stop();
```

Stop any music that is currently playing.
## `$tones_play`

```c
void $tones_play(tones sfx);
```

Play some tones on the tones channel. Any tones previously playing on the tones channel will be stopped.

| Parameter | Description |
| -- | -- |
| **sfx** | The tones to play. |
## `$tones_play_auto`

```c
void $tones_play_auto(tones sfx);
```

Play some tones on either the tones channel or the primary music channel. The primary music channel will be used only if the tones channel is occupied and the primary music channel is unoccupied. This is useful for situations where music is not playing; it allows two tones to play simultaneously.
## `$tones_play_primary`

```c
void $tones_play_primary(tones sfx);
```

Play some tones on the primary music channel. Any tones on the primary channel or music previously playing will be stopped. This is useful when some tones need to play that should not be interrupted by future calls to `$tones_play`.

| Parameter | Description |
| -- | -- |
| **sfx** | The tones to play.  |
## `$tones_playing`

```c
bool $tones_playing();
```

Get whether tones are playing on any channel.

### Return Value

`true` if tones are playing on any channel.
## `$tones_stop`

```c
void $tones_stop();
```

Stop any tones playing on any channel.

# Buttons

## `$any_pressed`

```c
bool $any_pressed(u8 buttons);
```

Test if any of the specified buttons are pressed.

| Parameter | Description |
| -- | -- |
| **buttons** | A bit mask indicating which buttons to test (can be a single button). |

### Return Value

`true` if *one or more* buttons in the provided mask are currently pressed.
## `$buttons`

```c
u8 $buttons();
```

Gets the state of the buttons combined into a single mask. The bit for each button is `1` when the button is pressed.
## `$just_pressed`

```c
bool $just_pressed(u8 button);
```

Test if a button has just been pressed since the last frame.

| Parameter | Description |
| -- | -- |
| **button** | The button to test for. Only one button should be specified. |

### Return Value

`true` if the specified button has just been pressed since the last frame.
## `$just_released`

```c
bool $just_released(u8 button);
```

Test if a button has just been released since the last frame.

| Parameter | Description |
| -- | -- |
| **button** | The button to test for. Only one button should be specified. |

### Return Value

`true` if the specified button has just been released since the last frame.
## `$not_pressed`

```c
bool $not_pressed(u8 buttons);
```

Test if all of the specified buttons are not pressed.

| Parameter | Description |
| -- | -- |
| **buttons** | A bit mask indicating which buttons to test (can be a single button). |

### Return Value

`true` if *all* buttons in the provided mask are currently released.
## `$pressed`

```c
bool $pressed(u8 buttons);
```

Test if all of the specified buttons are pressed.

| Parameter | Description |
| -- | -- |
| **buttons** | A bit mask indicating which buttons to test (can be a single button). |

### Return Value

`true` if *all* buttons in the provided mask are currently pressed.

# Math

## `$atan2`

```c
float $atan2(float y, float x);
```

Get the angle between the positive x-axis and the ray from the origin to the point (x, y) on the Cartesian plane.

| Parameter | Description |
| -- | -- |
| **y** | The y-coordinate of the ray endpoint on the Cartesian plane. |
| **x** | The x-coordinate of the ray endpoint on the Cartesian plane. |

### Return Value

The angle between the positive x-axis and the ray from the origin to the point (x, y) on the Cartesian plane.
## `$ceil`

```c
float $ceil(float x);
```

Get the least integer not less than a number.

| Parameter | Description |
| -- | -- |
| **x** | The number. |

### Return Value

The least integer not less than the number.
## `$cos`

```c
float $cos(float angle);
```

Get the cosine of an angle.

| Parameter | Description |
| -- | -- |
| **angle** | The angle in radians. |

### Return Value

The cosine of the angle.
## `$floor`

```c
float $floor(float x);
```

Get the greatest integer not greater than a number.

| Parameter | Description |
| -- | -- |
| **x** | The number. |

### Return Value

The greatest integer not greater than the number.
## `$mod`

```c
float $mod(float x, float y);
```

Get the remainder of a division.

| Parameter | Description |
| -- | -- |
| **x** | The numerator of the division. |
| **y** | The denominator of the division. |

### Return Value

The remainder of the division.
## `$pow`

```c
float $pow(float x, float y);
```

Get the result of an exponentiation.

| Parameter | Description |
| -- | -- |
| **x** | The base of the exponentiation. |
| **y** | The exponent of the exponentiation. |

### Return Value

The result of the exponentiation.
## `$round`

```c
float $round(float x);
```

Get the integer closest to a number, choosing the greater if equidistant from two integers.

| Parameter | Description |
| -- | -- |
| **x** | The number. |

### Return Value

The integer closest to the number, choosing the greater if equidistant from two integers.
## `$sin`

```c
float $sin(float angle);
```

Get the sine of an angle.

| Parameter | Description |
| -- | -- |
| **angle** | The angle in radians. |

### Return Value

The sine of the angle.
## `$sqrt`

```c
float $sqrt(float x);
```

Get the square root of a number.

| Parameter | Description |
| -- | -- |
| **x** | The number. |

### Return Value

The square root of the number.
## `$tan`

```c
float $tan(float angle);
```

Get the tangent of an angle.

| Parameter | Description |
| -- | -- |
| **angle** | The angle in radians. |

### Return Value

The tangent of the angle.

# Strings

## `$format`

```c
void $format(char[]& dst, char[] prog& fmt, ...);
```

Copy formatted text into a text string. The formatting supports a limited subset of `printf`-style format strings.

| Specifier | Description |
| -- | -- |
| `%%` | A single '%' character. |
| `%d` | A signed decimal integer (`i32`). |
| `%u` | An unsigned decimal integer (`u32`). |
| `%x` | An unsigned hexadecimal integer (`u32`). |
| `%f` | A floating-point number (`float`). |
| `%c` | A single character (`char`). |
| `%s` | A text string (`char[]&` or `char[] prog&`). |

The `%d`, `%u`, and `%x` specifiers support zero-padding to a given width, up to 9; for example, `%04u` would print 42 as "0042". The `%f` specifier supports a precision modifier of up to 9 digits for decimal fractions; for example, `.3f` would print 3.14159 as "3.142".

| Parameter | Description |
| -- | -- |
| **dst** | The destination text string. |
| **fmt** | The format string to use for constructing the destination text string. |
## `$strcmp`

```c
i8 $strcmp(char[]& str0, char[]& str1);
i8 $strcmp(char[]& str0, char[] prog& str1);
i8 $strcmp(char[] prog& str0, char[] prog& str1);
```

Compare two text strings against each other lexicographically.

| Parameter | Description |
| -- | -- |
| **str0** | The first string to compare. |
| **str1** | The second string to compare. |

### Return Value

An integral value indicating the result of the comparison. A zero value indicates the two strings are equal. A negative or positive value indicates the first string is lexicographically less than or greater than the second string, respectively.
## `$strcpy`

```c
char[]& $strcpy(char[]& dst, char[]& src);
char[]& $strcpy(char[]& dst, char[] prog& src);
```

Copy one text string to another. The two text strings may be different lengths or capacities.

| Parameter | Description |
| -- | -- |
| **dst** | The destination text string. |
| **src** | The source text string. |

### Return Value

A reference to the destination text string.
## `$strlen`

```c
u16 $strlen(char[]& str);
u24 $strlen(char[] prog& str);
```

Get the length of a text string in characters. Use the `len` operator to get the capacity of a text string in characters.

| Parameter | Description |
| -- | -- |
| **str** | The text string. |

### Return Value

The length of the text string in characters.

# Utility

## `$assert`

```c
void $assert(bool cond);
```

Runtime assertion for debug purposes. If the asserted condition evaluates to `false`, execution halts and a stack trace is displayed.

| Parameter | Description |
| -- | -- |
| **cond** | The condition to check. |
## `$debug_break`

```c
void $debug_break();
```

Issue an AVR `break` instruction. This can be useful for debugging with an emulator or hardware debugger.
## `$debug_printf`

```c
void $debug_printf(char[] prog& fmt, ...);
```

Output some formatted text to the serial console. This function does not work on a physical Arduboy FX, as the interpreter does not include a USB software stack.

| Parameter | Description |
| -- | -- |
| **fmt** | The format string to use for constructing the text to output. |
## `$idle`

```c
void $idle();
```

Do nothing for a short time (one millisecond or less) and the CPU in a low power mode. This can be useful for waiting to respond to a button press without calling `$display` and without utilizing the CPU at 100%. Most games do not need to use this function.
## `$memcpy`

```c
void $memcpy(byte[]& dst, byte[]& src);
void $memcpy(byte[]& dst, byte[] prog& src);
```

Copy one byte array to another. The two byte arrays must be the same size.

| Parameter | Description |
| -- | -- |
| **dst** | The destination byte array. |
| **src** | The source byte array. |
## `$memset`

```c
void $memset(byte[]& dst, u8 val);
```

Set each byte of some byte array to a single value.

| Parameter | Description |
| -- | -- |
| **dst** | The byte array to modify. |
| **val** | The value to copy to each byte. |
## `$millis`

```c
u32 $millis();
```

Gets the time elapsed in milliseconds since program start.

### Return Value

The time in milliseconds since program start.

# Random

## `$generate_random_seed`

```c
u32 $generate_random_seed();
```

Get a random value generated from true entropy.

### Return Value

A random value generated from true entropy.
## `$init_random_seed`

```c
void $init_random_seed();
```

Initialize the internal random seed with a value generated from true entropy.
## `$random`

```c
u32 $random();
```

Get a 32-bit random value generated from the internal random seed.

### Return Value

A 32-bit random value generated from the internal random seed.
## `$random_range`

```c
u32 $random_range(u32 lo, u32 hi);
```

Get a 32-bit random value generated from the internal random seed constrained to fall within a given range. The range is inclusive; `$random_range(1, 3)` will produce either 1, 2, or 3.

| Parameter | Description |
| -- | -- |
| **lo** | The lower bound of the constrained range, inclusive. |
| **hi** | The upper bound of the constrained range, inclusive. |

### Return Value

A 32-bit random value generated from the internal random seed constrained to fall within the given range.
## `$set_random_seed`

```c
void $set_random_seed(u32 seed);
```

Set the internal random seed.

| Parameter | Description |
| -- | -- |
| **seed** | The random seed. |

# Save/Load

## `$load`

```c
bool $load();
```

Overwrite all `saved` global variables from the save file, if it exists.

### Return Value

`true` if the save file exists and the global variables were overwritten.
## `$save`

```c
void $save();
```

Save all `saved` global variables to the save file.
## `$save_exists`

```c
bool $save_exists();
```

Get whether a save file exists.

### Return Value

`true` if a save file exists.


