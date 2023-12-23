#include "ards_assembler.hpp"
#include "ards_compiler.hpp"

namespace ards
{

bool sysfunc_is_format(sysfunc_t f)
{
    return f == SYS_DRAW_TEXTF || f == SYS_FORMAT;
}

std::unordered_map<std::string, sysfunc_t> const sys_names =
{
    { "display",            SYS_DISPLAY            },
    { "draw_pixel",         SYS_DRAW_PIXEL         },
    { "draw_hline",         SYS_DRAW_HLINE         },
    { "draw_vline",         SYS_DRAW_VLINE         },
    { "draw_line",          SYS_DRAW_LINE          },
    { "draw_rect",          SYS_DRAW_RECT          },
    { "draw_filled_rect",   SYS_DRAW_FILLED_RECT   },
    { "draw_circle",        SYS_DRAW_CIRCLE        },
    { "draw_filled_circle", SYS_DRAW_FILLED_CIRCLE },
    { "draw_sprite",        SYS_DRAW_SPRITE        },
    { "draw_text",          SYS_DRAW_TEXT          },
    { "draw_text_P",        SYS_DRAW_TEXT_P        },
    { "draw_textf",         SYS_DRAW_TEXTF         },
    { "text_width",         SYS_TEXT_WIDTH         },
    { "text_width_P",       SYS_TEXT_WIDTH_P       },
    { "set_frame_rate",     SYS_SET_FRAME_RATE     },
    { "next_frame",         SYS_NEXT_FRAME         },
    { "idle",               SYS_IDLE               },
    { "debug_break",        SYS_DEBUG_BREAK        },
    { "assert",             SYS_ASSERT             },
    { "poll_buttons",       SYS_POLL_BUTTONS       },
    { "just_pressed",       SYS_JUST_PRESSED       },
    { "just_released",      SYS_JUST_RELEASED      },
    { "pressed",            SYS_PRESSED            },
    { "any_pressed",        SYS_ANY_PRESSED        },
    { "not_pressed",        SYS_NOT_PRESSED        },
    { "millis",             SYS_MILLIS             },
    { "strlen",             SYS_STRLEN             },
    { "strlen_P",           SYS_STRLEN_P           },
    { "strcmp",             SYS_STRCMP             },
    { "strcmp_P",           SYS_STRCMP_P           },
    { "strcpy",             SYS_STRCPY             },
    { "strcpy_P",           SYS_STRCPY_P           },
    { "format",             SYS_FORMAT             },
    { "tones_play",         SYS_TONES_PLAY         },
    { "tones_playing",      SYS_TONES_PLAYING      },
    { "tones_stop",         SYS_TONES_STOP         },
    { "audio_enabled",      SYS_AUDIO_ENABLED      },
    { "audio_toggle",       SYS_AUDIO_TOGGLE       },
};

std::unordered_map<sysfunc_t, compiler_func_decl_t> const sysfunc_decls
{
    { SYS_DISPLAY,            { TYPE_VOID, { }, { } } },
    { SYS_DRAW_PIXEL,         { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8 }, { "x", "y", "color" } } },
    { SYS_DRAW_HLINE,         { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "w", "color" } } },
    { SYS_DRAW_VLINE,         { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "h", "color" } } },
    { SYS_DRAW_LINE,          { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_I16, TYPE_I16, TYPE_U8 }, { "x0", "y0", "x1", "y1", "color" } } },
    { SYS_DRAW_RECT,          { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8, TYPE_U8 }, { "x", "y", "w", "h", "color" } } },
    { SYS_DRAW_FILLED_RECT,   { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8, TYPE_U8 }, { "x", "y", "w", "h", "color" } } },
    { SYS_DRAW_CIRCLE,        { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "r", "color" } } },
    { SYS_DRAW_FILLED_CIRCLE, { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_U8, TYPE_U8 }, { "x", "y", "r", "color" } } },
    { SYS_DRAW_SPRITE,        { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_SPRITES, TYPE_U16 }, { "x", "y", "s", "frame" } } },
    { SYS_DRAW_TEXT,          { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_FONT, TYPE_STR }, { "x", "y", "f", "str" } } },
    { SYS_DRAW_TEXT_P,        { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_FONT, TYPE_STR_PROG }, { "x", "y", "f", "str" } } },
    { SYS_DRAW_TEXTF,         { TYPE_VOID, { TYPE_I16, TYPE_I16, TYPE_FONT, TYPE_STR_PROG }, { "x", "y", "f", "fmt" } } },
    { SYS_TEXT_WIDTH,         { TYPE_U16,  { TYPE_FONT, TYPE_STR }, { "f", "str" } } },
    { SYS_TEXT_WIDTH_P,       { TYPE_U16,  { TYPE_FONT, TYPE_STR_PROG }, { "f", "str" } } },
    { SYS_SET_FRAME_RATE,     { TYPE_VOID, { TYPE_U8 }, { "fps" } } },
    { SYS_NEXT_FRAME,         { TYPE_BOOL, { }, { } } },
    { SYS_IDLE,               { TYPE_VOID, { }, { } } },
    { SYS_DEBUG_BREAK,        { TYPE_VOID, { }, { } } },
    { SYS_ASSERT,             { TYPE_VOID, { TYPE_BOOL }, { "cond" } } },
    { SYS_POLL_BUTTONS,       { TYPE_VOID, { }, { } } },
    { SYS_JUST_PRESSED,       { TYPE_BOOL, { TYPE_U8 }, { "button" } } },
    { SYS_JUST_RELEASED,      { TYPE_BOOL, { TYPE_U8 }, { "button" } } },
    { SYS_PRESSED,            { TYPE_BOOL, { TYPE_U8 }, { "buttons" } } },
    { SYS_ANY_PRESSED,        { TYPE_BOOL, { TYPE_U8 }, { "buttons" } } },
    { SYS_NOT_PRESSED,        { TYPE_BOOL, { TYPE_U8 }, { "buttons" } } },
    { SYS_MILLIS,             { TYPE_U32,  { }, { } } },
    { SYS_STRLEN,             { TYPE_U16,  { TYPE_STR }, { "str" } } },
    { SYS_STRLEN_P,           { TYPE_U24,  { TYPE_STR_PROG }, { "str" } } },
    { SYS_STRCMP,             { TYPE_I8,   { TYPE_STR, TYPE_STR }, { "str0", "str1" } } },
    { SYS_STRCMP_P,           { TYPE_I8,   { TYPE_STR, TYPE_STR_PROG }, { "str0", "str1" } } },
    { SYS_STRCPY,             { TYPE_VOID, { TYPE_STR, TYPE_STR }, { "dst", "src" } } },
    { SYS_STRCPY_P,           { TYPE_VOID, { TYPE_STR, TYPE_STR_PROG }, { "dst", "src" } } },
    { SYS_FORMAT,             { TYPE_VOID, { TYPE_STR, TYPE_STR_PROG }, { "dst", "fmt" } } },
    { SYS_TONES_PLAY,         { TYPE_VOID, { TYPE_TONES }, { "song" } } },
    { SYS_TONES_PLAYING,      { TYPE_BOOL, { }, { } } },
    { SYS_TONES_STOP,         { TYPE_VOID, { }, { } } },
    { SYS_AUDIO_ENABLED,      { TYPE_BOOL, { }, { } } },
    { SYS_AUDIO_TOGGLE,       { TYPE_VOID, { }, { } } },
};

}
