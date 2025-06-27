# Graphics

<details><summary>`$display`</summary>

### Declaration

```c
void $display();
```

### Description

Call this function once at the end of each frame to update the display and inputs. This function does the following in sequence:
  1. Push the contents of the display buffer to the display.
  2. Clear the contents of the display buffer to all black pixels.
  3. Wait for frame timing (see `$set_frame_rate`).
  4. Poll button states.
</details>
<details><summary>`$display_noclear`</summary>

### Declaration

```c
void $display_noclear();
```

### Description

When not using grayscale, this function behaves exactly like `$display` except that the contents of the display buffer are left unmodified after pushing to the display. In grayscale modes, this function behaves exactly like `$display`, clearing the display buffer to all `BLACK` pixels.
</details>
<details><summary>`$draw_circle`</summary>

### Declaration

```c
void $draw_circle(i16 x, i16 y, u8 r, u8 color);
```

### Description

Draws a single pixel thick outline of a circle to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the center of the circle. |
| **y** | The y-coorindate of the center of the circle. |
| **r** | The radius of the circle in pixels. |
| **color** | The color of the circle. |
</details>
<details><summary>`$draw_filled_circle`</summary>

### Declaration

```c
void $draw_filled_circle(i16 x, i16 y, u8 r, u8 color);
```

### Description

Draws a filled circle to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the center of the circle. |
| **y** | The y-coorindate of the center of the circle. |
| **r** | The radius of the circle in pixels. |
| **color** | The color of the circle. |
</details>
<details><summary>`$draw_filled_rect`</summary>

### Declaration

```c
void $draw_filled_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
```

### Description

Draws a filled rectangle to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left side of the rectangle. |
| **y** | The y-coordinate of the top side of the rectangle. |
| **w** | The width of the rectangle in pixels. |
| **h** | The height of the rectangle in pixels. |
| **color** | The color of the rectangle. |
</details>
<details><summary>`$draw_hline`</summary>

### Declaration

```c
void $draw_hline(i16 x, i16 y, u8 w, u8 color);
```

### Description

Draws a horizontal line of a given color to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the leftmost pixel of the line. |
| **y** | The y-coordinate of the line. |
| **w** | The width of the line in pixels. |
| **color** | The color (`BLACK` or `WHITE`) of the line. |
</details>
<details><summary>`$draw_line`</summary>

### Declaration

```c
void $draw_line(i16 x0, i16 y0, i16 x1, i16 y1, u8 color);
```

### Description

Draws a line between two arbitrary points to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x0** | The x-coordinate of the start of the line. |
| **y0** | The y-coordinate of the start of the line. |
| **x1** | The x-coordinate of the end of the line. |
| **y1** | The y-coordinate of the end of the line. |
| **color** | The color (`BLACK` or `WHITE`) of the line. |
</details>
<details><summary>`$draw_pixel`</summary>

### Declaration

```c
void $draw_pixel(i16 x, i16 y, u8 color);
```

### Description

Draws a single pixel of a given color to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the pixel. |
| **y** | The y-coorindate of the pixel.The color (`BLACK` or `WHITE`) of the pixel. |
</details>
<details><summary>`$draw_rect`</summary>

### Declaration

```c
void $draw_rect(i16 x, i16 y, u8 w, u8 h, u8 color);
```

### Description

Draws a single pixel thick outline of a rectangle to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left side of the rectangle. |
| **y** | The y-coordinate of the top side of the rectangle. |
| **w** | The width of the rectangle in pixels. |
| **h** | The height of the rectangle in pixels. |
| **color** | The color of the rectangle. |
</details>
<details><summary>`$draw_sprite`</summary>

### Declaration

```c
void $draw_sprite(i16 x, i16 y, sprites s, u16 frame);
```

### Description

