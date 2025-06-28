#include "abc_assembler.hpp"
#include "abc_compiler.hpp"

namespace abc
{

bool sysfunc_is_format(sysfunc_t f)
{
    return f == SYS_DRAW_TEXTF || f == SYS_FORMAT || f == SYS_DEBUG_PRINTF;
}

std::unordered_map<std::string, std::vector<std::string>> const sys_overloads =
{
    { "draw_text", { "draw_text_P" }},
    { "text_width", { "text_width_P" }},
    { "memcpy", { "memcpy_P" }},
    { "strlen", { "strlen_P" }},
    { "strcmp", { "strcmp_P", "strcmp_PP" }},
    { "strcpy", { "strcpy_P" }},
};

std::unordered_map<std::string, sysfunc_t> const sys_names =
{
    { "display",              SYS_DISPLAY              },
    { "display_noclear",      SYS_DISPLAY_NOCLEAR      },
    { "get_pixel",            SYS_GET_PIXEL            },
    { "draw_pixel",           SYS_DRAW_PIXEL           },
    { "draw_hline",           SYS_DRAW_HLINE           },
    { "draw_vline",           SYS_DRAW_VLINE           },
    { "draw_line",            SYS_DRAW_LINE            },
    { "draw_rect",            SYS_DRAW_RECT            },
    { "draw_filled_rect",     SYS_DRAW_FILLED_RECT     },
    { "draw_circle",          SYS_DRAW_CIRCLE          },
    { "draw_filled_circle",   SYS_DRAW_FILLED_CIRCLE   },
    { "draw_sprite",          SYS_DRAW_SPRITE          },
    { "draw_sprite_selfmask", SYS_DRAW_SPRITE_SELFMASK },
    { "draw_tilemap",         SYS_DRAW_TILEMAP         },
    { "draw_text",            SYS_DRAW_TEXT            },
    { "draw_text_P",          SYS_DRAW_TEXT_P          },
    { "draw_textf",           SYS_DRAW_TEXTF           },
    { "text_width",           SYS_TEXT_WIDTH           },
    { "text_width_P",         SYS_TEXT_WIDTH_P         },
    { "wrap_text",            SYS_WRAP_TEXT            },
    { "set_text_font",        SYS_SET_TEXT_FONT        },
    { "set_text_color",       SYS_SET_TEXT_COLOR       },
    { "sprites_width",        SYS_SPRITES_WIDTH        },
    { "sprites_height",       SYS_SPRITES_HEIGHT       },
    { "sprites_frames",       SYS_SPRITES_FRAMES       },
    { "tilemap_width",        SYS_TILEMAP_WIDTH        },
    { "tilemap_height",       SYS_TILEMAP_HEIGHT       },
    { "tilemap_get",          SYS_TILEMAP_GET          },
    { "set_frame_rate",       SYS_SET_FRAME_RATE       },
    { "idle",                 SYS_IDLE                 },
    { "debug_break",          SYS_DEBUG_BREAK          },
    { "debug_printf",         SYS_DEBUG_PRINTF         },
    { "assert",               SYS_ASSERT               },
    { "buttons",              SYS_BUTTONS              },
    { "just_pressed",         SYS_JUST_PRESSED         },
    { "just_released",        SYS_JUST_RELEASED        },
    { "pressed",              SYS_PRESSED              },
    { "any_pressed",          SYS_ANY_PRESSED          },
    { "not_pressed",          SYS_NOT_PRESSED          },
    { "millis",               SYS_MILLIS               },
    { "memset",               SYS_MEMSET               },
    { "memcpy",               SYS_MEMCPY               },
    { "memcpy_P",             SYS_MEMCPY_P             },
    { "strlen",               SYS_STRLEN               },
    { "strlen_P",             SYS_STRLEN_P             },
    { "strcmp",               SYS_STRCMP               },
    { "strcmp_P",             SYS_STRCMP_P             },
    { "strcmp_PP",            SYS_STRCMP_PP            },
    { "strcpy",               SYS_STRCPY               },
    { "strcpy_P",             SYS_STRCPY_P             },
    { "format",               SYS_FORMAT               },
    { "music_play",           SYS_MUSIC_PLAY           },
    { "music_playing",        SYS_MUSIC_PLAYING        },
    { "music_stop",           SYS_MUSIC_STOP           },
    { "tones_play",           SYS_TONES_PLAY           },
    { "tones_play_primary",   SYS_TONES_PLAY_PRIMARY   },
    { "tones_play_auto",      SYS_TONES_PLAY_AUTO      },
    { "tones_playing",        SYS_TONES_PLAYING        },
    { "tones_stop",           SYS_TONES_STOP           },
    { "audio_enabled",        SYS_AUDIO_ENABLED        },
    { "audio_toggle",         SYS_AUDIO_TOGGLE         },
    { "audio_playing",        SYS_AUDIO_PLAYING        },
    { "audio_stop",           SYS_AUDIO_STOP           },
    { "save_exists",          SYS_SAVE_EXISTS          },
    { "save",                 SYS_SAVE                 },
    { "load",                 SYS_LOAD                 },
    { "sin",                  SYS_SIN                  },
    { "cos",                  SYS_COS                  },
    { "tan",                  SYS_TAN                  },
    { "atan2",                SYS_ATAN2                },
    { "floor",                SYS_FLOOR                },
    { "ceil",                 SYS_CEIL                 },
    { "round",                SYS_ROUND                },
    { "mod",                  SYS_MOD                  },
    { "pow",                  SYS_POW                  },
    { "sqrt",                 SYS_SQRT                 },
    { "generate_random_seed", SYS_GENERATE_RANDOM_SEED },
    { "init_random_seed",     SYS_INIT_RANDOM_SEED     },
    { "set_random_seed",      SYS_SET_RANDOM_SEED      },
    { "random",               SYS_RANDOM               },
    { "random_range",         SYS_RANDOM_RANGE         },
};

static std::string const CAT_GRAPHICS = "Graphics";
static std::string const CAT_SOUND = "Sound";
static std::string const CAT_BUTTONS = "Buttons";
static std::string const CAT_MATH = "Math";
static std::string const CAT_STRINGS = "Strings";
static std::string const CAT_UTILITY = "Utility";
static std::string const CAT_RANDOM = "Random";
static std::string const CAT_SAVELOAD = "Save/Load";

std::vector<std::string> const sysfunc_cats =
{
    CAT_GRAPHICS,
    CAT_SOUND,
    CAT_BUTTONS,
    CAT_MATH,
    CAT_STRINGS,
    CAT_UTILITY,
    CAT_RANDOM,
    CAT_SAVELOAD,
};

static std::string const HELP_FORMAT_STR =
    "The formatting supports a limited subset of `printf`-style format strings.\n\n"
    "| Specifier | Description |\n"
    "| :-- | :-- |\n"
    "| `%%` | A single '%' character. |\n"
    "| `%d` | A signed decimal integer (`i32`). |\n"
    "| `%u` | An unsigned decimal integer (`u32`). |\n"
    "| `%x` | An unsigned hexadecimal integer (`u32`). |\n"
    "| `%f` | A floating-point number (`float`). |\n"
    "| `%c` | A single character (`char`). |\n"
    "| `%s` | A text string (`char[]&` or `char[] prog&`). |\n\n"
    "The `%d`, `%u`, and `%x` specifiers support zero-padding to a given width, up to 9; "
    "for example, `%04u` would print 42 as \"0042\". "
    "The `%f` specifier supports a precision modifier of up to 9 digits for decimal fractions; "
    "for example, `.3f` would print 3.14159 as \"3.142\".";

std::unordered_map<sysfunc_t, sysfunc_info_t> const sysfunc_decls
{
    { SYS_DISPLAY,              { { TYPE_VOID,  { }, { } }, CAT_GRAPHICS,
        "Call this function once at the end of each frame to update the display and inputs. "
        "This function does the following in sequence:\n"
        "  1. Push the contents of the display buffer to the display.\n"
        "  2. Clear the contents of the display buffer to all black pixels.\n"
        "  3. Wait for frame timing (see `$set_frame_rate`).\n"
        "  4. Poll button states." } },
    { SYS_DISPLAY_NOCLEAR,      { { TYPE_VOID,  { }, { } }, CAT_GRAPHICS,
        "When not using grayscale, this function behaves exactly like `$display` except that the contents of the display "
        "buffer are left unmodified after pushing to the display. "
        "In grayscale modes, this function behaves exactly like `$display`, clearing the display buffer to all `BLACK` pixels." } },
    { SYS_GET_PIXEL,            { { TYPE_U8,    { TYPE_U8, TYPE_U8 }, { "x", "y" } }, CAT_GRAPHICS,
        "Get the color of a single pixel from the display buffer. "
        "In grayscale modes, this function always returns `BLACK`.", {
        "The x-coordinate of the pixel to test.",
        "The y-coordinate of the pixel to test."},
        "The color of the pixel: either `BLACK` or `WHITE`." } },
    { SYS_DRAW_PIXEL,           { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8 }, { "x", "y", "color" } }, CAT_GRAPHICS,
        "Draw a single pixel of a given color to the display buffer. "
        "This function does nothing in grayscale modes.", {
        "The x-coordinate of the pixel.",
        "The y-coorindate of the pixel."
        "The color (`BLACK` or `WHITE`) of the pixel." } } },
    { SYS_DRAW_HLINE,           { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "w", "color" } }, CAT_GRAPHICS,
        "Draw a horizontal line of a given color to the display buffer. "
        "This function does nothing in grayscale modes.", {
        "The x-coordinate of the leftmost pixel of the line.",
        "The y-coordinate of the line.",
        "The width of the line in pixels.",
        "The color (`BLACK` or `WHITE`) of the line." } } },
    { SYS_DRAW_VLINE,           { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "h", "color" } }, CAT_GRAPHICS,
        "Draw a vertical line of a given color to the display buffer. "
        "This function does nothing in grayscale modes.", {
        "The x-coordinate of the line.",
        "The y-coordinate of topmost pixel of the line.",
        "The height of the line in pixels.",
        "The color (`BLACK` or `WHITE`) of the line." } } },
    { SYS_DRAW_LINE,            { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_I16, TYPE_I16, TYPE_U8 }, { "x0", "y0", "x1", "y1", "color" } }, CAT_GRAPHICS,
        "Draw a line between two arbitrary points to the display buffer. "
        "This function does nothing in grayscale modes.", {
        "The x-coordinate of the start of the line.",
        "The y-coordinate of the start of the line.",
        "The x-coordinate of the end of the line.",
        "The y-coordinate of the end of the line.",
        "The color (`BLACK` or `WHITE`) of the line." } } },
    { SYS_DRAW_RECT,            { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8, TYPE_U8 }, { "x", "y", "w", "h", "color" } }, CAT_GRAPHICS,
        "Draw a single pixel thick outline of a rectangle to the display buffer. ", {
        "The x-coordinate of the left side of the rectangle.",
        "The y-coordinate of the top side of the rectangle.",
        "The width of the rectangle in pixels.",
        "The height of the rectangle in pixels.",
        "The color of the rectangle." } } },
    { SYS_DRAW_FILLED_RECT,     { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8, TYPE_U8 }, { "x", "y", "w", "h", "color" } }, CAT_GRAPHICS,
        "Draw a filled rectangle to the display buffer. ", {
        "The x-coordinate of the left side of the rectangle.",
        "The y-coordinate of the top side of the rectangle.",
        "The width of the rectangle in pixels.",
        "The height of the rectangle in pixels.",
        "The color of the rectangle." } } },
    { SYS_DRAW_CIRCLE,          { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "r", "color" } }, CAT_GRAPHICS,
        "Draw a single pixel thick outline of a circle to the display buffer. "
        "This function does nothing in grayscale modes.", {
        "The x-coordinate of the center of the circle.",
        "The y-coorindate of the center of the circle.",
        "The radius of the circle in pixels.",
        "The color of the circle." } } },
    { SYS_DRAW_FILLED_CIRCLE,   { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "r", "color" } }, CAT_GRAPHICS,
        "Draw a filled circle to the display buffer. ", {
        "The x-coordinate of the center of the circle.",
        "The y-coorindate of the center of the circle.",
        "The radius of the circle in pixels.",
        "The color of the circle." } } },
    { SYS_DRAW_SPRITE,          { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_SPRITES, TYPE_U16 }, { "x", "y", "s", "frame" } }, CAT_GRAPHICS,
        "Draw a sprite to the display buffer. ", {
        "The x-coorindate of the top side of the sprite.",
        "The y-coordinate of the left side of the sprite.",
        "The sprite set for the sprite to draw.",
        "The frame of the sprite to draw form the sprite set." } } },
    { SYS_DRAW_SPRITE_SELFMASK, { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_SPRITES, TYPE_U16 }, { "x", "y", "s", "frame" } }, CAT_GRAPHICS,
        "Draw a sprite to the display buffer. "
        "If the sprite is unmasked, the sprite is masked by its own white pixels. "
        "If the sprite is masked, the sprite is masked as normal. "
        "In grayscale modes, this function behaves exactly as `$draw_sprite`.", {
        "The x-coorindate of the top side of the sprite.",
        "The y-coordinate of the left side of the sprite.",
        "The sprite set for the sprite to draw.",
        "The frame of the sprite to draw form the sprite set." } } },
    { SYS_DRAW_TILEMAP,         { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_SPRITES, TYPE_TILEMAP }, { "x", "y", "s", "tm" } }, CAT_GRAPHICS,
        "Draw a tilemap resource to the display buffer.", {
        "The x-coordinate of the left side of the tilemap.",
        "The y-coordinate of the top side of the tilemap.",
        "The sprite set to use for the tiles.",
        "The tilemap to use for the sprite frames." } } },
    { SYS_DRAW_TEXT,            { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_STR }, { "x", "y", "text" } }, CAT_GRAPHICS,
        "Draw some text to the display buffer. "
        "The font and color that were previously set by `$set_text_font` and `$set_text_color` are used. ", {
        "The x-coordinate of the left edge of the first character in the text. "
        "When drawing, newline characters will set the x-coordinate of the next character to this value.",
        "The y-coordinate of the baseline of the first character in the text.",
        "The text to draw." } } },
    { SYS_DRAW_TEXT_P,          { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_STR_PROG }, { "x", "y", "text" } }, CAT_GRAPHICS, "" } },
    { SYS_DRAW_TEXTF,           { { TYPE_VOID,  { TYPE_I16, TYPE_I16, TYPE_STR_PROG }, { "x", "y", "fmt" } }, CAT_GRAPHICS,
        "Draw some formatted text to the display buffer. " + HELP_FORMAT_STR + " "
        "The font and color that were previously set by `$set_text_font` and `$set_text_color` are used. ", {
        "The x-coordinate of the left edge of the first character in the text. "
        "When drawing, newline characters will set the x-coordinate of the next character to this value.",
        "The y-coordinate of the baseline of the first character in the text.",
        "The format string to use for constructing the text to draw." } } },
    { SYS_TEXT_WIDTH,           { { TYPE_U16,   { TYPE_STR }, { "text" } }, CAT_GRAPHICS,
        "Get the width of some text in pixels. "
        "The font that was previously set by `$set_text_font` is used for character widths.", {
        "The text to compute the width." },
        "The width of the text in pixels." } },
    { SYS_TEXT_WIDTH_P,         { { TYPE_U16,   { TYPE_STR_PROG }, { "text" } }, CAT_GRAPHICS, "" } },
    { SYS_WRAP_TEXT,            { { TYPE_U16,   { TYPE_STR, TYPE_U8 }, { "text", "w"} }, CAT_GRAPHICS,
        "Word-wrap some text in-place by replacing space characters with newlines. "
        "The font that was previously set by `$set_text_font` is used for character widths.", {
        "The text to word-wrap.",
        "The wrap width in pixels." } } },
    { SYS_SET_TEXT_FONT,        { { TYPE_VOID,  { TYPE_FONT }, { "f" } }, CAT_GRAPHICS,
        "Set the font used by subsequent text functions.", {
        "The font to use in subsequent text functions." } } },
    { SYS_SET_TEXT_COLOR,       { { TYPE_VOID,  { TYPE_U8 }, { "color" } }, CAT_GRAPHICS,
        "Set the color used by subsequence text drawing functions.", {
        "The color to use in subsequent text functions." } } },
    { SYS_SPRITES_WIDTH,        { { TYPE_U8,    { TYPE_SPRITES }, { "s" } }, CAT_GRAPHICS,
        "Get the sprite width for a sprite set.", {
        "The sprite set." },
        "The sprite width for the sprite set." } },
    { SYS_SPRITES_HEIGHT,       { { TYPE_U8,    { TYPE_SPRITES }, { "s" } }, CAT_GRAPHICS,
        "Get the sprite height for a sprite set.", {
        "The sprite set." },
        "The sprite height for the sprite set." } },
    { SYS_SPRITES_FRAMES,       { { TYPE_U16,   { TYPE_SPRITES }, { "s" } }, CAT_GRAPHICS,
        "Get the number of sprite frames in a sprite set.", {
        "The sprite set." },
        "The number of sprite frames in the sprite set." } },
    { SYS_TILEMAP_WIDTH,        { { TYPE_U16,   { TYPE_TILEMAP }, { "tm" } }, CAT_GRAPHICS,
        "Get the width of a tilemap in tiles.", {
        "The tilemap." },
        "The width of the tilemap in tiles." } },
    { SYS_TILEMAP_HEIGHT,       { { TYPE_U16,   { TYPE_TILEMAP }, { "tm" } }, CAT_GRAPHICS,
        "Get the height of a tilemap in tiles.", {
        "The tilemap." },
        "The height of the tilemap in tiles." } },
    { SYS_TILEMAP_GET,          { { TYPE_U16,   { TYPE_TILEMAP, TYPE_U16, TYPE_U16 }, { "tm", "x", "y" }}, CAT_GRAPHICS,
        "Get the tile at the given coordinates in a tilemap. "
        "Tilemap coordinates are unsigned. The x-coordinate runs left-to-right and the y-coordinate runs top-to-bottom.", {
        "The x-coordinate of the tile to get.",
        "The y-coordinate of the tile to get." },
        "The tile at the given coordinates in the tilemap." } },
    { SYS_SET_FRAME_RATE,       { { TYPE_VOID,  { TYPE_U8 }, { "fps" } }, CAT_GRAPHICS,
        "Set the target frame rate (see `$display`).", {
        "The frame rate in frames per second." } } },
    { SYS_IDLE,                 { { TYPE_VOID,  { }, { } }, CAT_UTILITY,
        "Do nothing for a short time (one millisecond or less) and the CPU in a low power mode. "
        "This can be useful for waiting to respond to a button press without calling `$display` and "
        "without utilizing the CPU at 100%. "
        "Most games do not need to use this function." } },
    { SYS_DEBUG_BREAK,          { { TYPE_VOID,  { }, { } }, CAT_UTILITY,
        "Issue an AVR `break` instruction. "
        "This can be useful for debugging with an emulator or hardware debugger." } },
    { SYS_DEBUG_PRINTF,         { { TYPE_VOID,  { TYPE_STR_PROG }, { "fmt" } }, CAT_UTILITY,
        "Output some formatted text to the serial console. "
        "This function does not work on a physical Arduboy FX, as the interpreter does not include "
        "a USB software stack.", {
        "The format string to use for constructing the text to output." } } },
    { SYS_ASSERT,               { { TYPE_VOID,  { TYPE_BOOL }, { "cond" } }, CAT_UTILITY,
        "Runtime assertion for debug purposes. If the asserted condition evaluates to `false`, "
        "execution halts and a stack trace is displayed.", {
        "The condition to check." } } },
    { SYS_BUTTONS,              { { TYPE_U8,    { }, { } }, CAT_BUTTONS,
        "Gets the state of the buttons combined into a single mask. "
        "The bit for each button is `1` when the button is pressed." } },
    { SYS_JUST_PRESSED,         { { TYPE_BOOL,  { TYPE_U8 }, { "button" } }, CAT_BUTTONS,
        "Test if a button has just been pressed since the last frame.", {
        "The button to test for. Only one button should be specified." },
        "`true` if the specified button has just been pressed since the last frame." } },
    { SYS_JUST_RELEASED,        { { TYPE_BOOL,  { TYPE_U8 }, { "button" } }, CAT_BUTTONS,
        "Test if a button has just been released since the last frame.", {
        "The button to test for. Only one button should be specified." },
        "`true` if the specified button has just been released since the last frame." } },
    { SYS_PRESSED,              { { TYPE_BOOL,  { TYPE_U8 }, { "buttons" } }, CAT_BUTTONS,
        "Test if all of the specified buttons are pressed.", {
        "A bit mask indicating which buttons to test (can be a single button)." },
        "`true` if *all* buttons in the provided mask are currently pressed." } },
    { SYS_ANY_PRESSED,          { { TYPE_BOOL,  { TYPE_U8 }, { "buttons" } }, CAT_BUTTONS,
        "Test if any of the specified buttons are pressed.", {
        "A bit mask indicating which buttons to test (can be a single button)." },
        "`true` if *one or more* buttons in the provided mask are currently pressed." } },
    { SYS_NOT_PRESSED,          { { TYPE_BOOL,  { TYPE_U8 }, { "buttons" } }, CAT_BUTTONS,
        "Test if all of the specified buttons are not pressed.", {
        "A bit mask indicating which buttons to test (can be a single button)." },
        "`true` if *all* buttons in the provided mask are currently released." } },
    { SYS_MILLIS,               { { TYPE_U32,   { }, { } }, CAT_UTILITY,
        "Gets the time elapsed in milliseconds since program start.", {},
        "The time in milliseconds since program start." } },
    { SYS_MEMSET,               { { TYPE_VOID,  { TYPE_BYTE_AREF, TYPE_U8 }, { "dst", "val" } }, CAT_UTILITY,
        "Set each byte of some byte array to a single value.", {
        "The byte array to modify.",
        "The value to copy to each byte." } } },
    { SYS_MEMCPY,               { { TYPE_VOID,  { TYPE_BYTE_AREF, TYPE_BYTE_AREF }, { "dst", "src" } }, CAT_UTILITY,
        "Copy one byte array to another. The two byte arrays must be the same size.", {
        "The destination byte array.",
        "The source byte array." } } },
    { SYS_MEMCPY_P,             { { TYPE_VOID,  { TYPE_BYTE_AREF, TYPE_BYTE_PROG_AREF }, { "dst", "src" } }, CAT_UTILITY, "" } },
    { SYS_STRLEN,               { { TYPE_U16,   { TYPE_STR }, { "str" } }, CAT_STRINGS,
        "Get the length of a text string in characters. "
        "Use the `len` operator to get the capacity of a text string in characters.", {
        "The text string." },
        "The length of the text string in characters." } },
    { SYS_STRLEN_P,             { { TYPE_U24,   { TYPE_STR_PROG }, { "str" } }, CAT_STRINGS, "" } },
    { SYS_STRCMP,               { { TYPE_I8,    { TYPE_STR, TYPE_STR }, { "str0", "str1" } }, CAT_STRINGS,
        "Compare two text strings against each other lexicographically.", {
        "The first string to compare.",
        "The second string to compare." },
        "An integral value indicating the result of the comparison. "
        "A zero value indicates the two strings are equal. "
        "A negative or positive value indicates the first string is lexicographically "
        "less than or greater than the second string, respectively." } },
    { SYS_STRCMP_P,             { { TYPE_I8,    { TYPE_STR, TYPE_STR_PROG }, { "str0", "str1" } }, CAT_STRINGS, "" } },
    { SYS_STRCMP_PP,            { { TYPE_I8,    { TYPE_STR_PROG, TYPE_STR_PROG }, { "str0", "str1" } }, CAT_STRINGS, "" } },
    { SYS_STRCPY,               { { TYPE_STR,   { TYPE_STR, TYPE_STR }, { "dst", "src" } }, CAT_STRINGS,
        "Copy one text string to another. The two text strings may be different lengths or capacities.", {
        "The destination text string.",
        "The source text string." },
        "A reference to the destination text string." } },
    { SYS_STRCPY_P,             { { TYPE_STR,   { TYPE_STR, TYPE_STR_PROG }, { "dst", "src" } }, CAT_STRINGS, "" } },
    { SYS_FORMAT,               { { TYPE_VOID,  { TYPE_STR, TYPE_STR_PROG }, { "dst", "fmt" } }, CAT_STRINGS,
        "Copy formatted text into a text string. " + HELP_FORMAT_STR, {
        "The destination text string.",
        "The format string to use for constructing the destination text string." } } },
    { SYS_MUSIC_PLAY,           { { TYPE_VOID,  { TYPE_MUSIC }, { "song" } }, CAT_SOUND,
        "Start playing some music. Any previoously playing music will be stopped.", {
        "The music to play." } } },
    { SYS_MUSIC_PLAYING,        { { TYPE_BOOL,  { }, { } }, CAT_SOUND,
        "Get whether any music is playing.", {},
        "`true` if any music is playing." } },
    { SYS_MUSIC_STOP,           { { TYPE_VOID,  { }, { } }, CAT_SOUND,
        "Stop any music that is currently playing." } },
    { SYS_TONES_PLAY,           { { TYPE_VOID,  { TYPE_TONES }, { "sfx" } }, CAT_SOUND,
        "Play some tones on the tones channel. "
        "Any tones previously playing on the tones channel will be stopped.", {
        "The tones to play." } } },
    { SYS_TONES_PLAY_PRIMARY,   { { TYPE_VOID,  { TYPE_TONES }, { "sfx" } }, CAT_SOUND,
        "Play some tones on the primary music channel. "
        "Any tones on the primary channel or music previously playing will be stopped. "
        "This is useful when some tones need to play that should not be interrupted by "
        "future calls to `$tones_play`.", {
        "The tones to play. " } } },
    { SYS_TONES_PLAY_AUTO,      { { TYPE_VOID,  { TYPE_TONES }, { "sfx" } }, CAT_SOUND,
        "Play some tones on either the tones channel or the primary music channel. "
        "The primary music channel will be used only if the tones channel is occupied "
        "and the primary music channel is unoccupied. "
        "This is useful for situations where music is not playing; it allows two tones "
        "to play simultaneously." } },
    { SYS_TONES_PLAYING,        { { TYPE_BOOL,  { }, { } }, CAT_SOUND,
        "Get whether tones are playing on any channel.", {},
        "`true` if tones are playing on any channel." } },
    { SYS_TONES_STOP,           { { TYPE_VOID,  { }, { } }, CAT_SOUND,
        "Stop any tones playing on any channel." } },
    { SYS_AUDIO_ENABLED,        { { TYPE_BOOL,  { }, { } }, CAT_SOUND,
        "Get whether music and tones are able to play.", {},
        "`true` if music and tones are able to play." } },
    { SYS_AUDIO_TOGGLE,         { { TYPE_VOID,  { }, { } }, CAT_SOUND,
        "Toggle whether music and tones are able to play." } },
    { SYS_AUDIO_PLAYING,        { { TYPE_BOOL,  { }, { } }, CAT_SOUND,
        "Get whether any music or tones are playing on any channel.", {},
        "`true` if any music or tones are playing on any channel." } },
    { SYS_AUDIO_STOP,           { { TYPE_VOID,  { }, { } }, CAT_SOUND,
        "Stop any music or tones that are currently playing on any channel." } },
    { SYS_SAVE_EXISTS,          { { TYPE_BOOL,  { }, { } }, CAT_SAVELOAD,
        "Get whether a save file exists.", {},
        "`true` if a save file exists." } },
    { SYS_SAVE,                 { { TYPE_VOID,  { }, { } }, CAT_SAVELOAD,
        "Save all `saved` global variables to the save file." } },
    { SYS_LOAD,                 { { TYPE_BOOL,  { }, { } }, CAT_SAVELOAD,
        "Overwrite all `saved` global variables from the save file, if it exists.", {},
        "`true` if the save file exists and the global variables were overwritten." } },
    { SYS_SIN,                  { { TYPE_FLOAT, { TYPE_FLOAT }, { "angle" } }, CAT_MATH,
        "Get the sine of an angle.", {
        "The angle in radians." },
        "The sine of the angle." } },
    { SYS_COS,                  { { TYPE_FLOAT, { TYPE_FLOAT }, { "angle" } }, CAT_MATH,
        "Get the cosine of an angle.", {
        "The angle in radians." },
        "The cosine of the angle." } },
    { SYS_TAN,                  { { TYPE_FLOAT, { TYPE_FLOAT }, { "angle" } }, CAT_MATH,
        "Get the tangent of an angle.", {
        "The angle in radians." },
        "The tangent of the angle." } },
    { SYS_ATAN2,                { { TYPE_FLOAT, { TYPE_FLOAT, TYPE_FLOAT }, { "y", "x" } }, CAT_MATH,
        "Get the angle between the positive x-axis and the ray from the origin "
        "to the point (x, y) on the Cartesian plane.", {
        "The y-coordinate of the ray endpoint on the Cartesian plane.",
        "The x-coordinate of the ray endpoint on the Cartesian plane." },
        "The angle between the positive x-axis and the ray from the origin "
        "to the point (x, y) on the Cartesian plane." } },
    { SYS_FLOOR,                { { TYPE_FLOAT, { TYPE_FLOAT }, { "x" } }, CAT_MATH,
        "Get the greatest integer not greater than a number.", {
        "The number." },
        "The greatest integer not greater than the number." } },
    { SYS_CEIL,                 { { TYPE_FLOAT, { TYPE_FLOAT }, { "x" } }, CAT_MATH,
        "Get the least integer not less than a number.", {
        "The number." },
        "The least integer not less than the number." } },
    { SYS_ROUND,                { { TYPE_FLOAT, { TYPE_FLOAT }, { "x" } }, CAT_MATH,
        "Get the integer closest to a number, choosing the greater if equidistant from two integers.", {
        "The number." },
        "The integer closest to the number, choosing the greater if equidistant from two integers." } },
    { SYS_MOD,                  { { TYPE_FLOAT, { TYPE_FLOAT, TYPE_FLOAT }, { "x", "y" } }, CAT_MATH,
        "Get the remainder of a division.", {
        "The numerator of the division.",
        "The denominator of the division." },
        "The remainder of the division." } },
    { SYS_POW,                  { { TYPE_FLOAT, { TYPE_FLOAT, TYPE_FLOAT }, { "x", "y" } }, CAT_MATH,
        "Get the result of an exponentiation.", {
        "The base of the exponentiation.",
        "The exponent of the exponentiation." },
        "The result of the exponentiation." } },
    { SYS_SQRT,                 { { TYPE_FLOAT, { TYPE_FLOAT }, { "x" } }, CAT_MATH,
        "Get the square root of a number.", {
        "The number." },
        "The square root of the number." } },
    { SYS_GENERATE_RANDOM_SEED, { { TYPE_U32,   { }, { } }, CAT_RANDOM,
        "Get a random value generated from true entropy.", {},
        "A random value generated from true entropy." } },
    { SYS_INIT_RANDOM_SEED,     { { TYPE_VOID,  { }, { } }, CAT_RANDOM,
        "Initialize the internal random seed with a value generated from true entropy." } },
    { SYS_SET_RANDOM_SEED,      { { TYPE_VOID,  { TYPE_U32 }, { "seed" } }, CAT_RANDOM,
        "Set the internal random seed.", {
        "The random seed." } } },
    { SYS_RANDOM,               { { TYPE_U32,   { }, { } }, CAT_RANDOM,
        "Get a 32-bit random value generated from the internal random seed.", {},
        "A 32-bit random value generated from the internal random seed." } },
    { SYS_RANDOM_RANGE,         { { TYPE_U32,   { TYPE_U32, TYPE_U32 }, { "lo", "hi" } }, CAT_RANDOM,
        "Get a 32-bit random value generated from the internal random seed "
        "constrained to fall within a given range. "
        "The range is inclusive; `$random_range(1, 3)` will produce either 1, 2, or 3.", {
        "The lower bound of the constrained range, inclusive.",
        "The upper bound of the constrained range, inclusive." },
        "A 32-bit random value generated from the internal random seed "
        "constrained to fall within the given range." } },
};

}
