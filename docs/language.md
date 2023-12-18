# Language

ABC's language design is inspired primarily from C/C++. It is designed to be [memory safe](https://en.wikipedia.org/wiki/Memory_safety): all array accesses are bounds-checked and pointers are excluded in favor of reference semantics. If you find an instance in which a compiled ABC program is able to violate memory safety, please let me know as I'd consider that a bug.

Language features:
- Functions
- Essential arithmetic, bitwise, and (short-circuiting) logical operators with same precedence as C
- Structs
- Multidimensional arrays (some small syntax differences noted below)
- References
  - Function arguments can be [passed by reference](https://en.wikipedia.org/wiki/Evaluation_strategy#Call_by_reference)
  - Structs and arrays can contain references, but are then noncopyable
- Control: `for`, `while`, `if` (`do`-`while` and `switch` not currently supported)

## Assets (sprites, fonts, etc)
Various asset types each have their own dedicated type, which acts as a handle to a location in program memory. These handles are copyable but cannot be otherwise inspected or used in arithmetic expressions.

### Sprite Sets
The `sprites` type identifies a set of sprites. Sprites can be defined inline via a kind of ASCII-art or loaded by specifying a path to an image file. Both masked and unmasked sprites are supported, and the same drawing methods (e.g., $draw_sprite) can be used for both.

Examples of ASCII-art style sprite set initialization:
```c
// In ASCII-art sprite literals, characters have the following meaning:
//     '-' -- transparency
//     '.' -- black
//     Any other non-whitespace character indicates white.
constexpr sprites DIGITS = sprites{
    3x5 // ASCII-art sprite literals need to know how large one sprite is
    .X.  .X.  XX.  XX.  X.X  XXX  .X.  XXX  .X.  .X.
    X.X  XX.  ..X  ..X  X.X  X..  X..  ..X  X.X  X.X
    X.X  .X.  .X.  .X.  XXX  XX.  XX.  ..X  .X.  .XX
    X.X  .X.  X..  ..X  ..X  ..X  X.X  .X.  X.X  ..X
    .X.  XXX  XXX  XX.  ..X  XX.  .X.  .X.  .X.  .X.
};

constexpr sprites MASKED_BALL = sprites{
    8x8
    --XXXX--
    -X....X-
    X......X
    X......X
    X......X
    X......X
    -X....X-
    --XXXX--
};
```

Examples of declaring sprites from a file:
```c
// Specifying sprite size for a set of sprites
constexpr sprites TILES = sprites{ 16x16 "assets/tileset.png" };

// Leaving out the size is allowed when loading from a file. In this case,
// there will be just one sprite the same size as the image.
constexpr sprites TITLE_IMG = sprites{ "assets/title.png" };
```

### Fonts
The `font` type identifies a font that can be used to draw text (e.g., with the `$draw_text` family of system functions).

```c
// To load a font from a TTF file, specify the font size and the filename.
constexpr font f = font{ 12 "assets/font.ttf" };

$draw_text_P(x, y, f, "Hello World!");
```

## System Functions
This list may be outdated -- see [here](https://github.com/tiberiusbrown/abc/blob/master/docs/system.md) for the current list.
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
```

## Predefined Constants
This list may be outdated -- see [here](https://github.com/tiberiusbrown/abc/blob/master/docs/system.md) for the current list.
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

## Planned to Include in ABC (TODO)
- Compound assignment operators (e.g., `+=`)
- Pre and post increment/decrement operators (`++`/`--`)
- Floating point data types
- Python-style array slicing (e.g., given `int[6] a;`, the expression `a[2:5]` results in a reference of type `int[3]&` referring to `a[2]`)
- Function references

## Intentionally Excluded from ABC
- Pointers
- Comma operator
- Macros
- Variadic functions (except for system methods like `$format` which have special support)
- `goto`

## Differences from C

ABC differs from C in several aspects, both to simplify the language and to avoid inheriting some "gotchas" from C.

### Primitive Numeric Types
The following keyword types are exposed in ABC.

| Keyword | Type      | Bit Width |
|:-------:|-----------|:---------:|
| `bool`  | boolean   | 8         |
| `char`  | character | 8         |
| `i8`    | signed    | 8         |
| `i16`   | signed    | 16        |
| `i24`   | signed    | 24        |
| `i32`   | signed    | 32        |
| `short` | signed    | 8         |
| `int`   | signed    | 16        |
| `long`  | signed    | 32        |
| `u8`    | unsigned  | 8         |
| `u16`   | unsigned  | 16        |
| `u24`   | unsigned  | 24        |
| `u32`   | unsigned  | 32        |
| `ushort`| unsigned  | 8         |
| `uint`  | unsigned  | 16        |
| `ulong` | unsigned  | 32        |

### Array Type Syntax
In general, ABC strives to maintain consistent behavior across the syntax for different types: in a variable declaration `T x;` the syntax `T` should convey the complete type of variable `x`.

Because of this principle, an ABC array is declared as below, with both the number of array elements and the element type kept together in the same type syntax.
```c
u16[4] values;
```

The equivalent C declaration separates the type from the array size.
```c
uint16_t values[4];
```

### Multidimensional Array Syntax
Type syntax in ABC is intended to be consistent and easy to parse. For any type `T`, the type `T[N]` should always represent an array of `N` elements of type `T`. Thus, in ABC, the type `int[2][3]` represents an array of 3 elements of type `int[2]`. This leads to a reversal of the dimension order from C syntax for multidimensional arrays.

```c
// C syntax
uint8_t map_items[MAP_ROWS][MAP_COLS][MAX_ITEMS];

// ABC syntax
u8[MAX_ITEMS][MAP_COLS][MAP_ROWS] map_items;
```

### Array Decay Behavior
In C, an array decays to a pointer to its first element when it is used as part of an expression. Furthermore, passing a sized array as a function parameter in C merely passes a pointer, effectively passing the array "by reference" to the function. Neither of these behaviors are shared with structs, another class of compound types, and the design philosophy of ABC considers them inconsistencies.

Instead, arrays in ABC are always value types in the same way as structs. They never decay to pointers or references, and passing an array to a function passes it by value.

```c
int[2] sum(int[2] a)
{
    a[0] += a[1];
    return a;
}

void main()
{
    int[2] a = { 1, 2 };
    int[2] b = sum(a);
    $assert(a[0] == 1 && a[1] == 2);
    $assert(b[0] == 3 && b[1] == 2);
}
```

Furthermore, arrays are copyable.

```c
int[4] x = { 1, 2, 3, 4 };
int[4] y = { 5, 6, 7, 8 };

$assert(x[2] == 3);
x = y;
y[2] = 3;
$assert(x[2] == 7);
```

### String Literals
In C, a string literal is of type `char const[N]`, where `N` is the length of the string, including a null terminator.

In ABC, a string literal is of type `char[N] prog`, where `N` is the length of the string, *not* including a null terminator.

### References
ABC has references (like C++) but not pointers. For any type `T`, the type `T&` is a reference to `T`. As in C++, references cannot be reassigned and must be initialized when declared.

```c
int x = 2;
int& a = x;

// Error: references must be initialized
// int& b;

$assert(x == 2);
a = 3;
$assert(x == 3);
```

### Unsized Array References
Normally, creating a reference to an array requires knowledge of the length of the array in the reference's type to enable bounds-checking for all array accesses through the reference.

```c
int[4] a = { 1, 2, 3, 4 };

// All accesses to 'a' through 'r' are bounds-checked.
int[4]& r = a;
```

However, it is possible to create a reference to an array of unspecified size, or an unsized array reference (UAR). Internally, a UAR also stores the size of the array it references (and are thus double the size of a normal reference), so bounds-checking is still possible. UARs are conceptually similar to C++20's `std::span` with dynamic extent.

For some array of type `T[N]`, the type of its UAR is `T[]&`. For a `prog` array of type `T[N] prog`, its UAR type is `T[] prog&`. This syntax was intentionally chosen to easily allow changing standard array references to UARs by simply removing the array size.

```c
int[4] a = { 1, 2, 3, 4 };

// All accesses to 'a' through 'r' are bounds-checked.
// However, the storage for 'r' is double that of a sized array reference.
int[]& r = a;
```

UARs can be useful for defining an array of strings. The below example declares a `prog` array of 3 elements, each of which is a reference to a `prog` unsized array of `char`:

```c
char[] prog&[3] prog MY_STRINGS = {
    "Short",
    "A somewhat longer string",
    "?"
};
```

UARs are also useful in defining functions that can take arrays of any length:

```c
int sum(int[]& a)
{
    int t = 0;
    for(int i = 0; i < len(a); ++i)
        t += a[i];
    return t;
}
```

### Bitwise Operations with `bool`
Consider the following conditional.
```c
// call f() if the lowest three bits of x are cleared
if(x & 7 == 0)
    f();
```
Despite the obvious intention behind the code, this conditional always evaluates as false. Due to operator precedence, it's equivalent to the form below.
```c
if(x & (7 == 0))
    f();
```
To avoid this mistake, ABC could adjust operator precedence to differ from C. However, this would prove poor training for someone who is learning with ABC and will eventually move to C. Instead, ABC disallows performing bitwise operations with boolean types, so the original code would produce a compiler error (since it tries to bitwise AND `x` with `7 == 0`, which is of type `bool`). Thus the programmer is forced to insert parentheses to reflect the intent appropriately, as would be done in C.
```c
if((x & 7) == 0)
    f();
```
Naturally, if you actually want to perform a bitwise operation with boolean types, you may first cast any boolean expressions to integer types.