Draws a sprite to the display buffer. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coorindate of the top side of the sprite. |
| **y** | The y-coordinate of the left side of the sprite. |
| **s** | The sprite set for the sprite to draw. |
| **frame** | The frame of the sprite to draw form the sprite set. |
</details>
<details><summary>`$draw_sprite_selfmask`</summary>

### Declaration

```c
void $draw_sprite_selfmask(i16 x, i16 y, sprites s, u16 frame);
```

### Description

Draws a sprite to the display buffer. If the sprite is unmasked, the sprite is masked by its own white pixels. If the sprite is masked, the sprite is masked as normal. In grayscale modes, this function behaves exactly as `$draw_sprite`.

| Parameter | Description |
| -- | -- |
| **x** | The x-coorindate of the top side of the sprite. |
| **y** | The y-coordinate of the left side of the sprite. |
| **s** | The sprite set for the sprite to draw. |
| **frame** | The frame of the sprite to draw form the sprite set. |
</details>
<details><summary>`$draw_text`</summary>

### Declaration

```c
void $draw_text(i16 x, i16 y, char[]& text);
void $draw_text(i16 x, i16 y, char[] prog& text);
```

### Description

Draws some text to the display buffer. The font and color that were previously set by `$set_text_font` and `$set_text_color` are used. 

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the left edge of the first character in the text. When drawing, newline characters will set the x-coordinate of the next character to this value. |
| **y** | The y-coordinate of the baseline of the first character in the text. |
| **text** | The text to draw. |
</details>
<details><summary>`$draw_textf`</summary>

### Declaration

```c
void $draw_textf(i16 x, i16 y, char[] prog& fmt, ...);
```

### Description

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
</details>
<details><summary>`$draw_tilemap`</summary>

### Declaration

```c
void $draw_tilemap(i16 x, i16 y, sprites s, tilemap tm);
```

### Description


</details>
<details><summary>`$draw_vline`</summary>

### Declaration

```c
void $draw_vline(i16 x, i16 y, u8 h, u8 color);
```

### Description

Draws a vertical line of a given color to the display buffer. This function does nothing in grayscale modes.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the line. |
| **y** | The y-coordinate of topmost pixel of the line. |
| **h** | The height of the line in pixels. |
| **color** | The color (`BLACK` or `WHITE`) of the line. |
</details>
<details><summary>`$get_pixel`</summary>

### Declaration

```c
u8 $get_pixel(u8 x, u8 y);
```

### Description

Retrieves the color of a single pixel from the display buffer. In grayscale modes, this function always returns `BLACK`.

| Parameter | Description |
| -- | -- |
| **x** | The x-coordinate of the pixel to test. |
| **y** | The y-coordinate of the pixel to test. |

### Return Value

The color of the pixel: either `BLACK` or `WHITE`.
</details>
<details><summary>`$set_frame_rate`</summary>

### Declaration

```c
void $set_frame_rate(u8 fps);
```

### Description

Set the target frame rate (see `$display`).

| Parameter | Description |
| -- | -- |
| **fps** | The frame rate in frames per second. |
</details>
<details><summary>`$set_text_color`</summary>

### Declaration

```c
void $set_text_color(u8 color);
```

### Description

Set the color used by subsequence text drawing functions.

| Parameter | Description |
| -- | -- |
| **color** | The color to use in subsequent text functions. |
</details>
<details><summary>`$set_text_font`</summary>

### Declaration

```c
void $set_text_font(font f);
```

### Description

Set the font used by subsequent text functions.

| Parameter | Description |
| -- | -- |
| **f** | The font to use in subsequent text functions. |
</details>
<details><summary>`$sprites_frames`</summary>

### Declaration

```c
u16 $sprites_frames(sprites s);
```

### Description

Get the number of sprite frames in a sprite set.

| Parameter | Description |
| -- | -- |
| **s** | The sprite set. |

### Return Value

The number of sprite frames in the sprite set.
</details>
<details><summary>`$sprites_height`</summary>

### Declaration

```c
u8 $sprites_height(sprites s);
```

### Description

Get the sprite height for a sprite set.

