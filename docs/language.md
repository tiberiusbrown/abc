# Language

- [Language](#language)
  - [Introduction](#introduction)
  - [Types](#types)
    - [Primitive Numeric Types](#primitive-numeric-types)
      - [The `bool` type](#the-bool-type)
      - [The `byte` type](#the-byte-type)
      - [The `char` type](#the-char-type)
    - [The `prog` Type Attribute](#the-prog-type-attribute)
    - [Arrays](#arrays)
      - [Multidimensional Arrays](#multidimensional-arrays)
      - [Strings](#strings)
    - [Aggregates: `struct`](#aggregates-struct)
    - [References](#references)
      - [Unsized Array References](#unsized-array-references)
      - [Function References](#function-references)
    - [Assets (sprites, fonts, etc)](#assets-sprites-fonts-etc)
      - [Sprite Sets](#sprite-sets)
      - [Tilemaps](#tilemaps)
      - [Fonts](#fonts)
      - [Tones](#tones)
      - [Music](#music)
    - [`constexpr` Variables](#constexpr-variables)
  - [Language](#language-1)
    - [Flow Control](#flow-control)
      - [The `switch` Statement](#the-switch-statement)
    - [Arrays as Value Types](#arrays-as-value-types)
    - [Array Slices](#array-slices)
  - [The `main` Function](#the-main-function)
  - [System Functions](#system-functions)
  - [Compiler Directives](#compiler-directives)
    - [The `#shades` Directive](#the-shades-directive)
  - [Planned to Include in ABC (TODO)](#planned-to-include-in-abc-todo)
  - [Intentionally Excluded from ABC](#intentionally-excluded-from-abc)

## Introduction

ABC's language design is inspired primarily from C/C++. It is designed to be [memory safe](https://en.wikipedia.org/wiki/Memory_safety): all array accesses are bounds-checked and pointers are excluded in favor of reference semantics. If you find an instance in which a compiled ABC program is able to violate memory safety, please let me know as I'd consider that a bug.

Language features:
- Functions
- Floating point type and built-in mathematical functions
- Asset types: sprites, fonts, tones
- Limited `constexpr` variables (numeric and asset handle types only)
- Essential arithmetic, bitwise, and (short-circuiting) logical operators with same precedence as C
- Structs
- Multidimensional arrays (some small syntax differences noted below)
- References
  - Function arguments can be [passed by reference](https://en.wikipedia.org/wiki/Evaluation_strategy#Call_by_reference)
  - Structs and arrays can contain references, but are then noncopyable
- Control: `for`, `while`, `do`-`while`, `if`, `switch`

## Types

### Primitive Numeric Types
The following keyword types are exposed in ABC.

| Keyword | Usage     | Bit Width |
|:-------:|-----------|:---------:|
| `bool`  | boolean   | 8         |
| `byte`  | byte      | 8         |
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
| `float` | float     | 32        |

#### The `bool` type

The `bool` type is the type of relational and boolean operations (such as `a == b` or `a && b`). It can only contain the values `true` or `false`, which convert to `1` and `0`, respectively, when cast to another numeric type. When converted to `bool`, values of a non-`bool` type convert to `false` when zero, and `true` otherwise.

Unlike C, bitwise operators may not operate on `bool`. This prevents mistakes such as the following:
```c
// Intent:  call f() if the lowest three bits of x are cleared
// Reality: never call f(), as the condition is actually "x & (7 == 0)"
if(x & 7 == 0)
    f();
```

#### The `byte` type

Any reference to a copyable type may be converted to an [unsized array reference](#unsized-array-references) of `byte` type.

#### The `char` type

Arrays of type `char` are allowed to resize when assigned to each other. This reflects the intended usage of `char` values as characters in strings.

```c
int[4] a_int;
int[6] b_int;

a_int = b_int; // ERROR! array sizes differ

char[4] a_char;
char[6] b_char;

a_char = b_char; // OK
```

### The `prog` Type Attribute

For any type `T`, the type `T prog` indicates storage in program memory.

### Arrays

In general, ABC strives to maintain consistent behavior across the syntax for different types: in a variable declaration `T x;` the syntax `T` should convey the complete type of variable `x`.

Because of this principle, an ABC array is declared as below, with both the number of array elements and the element type kept together in the same type syntax.
```c
u16[4] values;
```

The equivalent C declaration separates the type from the array size.
```c
uint16_t values[4];
```

#### Multidimensional Arrays
Type syntax in ABC is intended to be consistent and easy to parse. For any type `T`, the type `T[N]` should always represent an array of `N` elements of type `T`. Thus, in ABC, the type `int[2][3]` represents an array of 3 elements of type `int[2]`. This leads to a reversal of the dimension order from C syntax for multidimensional arrays.

```c
// C syntax
uint8_t map_items[MAP_ROWS][MAP_COLS][MAX_ITEMS];

// ABC syntax
u8[MAX_ITEMS][MAP_COLS][MAP_ROWS] map_items;
```

#### Strings

Strings in ABC are `char` arrays, where the length of the array is the capacity (maximum length) of the string. An ABC string does not require null termination if it occupies the full capacity of its array.

```c
// str1 will contain the 5 characters "Hello" followed by a null terminator.
char[12] str1 = "Hello";

// str2 will contain the 12 characters "Hello World!" and no null terminator.
char[12] str2 = "Hello World!";

// str3 will contain the 12 characters "abcdefghijkl" and no null terminator.
char[12] str3 = "abcdefghijklmnop";
```

A string literal is of type `char[N] prog`, where `N` is the length of the string, *not* including a null terminator.

### Aggregates: `struct`

As in C/C++, the  `struct` keyword may be used to define an aggregate type.

```c
struct my_type_t
{
    int x;
    float f;
    u8[10] str;
};

my_type_t foo;
my_type_t[3] foo_array;
```

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

#### Unsized Array References
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

UARs may be created from multidimensional arrays:

```
u16[2][3] a = { {1, 2}, {3, 4}, {5, 6} };
u16[]& r = a;
$assert(len(r) == 6);
$assert(r[0] == 1);
$assert(r[1] == 2);
$assert(r[2] == 3);
$assert(r[3] == 4);
$assert(r[4] == 5);
$assert(r[5] == 6);
```

#### Function References

References to functions are a special type of reference that is copyable and reassignable.

```c
u8 f(u8 x, i8 y) { return x + y + 2; }
u8 g(u8 x, i8 y) { return x + y + 3; }
u8 h(u8 x, u8 y) { return 0; }

void main()
{
    // 'r' is a reference to function 'f'
    :u8(u8 x, i8 y)& r = f;

    // indirect call to 'f' through 'r'
    $assert(r(0, 0) == 2);

    // function references can be reassigned
    r = g;

    // indirect call to 'g' through 'r'
    $assert(r(0, 0) == 3);

    // ERROR: function signature of 'h' differs from 'r'
    // the second argument of 'h' is 'u8' instead of 'i8'
    r = h;
}
```

The type syntax of a function reference is `:return_type(arg_types_list)&`.

### Assets (sprites, fonts, etc)
Various asset types each have their own dedicated type, which acts as a handle to a location in program memory. These handles are copyable but cannot be otherwise inspected or used in arithmetic expressions.

#### Sprite Sets
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

#### Tilemaps
The `tilemap` type identifies a sized 2D-array of sprite indices, which can be used for drawing with the `$draw_tilemap` system function.

```c
// TMX tilemaps (e.g., from Tiled Map Editor) are supported
$draw_tilemap(x, y, sprites{ 16x16 "tiles.png" }, tilemap{ "world.tmx" });
```

#### Fonts
The `font` type identifies a font that can be used to draw text (e.g., with the `$draw_text` family of system functions).

```c
// To load a font from a TTF file, specify the font size and the filename.
constexpr font f = font{ 12 "assets/font.ttf" };

$draw_text_P(x, y, f, "Hello World!");
```

There are a number of built-in fonts that are automatically included when used. See the full list [here](https://github.com/tiberiusbrown/abc/blob/master/docs/builtin_fonts.md).

#### Tones
The `tones` type identifies a monophonic sequence of musical tones. Each tone is defined by a musical note and a duration in milliseconds.

```c
// A two-tone note: B5 for 100 ms followed by E6 for 200 ms (e.g., platformer coin sound)
constexpr tones my_sfx = tones{ B5 100 E6 200 };

// MIDI import is supported
constexpr tones my_song = tones{ "assets/song.mid" };

$tones_play(my_sfx);

// As with other resource handle types, you can use a literal directly:
$tones_play(tones{ C4# 50 C4 50 C4b 50 });
```

#### Music
The `music` type identifies a sequence of musical tones in which up to two notes may play simultaneously.

```c
// song.mid may have up to two notes playing at once
constexpr music my_song = music{ "assets/song.mid" };
```

### `constexpr` Variables
Numeric or asset handle variables can be declared `constexpr`. When declared `constexpr`, a variable occupies no storage and its value (calculated at compile time) is inserted directly into any expression in which the variable is used.

```cpp
// Numeric types may be declared constexpr
constexpr i16 X = 3;
constexpr float Y = X + 2;

// Asset handles may be declared constexpr as well
constexpr font MY_FONT = font{ 8 "assets/font.ttf" };
```

Certain language features, such as [array slices](#array-slices), have behavior that depends on whether or not an integral expression's value is computable at compile-time. Integer literals,  `constexpr` variables, and the `len()` operator on arrays and array references are guaranteed to produce compile-time constants.

## Language

### Flow Control

The following standard flow control elements exist in ABC and behave just as in C:
- `if`-`else`
- `while`
- `do`-`while`
- `for`

Additionally, for these control flow elements, the `break` and `continue` statements operate as in C.

There are no labels or `goto` statements in ABC.

#### The `switch` Statement

Since ABC lacks labels and `goto`, there is a departure from C in the syntax and behavior of the `switch` statement.

ABC's switch statement has new syntax for cases which is more similar to `if`-statements. ABC also includes support for case ranges and multiple values per `case` statement.

```c
switch(x)
{
case(1 ... 3, 7) f();
case(8 ... 10)   g();
}
```

Additionally, unlike C, fallthrough is explicit in ABC. The `continue` statement is used to fall through to the next case, while the `break` statement retains its original behavior of breaking out of the switch statement.

```c
switch(state)
{
case(STATE_SETUP)
{
    setup();
    state = STATE_PLAY;
    continue;
}
case(STATE_PLAY)
    play();
// ...
}
```

### Arrays as Value Types

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

### Array Slices

Given some array, array [reference](#references), or [UAR](#unsized-array-references) `a`, the syntax `a[start+:length]` or `a[start:stop]` is a slice into `a` including indices in the range `[start, start+length)` or `[start, stop)`. If the array elements are type `T`, the type of the array slice is
- `T[length]&` or `T[stop-start]&` if `length` or both `start` and `stop` are compile-time integral constants, or
- `T[]&` otherwise (see the section on [unsized array references](#unsized-array-references)).

Array slices are useful when accessing `prog` data, which incurs overhead per access. Consider the following approach to rendering a 8x8 section of a 32x32 tilemap:

```c
void draw_tilemap(u8[32][32] prog& m, u8 r, u8 c)
{
    for(u8 row = 0; row < 8; ++row)
        for(u8 col = 0; col < 8; ++col)
            $draw_sprite(col * 8, row * 8, TILE_IMG, m[r + row][c + col]);
}
```

The above will perform 64 individual `prog` accesses, one for each tile. Performance can be improved by batching the accesses in each row, reducing the number of `prog` accesses from 64 to 8:

```c
void draw_tilemap(u8[32][32] prog& m, u8 r, u8 c)
{
    for(u8 row = 0; row < 8; ++row)
    {
        u8[16] rowdata = m[row][c +: 8];
        for(u8 col = 0; col < 8; ++col)
            $draw_sprite(col * 8, row * 8, TILE_IMG, rowdata[col]);
    }
}
```

## The `main` Function

Every ABC program must define a function called `main` accepting no arguments and returning `void`. The execution of an ABC program is structured as below.

1. Initialize global variables according to their initializers.
2. Loop forever:
   1. Call `main`. 

Because the `main` function automatically loops if it returns, it may be used for the game loop itself.

```c
void main()
{
    $draw_text(30, 36, "Hello World!");
    $display();
}
```

![Hello World screenshot](img/helloworld.png)

## System Functions
See [here](https://github.com/tiberiusbrown/abc/blob/master/docs/system.md) for the current list of system functions and predefined constants.

## Compiler Directives

Compiler directives affect the metadata packaged with the compiled ABC program. When exporting to `.arduboy` files, these directives may be used to populate various fields within the `info.json` file.

All compiler directives must occur before any variable, function, or `struct` definitions.

```c
// Defaults to "Untitled Arduboy Game"
#title "My Game"

// Defaults to "Unknown Author"
#author "Joe Smith"

// Defaults to "1.0"
#version "1.2"

// Defaults to date at time of compilation
#date "2020-04-16"

#description "This is a really cool game!"
#genre "Action"
#publisher "Joe Smith Inc"
#idea "Joe Smith"
#code "Joe Smith"
#art "Joe Smith"
#sound "Joe Smith"
#url "https://joesmith.com/arduboygame"
#sourceUrl "https://github.com/joesmith/arduboygame"
#email "joe@joesmith.com"
#companion "https://github.com/joesmith/arduboygame_editor"
```

### The `#shades` Directive

ABC has basic support for grayscale games with the `#shades` directive. Valid values of this directive are "2" (the default), "3", and "4". These values indicate how many shades of gray the game has available to use: the default value of "2" indicates a `BLACK` and `WHITE` game, while "3" adds `GRAY` and "4" adds `DARK_GRAY` and `LIGHT_GRAY`.

The `#shades` directive also affects how `sprites` data is encoded in the compiled program.

Note that enabling grayscale with `#shades "3"` or `#shades "4"` will reduce the amount of memory available for global variables from 1024 to 256 bytes. Additionally, for performance reasons, only the following system methods are available in grayscale modes:
- `$draw_filled_rect`
- `$draw_rect`
- `$draw_sprite`
- `$draw_text`

## Planned to Include in ABC (TODO)
- Function references

## Intentionally Excluded from ABC
- Pointers
- Comma operator
- Macros and preprocessor directives
- Variadic functions (except for system methods like `$format` which have special support)
- `goto`
