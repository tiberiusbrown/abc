# Language

- [Language](#language)
  - [Introduction](#introduction)
    - [Primitive Numeric Types](#primitive-numeric-types)
  - [Arrays](#arrays)
    - [Array Type Syntax](#array-type-syntax)
    - [Multidimensional Array Syntax](#multidimensional-array-syntax)
    - [Array Decay Behavior](#array-decay-behavior)
    - [Array Slices](#array-slices)
  - [Strings](#strings)
  - [References](#references)
    - [Unsized Array References](#unsized-array-references)
  - [Assets (sprites, fonts, etc)](#assets-sprites-fonts-etc)
    - [Sprite Sets](#sprite-sets)
    - [Fonts](#fonts)
    - [Tones](#tones)
  - [`constexpr` Variables](#constexpr-variables)
  - [System Functions](#system-functions)
  - [Predefined Constants](#predefined-constants)
  - [Planned to Include in ABC (TODO)](#planned-to-include-in-abc-todo)
  - [Intentionally Excluded from ABC](#intentionally-excluded-from-abc)
  - [Differences from C](#differences-from-c)
    - [String Literals](#string-literals)
    - [Bitwise Operations with `bool`](#bitwise-operations-with-bool)

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
- Control: `for`, `while`, `if` (`do`-`while` and `switch` not currently supported)

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
| `float` | float     | 32        |

## Arrays

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

### Array Slices

Given some array, array [reference](#references), or [UAR](#unsized-array-references) `a`, the syntax `a[start:stop]` is a slice into `a` including indices in the range `[start, stop)`. If the array elements are type `T`, the type of the array slice is
- `T[stop-start]&` if both `start` and `stop` are compile-time integral constants, or
- `T[]&` otherwise (see the section on [unsized array references](#unsized-array-references)).

## Strings

Strings in ABC are `char` arrays, where the length of the array is the capacity (maximum length) of the string. An ABC string does not require null termination if it occupies the full capacity of its array.

```c
// str1 will contain the 5 characters "Hello" followed by a null terminator.
char[12] str1 = "Hello";

// str2 will contain the 12 characters "Hello World!" and no null terminator.
char[12] str2 = "Hello World!";

// str3 will contain the 12 characters "abcdefghijkl" and no null terminator.
char[12] str3 = "abcdefghijklmnop";
```

## References
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

There are a number of built-in fonts that are automatically included when used. See the full list [here](https://github.com/tiberiusbrown/abc/blob/master/docs/builtin_fonts.md).

### Tones
The `tones` type identifies a sequence of musical tones. Each tone is defined by a musical note and a duration in milliseconds.

```c
// A two-tone note: B5 for 100 ms followed by E6 for 200 ms (e.g., platformer coin sound)
constexpr tones my_sfx = tones{ B5 100 E6 200 };

// In the future, MIDI import will be supported
// constexpr tones my_song = tones{ "assets/song.mid" };

$tones_play(my_sfx);

// As with other resource handle types, you can use a literal directly:
$tones_play(tones{ C4# 50 C4 50 C4b 50 });
```

## `constexpr` Variables
Numeric or asset handle variables can be declared `constexpr`. When declared `constexpr`, a variable occupies no storage and its value (calculated at compile time) is inserted directly into any expression in which the variable is used.

```cpp
// Numeric types may be declared constexpr
constexpr i16 X = 3;
constexpr float Y = X + 2;

// Asset handles may be declared constexpr as well
constexpr font MY_FONT = font{ 8 "assets/font.ttf" };
```

Certain language features, such as [array slices](#array-slices), have behavior that depends on whether or not an integral expression's value is computable at compile-time. Integer literals,  `constexpr` variables, and the `len()` operator on arrays and array references are guaranteed to produce compile-time constants.

## System Functions
See [here](https://github.com/tiberiusbrown/abc/blob/master/docs/system.md) for the current list of system functions.

## Predefined Constants
See [here](https://github.com/tiberiusbrown/abc/blob/master/docs/system.md) for the current list of predefined constants.

## Planned to Include in ABC (TODO)
- Function references

## Intentionally Excluded from ABC
- Pointers
- Comma operator
- Macros
- Variadic functions (except for system methods like `$format` which have special support)
- `goto`

## Differences from C

ABC differs from C in several aspects, both to simplify the language and to avoid inheriting some "gotchas" from C.

### String Literals
In C, a string literal is of type `char const[N]`, where `N` is the length of the string, including a null terminator.

In ABC, a string literal is of type `char[N] prog`, where `N` is the length of the string, *not* including a null terminator.

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