| Parameter | Description |
| -- | -- |
| **s** | The sprite set. |

### Return Value

The sprite height for the sprite set.
</details>
<details><summary>`$sprites_width`</summary>

### Declaration

```c
u8 $sprites_width(sprites s);
```

### Description

Get the sprite width for a sprite set.

| Parameter | Description |
| -- | -- |
| **s** | The sprite set. |

### Return Value

The sprite width for the sprite set.
</details>
<details><summary>`$text_width`</summary>

### Declaration

```c
u16 $text_width(char[]& text);
u16 $text_width(char[] prog& text);
```

### Description

Gets the width of some text in pixels. The font that was previously set by `$set_text_font` is used for character widths.

| Parameter | Description |
| -- | -- |
| **text** | The text to compute the width. |

### Return Value

The width of the text in pixels.
</details>
<details><summary>`$tilemap_get`</summary>

### Declaration

```c
u16 $tilemap_get(tilemap tm, u16 x, u16 y);
```

### Description

Get the tile at the given coordinates in a tilemap. Tilemap coordinates are unsigned. The x-coordinate runs left-to-right and the y-coordinate runs top-to-bottom.

| Parameter | Description |
| -- | -- |
| **tm** | The x-coordinate of the tile to get. |
| **x** | The y-coordinate of the tile to get. |
</details>
<details><summary>`$tilemap_height`</summary>

### Declaration

```c
u16 $tilemap_height(tilemap tm);
```

### Description

Get the height of a tilemap in tiles.

| Parameter | Description |
| -- | -- |
| **tm** | The tilemap. |

### Return Value

The height of the tilemap in tiles.
</details>
<details><summary>`$tilemap_width`</summary>

### Declaration

```c
u16 $tilemap_width(tilemap tm);
```

### Description

Get the width of a tilemap in tiles.

| Parameter | Description |
| -- | -- |
| **tm** | The tilemap. |

### Return Value

The width of the tilemap in tiles.
</details>
<details><summary>`$wrap_text`</summary>

### Declaration

```c
u16 $wrap_text(char[]& text, u8 w);
```

### Description

Word-wrap some text in-place by replacing space characters with newlines. The font that was previously set by `$set_text_font` is used for character widths.

| Parameter | Description |
| -- | -- |
| **text** | The text to word-wrap. |
| **w** | The wrap width in pixels. |
</details>

# Sound

<details><summary>`$audio_enabled`</summary>

### Declaration

```c
bool $audio_enabled();
```

### Description


</details>
<details><summary>`$audio_playing`</summary>

### Declaration

```c
bool $audio_playing();
```

### Description


</details>
<details><summary>`$audio_stop`</summary>

### Declaration

```c
void $audio_stop();
```

### Description


</details>
<details><summary>`$audio_toggle`</summary>

### Declaration

```c
void $audio_toggle();
```

### Description


</details>
<details><summary>`$music_play`</summary>

### Declaration

```c
void $music_play(music song);
```

### Description


</details>
<details><summary>`$music_playing`</summary>

### Declaration

```c
bool $music_playing();
```

### Description


</details>
<details><summary>`$music_stop`</summary>

### Declaration

```c
void $music_stop();
```

### Description


</details>
<details><summary>`$tones_play`</summary>

### Declaration

```c
void $tones_play(tones sfx);
```

### Description


</details>
<details><summary>`$tones_play_auto`</summary>

### Declaration

```c
void $tones_play_auto(tones sfx);
```

### Description


</details>
<details><summary>`$tones_play_primary`</summary>

### Declaration

```c
void $tones_play_primary(tones sfx);
```

### Description


</details>
<details><summary>`$tones_playing`</summary>

### Declaration

```c
bool $tones_playing();
```

### Description


</details>
<details><summary>`$tones_stop`</summary>

### Declaration

```c
void $tones_stop();
```

### Description


</details>

# Buttons

<details><summary>`$any_pressed`</summary>

### Declaration

```c
bool $any_pressed(u8 buttons);
```

### Description

Test if any of the specified buttons are pressed.

| Parameter | Description |
| -- | -- |
| **buttons** | A bit mask indicating which buttons to test (can be a single button). |

### Return Value

`true` if *one or more* buttons in the provided mask are currently pressed.
</details>
<details><summary>`$buttons`</summary>

### Declaration

```c
u8 $buttons();
```

### Description

Gets the state of the buttons combined into a single mask. The bit for each button is `1` when the button is pressed.
</details>
<details><summary>`$just_pressed`</summary>

### Declaration

```c
bool $just_pressed(u8 button);
```

### Description

Test if a button has just been pressed since the last frame.

| Parameter | Description |
| -- | -- |
| **button** | The button to test for. Only one button should be specified. |

### Return Value

`true` if the specified button has just been pressed since the last frame.
</details>
<details><summary>`$just_released`</summary>

### Declaration

```c
bool $just_released(u8 button);
```

### Description

Test if a button has just been released since the last frame.

| Parameter | Description |
| -- | -- |
| **button** | The button to test for. Only one button should be specified. |

### Return Value

`true` if the specified button has just been released since the last frame.
</details>
<details><summary>`$not_pressed`</summary>

### Declaration

```c
bool $not_pressed(u8 buttons);
```

### Description

Test if all of the specified buttons are not pressed.

| Parameter | Description |
| -- | -- |
| **buttons** | A bit mask indicating which buttons to test (can be a single button). |

### Return Value

`true` if *all* buttons in the provided mask are currently released.
</details>
<details><summary>`$pressed`</summary>

### Declaration

```c
bool $pressed(u8 buttons);
```

### Description

Test if all of the specified buttons are pressed.

| Parameter | Description |
| -- | -- |
| **buttons** | A bit mask indicating which buttons to test (can be a single button). |

### Return Value

`true` if *all* buttons in the provided mask are currently pressed.
</details>

# Math

<details><summary>`$atan2`</summary>

### Declaration

```c
float $atan2(float y, float x);
```

### Description


</details>
<details><summary>`$ceil`</summary>

### Declaration

```c
float $ceil(float x);
```

### Description


</details>
<details><summary>`$cos`</summary>

### Declaration

```c
float $cos(float angle);
```

### Description


</details>
<details><summary>`$floor`</summary>

### Declaration

```c
float $floor(float x);
```

### Description


</details>
<details><summary>`$mod`</summary>

### Declaration

```c
float $mod(float x, float y);
```

### Description


</details>
<details><summary>`$pow`</summary>

### Declaration

```c
float $pow(float x, float y);
```

### Description


</details>
<details><summary>`$round`</summary>

### Declaration

```c
float $round(float x);
```

### Description


</details>
<details><summary>`$sin`</summary>

### Declaration

```c
float $sin(float angle);
```

### Description


</details>
<details><summary>`$sqrt`</summary>

### Declaration

```c
float $sqrt(float x);
```

### Description


</details>
<details><summary>`$tan`</summary>

### Declaration

```c
float $tan(float angle);
```

### Description


</details>

# Strings

<details><summary>`$format`</summary>

### Declaration

```c
void $format(char[]& dst, char[] prog& fmt, ...);
```

### Description

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
</details>
<details><summary>`$strcmp`</summary>

### Declaration

```c
i8 $strcmp(char[]& str0, char[]& str1);
i8 $strcmp(char[]& str0, char[] prog& str1);
i8 $strcmp(char[] prog& str0, char[] prog& str1);
```

### Description

Compare two text strings against each other lexicographically.

| Parameter | Description |
| -- | -- |
| **str0** | The first string to compare. |
| **str1** | The second string to compare. |

### Return Value

An integral value indicating the result of the comparison. A zero value indicates the two strings are equal. A negative or positive value indicates the first string is lexicographically less than or greater than the second string, respectively.
</details>
<details><summary>`$strcpy`</summary>

### Declaration

```c
char[]& $strcpy(char[]& dst, char[]& src);
char[]& $strcpy(char[]& dst, char[] prog& src);
```

### Description

Copy one text string to another. The two text strings may be different lengths or capacities.

| Parameter | Description |
| -- | -- |
| **dst** | The destination text string. |
| **src** | The source text string. |

### Return Value

A reference to the destination text string.
</details>
<details><summary>`$strlen`</summary>

### Declaration

```c
u16 $strlen(char[]& str);
u24 $strlen(char[] prog& str);
```

### Description

Get the length of a text string in characters. Use the `len` operator to get the capacity of a text string in characters.

| Parameter | Description |
| -- | -- |
| **str** | The text string. |

### Return Value

The length of the text string in characters.
</details>

# Utility

<details><summary>`$assert`</summary>

### Declaration

```c
void $assert(bool cond);
```

### Description

Runtime assertion for debug purposes. If the asserted condition evaluates to `false`, execution halts and a stack trace is displayed.

| Parameter | Description |
| -- | -- |
| **cond** | The condition to check. |
</details>
<details><summary>`$debug_break`</summary>

### Declaration

```c
void $debug_break();
```

### Description

Issue an AVR `break` instruction. This can be useful for debugging with an emulator or hardware debugger.
</details>
<details><summary>`$debug_printf`</summary>

### Declaration

```c
void $debug_printf(char[] prog& fmt, ...);
```

### Description

Output some formatted text to the serial console. This function does not work on a physical Arduboy FX, as the interpreter does not include a USB software stack.

| Parameter | Description |
| -- | -- |
| **fmt** | The format string to use for constructing the text to output. |
</details>
<details><summary>`$idle`</summary>

### Declaration

```c
void $idle();
```

### Description

Do nothing for a short time (one millisecond or less) and the CPU in a low power mode. This can be useful for waiting to respond to a button press without calling `$display` and without utilizing the CPU at 100%. Most games do not need to use this function.
</details>
<details><summary>`$memcpy`</summary>

### Declaration

```c
void $memcpy(byte[]& dst, byte[]& src);
void $memcpy(byte[]& dst, byte[] prog& src);
```

### Description

Copy one byte array to another. The two byte arrays must be the same size.

| Parameter | Description |
| -- | -- |
| **dst** | The destination byte array. |
| **src** | The source byte array. |
</details>
<details><summary>`$memset`</summary>

### Declaration

```c
void $memset(byte[]& dst, u8 val);
```

### Description

Set each byte of some byte array to a single value.

| Parameter | Description |
| -- | -- |
| **dst** | The byte array to modify. |
| **val** | The value to copy to each byte. |
</details>
<details><summary>`$millis`</summary>

### Declaration

```c
u32 $millis();
```

### Description

Gets the time elapsed in milliseconds since program start.

### Return Value

The time in milliseconds since program start.
</details>

# Random

<details><summary>`$generate_random_seed`</summary>

### Declaration

```c
u32 $generate_random_seed();
```

### Description


</details>
<details><summary>`$init_random_seed`</summary>

### Declaration

```c
void $init_random_seed();
```

### Description


</details>
<details><summary>`$random`</summary>

### Declaration

```c
u32 $random();
```

### Description


</details>
<details><summary>`$random_range`</summary>

### Declaration

```c
u32 $random_range(u32 lo, u32 hi);
```

### Description


</details>
<details><summary>`$set_random_seed`</summary>

### Declaration

```c
void $set_random_seed(u32 seed);
```

### Description


</details>

# Save/Load

<details><summary>`$load`</summary>

### Declaration

```c
bool $load();
```

### Description


</details>
<details><summary>`$save`</summary>

### Declaration

```c
void $save();
```

### Description


</details>
<details><summary>`$save_exists`</summary>

### Declaration

```c
bool $save_exists();
```

### Description


</details>


