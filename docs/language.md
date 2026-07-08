# Language

ABC is a C-like, statically typed language for the Arduboy FX. It aims to be memory-safe: array access is bounds-checked, arrays do not decay to pointers, and there are no pointers at all. The language keeps the syntax familiar, but deliberately leaves out the unsafe or ambiguous parts of C and C++.

This reference is organized from the surface syntax down to the runtime model.

- [Language](#language)
  - [Introduction](#introduction)
  - [Source Form](#source-form)
    - [Comments](#comments)
    - [Identifiers and Keywords](#identifiers-and-keywords)
    - [Literals](#literals)
  - [Types](#types)
    - [Primitive Numeric Types](#primitive-numeric-types)
    - [Storage Qualifiers](#storage-qualifiers)
    - [Arrays and Strings](#arrays-and-strings)
    - [Structs and Unions](#structs-and-unions)
    - [References](#references)
    - [Function References](#function-references)
    - [Asset Handles](#asset-handles)
      - [`sprites`](#sprites)
      - [`font`](#font)
      - [`tones`](#tones)
      - [`music`](#music)
      - [`tilemap`](#tilemap)
    - [Enums](#enums)
  - [Declarations](#declarations)
    - [Imports](#imports)
    - [Compound Literals](#compound-literals)
  - [Expressions](#expressions)
    - [Operator Precedence](#operator-precedence)
    - [Casts](#casts)
    - [Indexing, Members, and Slices](#indexing-members-and-slices)
    - [String Operations](#string-operations)
    - [Constant Expressions](#constant-expressions)
  - [Statements](#statements)
    - [Blocks and Scope](#blocks-and-scope)
    - [Conditionals and Loops](#conditionals-and-loops)
    - [Switch](#switch)
    - [Return, Break, and Continue](#return-break-and-continue)
  - [Program Structure](#program-structure)
  - [Built-in Constants](#built-in-constants)
  - [Compiler Directives](#compiler-directives)
    - [Metadata Directives](#metadata-directives)
    - [The `#shades` Directive](#the-shades-directive)
  - [System Functions](#system-functions)
  - [Intentionally Excluded](#intentionally-excluded)

## Introduction

ABC is designed around a few simple rules:

- It is statically typed and mostly C-like in expression syntax and control flow.
- Arrays are value types, not pointers.
- References are explicit in the type system and cannot be re-seated.
- Global data can live in RAM, in program memory, or in persistent save data.
- The compiler folds many constant expressions at compile time.

Top-level names all share one namespace. Functions, globals, structs, enums, and imported definitions must therefore have unique names.

The compiler collects top-level declarations before generating code, so forward references are allowed in normal source order.

## Source Form

### Comments

ABC uses ordinary C-style comments.

```c
// line comment
/* block comment */
```

### Identifiers and Keywords

Identifiers are made from letters, digits, and underscores, and may optionally begin with `$`. The `$` prefix is conventionally used for system functions.

The reserved keywords are:

`u8`, `i8`, `u16`, `i16`, `u24`, `i24`, `u32`, `i32`, `void`, `bool`, `char`, `uint`, `int`, `ulong`, `long`, `sprites`, `font`, `tones`, `music`, `tilemap`, `constexpr`, `saved`, `prog`, `if`, `else`, `while`, `for`, `return`, `break`, `continue`, `struct`, `import`, `len`, `float`, `byte`, `enum`, `do`, `switch`, `case`, and `default`.

The parser also reserves `uchar`, but it is not currently accepted as a usable primitive type alias.

### Literals

Numeric literals, character literals, and string literals are the basic literal forms.

Numeric literals:

- Decimal integers use digits only, with an optional `u` suffix, for example `42` or `42u`.
- Hex integers use the `0x` prefix, for example `0xff` or `0xffu`.
- Floating-point literals support a decimal point and/or exponent, for example `1.5`, `.5`, `1.`, `1e3`, or `2.0e-1`.

Integer literals are automatically given the smallest fitting primitive integer type. Decimal literals default to signed; hex literals default to signed only while the signed range fits.

Character literals are single-quoted and use the same escape sequences as strings.

String literals are double-quoted. The supported escapes are `\0`, `\n`, `\r`, `\t`, `\"`, `\'`, `\\`, and `\xHH`.

Adjacent quoted string literal pieces are concatenated by the parser, so `"Hello" " World"` is one literal.

A string literal expression has type `char[N] prog&`, where `N` is the number of characters after escape processing and before any terminator is added by assignment into a larger destination.

## Types

### Primitive Numeric Types

| Type | Bits | Notes |
|---|---:|---|
| `void` | 0 | Only valid as a function return type. |
| `bool` | 8 | Logical false/true. Converts to `0` or `1`. |
| `byte` | 8 | Raw unsigned byte type, useful for memory views. |
| `char` | 8 | Character type used for strings. |
| `u8` | 8 | Unsigned integer. |
| `u16` | 16 | Unsigned integer. |
| `u24` | 24 | Unsigned integer. |
| `u32` | 32 | Unsigned integer. |
| `i8` | 8 | Signed integer. |
| `i16` | 16 | Signed integer. |
| `i24` | 24 | Signed integer. |
| `i32` | 32 | Signed integer. |
| `float` | 32 | IEEE-754 single-precision floating point. |

Common aliases:

`short` = `i8`, `int` = `i16`, `long` = `i32`, `ushort` = `u8`, `uint` = `u16`, and `ulong` = `u32`.

Notes:

- `bool` is a real 8-bit type, not a C++-style special case.
- `char` is an unsigned 8-bit type and is treated specially in string contexts.
- `byte` is intended for raw memory and byte-oriented operations.
- `float` arithmetic is 32-bit single precision.
- `bool` may participate in arithmetic like other primitives, but bitwise operators do not accept `bool`.

### Storage Qualifiers

ABC has three important storage modifiers.

`prog`

- `prog` stores the value in program memory instead of RAM.
- It is a postfix type modifier, so write `T[N] prog`, not `T prog[N]`.
- `prog` globals must be initialized.
- `prog` values are read-only at runtime from the language's point of view.
- `prog` is a global-only concept; local variables may not be `prog`.

`constexpr`

- `constexpr` values are compile-time only and occupy no storage.
- `constexpr` is allowed on primitive numeric types and asset-handle types.
- `constexpr` values can be local or global.
- `constexpr` declarations must be initialized.

`saved`

- `saved` marks a global value that is persisted in save data.
- `saved` is global-only.
- `saved` values may not be references or contain references of any kind.
- `saved` values participate in the runtime save/load system.

Ordinary non-`prog`, non-`constexpr` variables without initializers are zero-initialized.

### Arrays and Strings

Arrays are written as `T[N]`, where `N` is the number of elements. Multidimensional arrays are written inside-out compared with C: `u8[MAX_ITEMS][COLS][ROWS] map` in ABC corresponds to `u8 map[ROWS][COLS][MAX_ITEMS]` in C.

Arrays are value types:

- They do not decay to pointers.
- Passing an array to a function passes a copy.
- Arrays can be returned from functions if the type is copyable and not `prog`.

Array indices are zero-based and bounds-checked.

Arrays can be nested and sliced through their contiguous storage. For example:

```c
u16[2][3] a = { {1, 2}, {3, 4}, {5, 6} };
u16[]& r = a;
$assert(len(r) == 6);
$assert(r[0] == 1);
$assert(r[5] == 6);
```

Strings are `char` arrays.

- A string's capacity is the array length.
- `len(s)` returns the capacity, not the runtime NUL-terminated length.
- Use `$strlen(s)` from the system functions to measure the current NUL-terminated text length.
- String literals live in program memory and behave like `char[N] prog&`.
- Assigning one `char` array to another can resize the destination logically; other array types must match exactly.
- String equality and inequality are supported directly.
- String concatenation is supported through `+` and `+=` in assignment contexts.

Examples:

```c
char[20] s = "Hello";
s += " World!";
$assert(s == "Hello World!");
```

### Structs and Unions

`struct` and `union` define aggregate types.

```c
struct enemy_t
{
    int x;
    int y;
    char[12] attrs;
};
```

Rules:

- `struct` and `union` members may not be declared `prog`.
- `union` members must be copyable.
- `union` initializers may contain only one element.
- Missing trailing members are zero-initialized when the type allows it.
- Any member that is a reference must be explicitly initialized.

### References

ABC has references, but not pointers.

- `T&` is a reference to `T`.
- References must be initialized.
- References cannot be re-seated.
- There is no address-of operator in expression syntax.
- A reference to a reference is not allowed.

Unsized array references are written as `T[]&`.

- They store the referenced array's length as well as the reference itself.
- They are useful for functions that accept arrays of any length.
- `T[] prog&` is the prog-memory variant.
- `byte[]&` is a raw byte view and can be created from any copyable referenced value.

Non-`prog` references make a composite value noncopyable. `prog` references and function references are copyable.

Example:

```c
int sum(int[]& a)
{
    int t = 0;
    for(u16 i = 0; i < len(a); i = i + 1)
        t = t + a[i];
    return t;
}
```

### Function References

Function references are a special copyable reference-like type. Their syntax is:

` :return_type(arg_types_list)& `

The argument names in the type syntax are optional and ignored.

Examples:

```c
u8 f(u8 x, i8 y) { return x + y + 2; }
u8 g(u8 x, i8 y) { return x + y + 3; }

void main()
{
    :u8(u8 x, i8 y)& r = f;
    $assert(r(0, 0) == 2);
    r = g;
    $assert(r(0, 0) == 3);
}
```

Notes:

- A bare function name in expression position evaluates to a function reference.
- Function references are reassignable.
- The referenced function signatures must match exactly.

### Asset Handles

ABC has five opaque asset-handle types:

- `sprites`
- `font`
- `tones`
- `music`
- `tilemap`

These values identify data stored in program memory. They are copyable and can be `constexpr`, but they are not numeric values and may not be used in arithmetic expressions.

Same-type comparisons are allowed, but the handle value itself is otherwise opaque.

Asset literal file paths are resolved relative to the current source file.

#### `sprites`

`sprites{ ... }` creates a sprite-set handle.

Two forms are supported:

```c
constexpr sprites DIGITS = sprites{
    3x5
    .X.  .X.  XX.
    X.X  XX.  ..X
    X.X  .X.  .X.
    X.X  .X.  X..
    .X.  XXX  XXX
};

constexpr sprites TITLE = sprites{ "assets/title.png" };
constexpr sprites TILES = sprites{ 16x16 "assets/tileset.png" };
```

Notes:

- The size fields are decimal literals, not general expressions.
- If width and height are omitted for a file-backed sprite literal, the image is treated as a single sprite.
- If width and height are given, the loaded image must be a multiple of that sprite size.
- In ASCII-art sprite data, whitespace is ignored, `-` is transparent, `.` is the darkest visible shade, `1` and `2` step through brighter shades, and any other non-whitespace character becomes the brightest shade for the active `#shades` mode.

#### `font`

`font{ size "file.ttf" }` loads a font from a TTF file.

```c
constexpr font f = font{ 12 "assets/font.ttf" };
```

Builtin fonts are also exposed as `constexpr font` globals. See [builtin_fonts.md](builtin_fonts.md).

#### `tones`

`tones` describes a monophonic sequence of notes.

```c
constexpr tones my_sfx = tones{ B5 100 E6 200 };
constexpr tones from_midi = tones{ "assets/sound.mid" };
```

Notes:

- Inline tones use note-duration pairs, where the duration is in milliseconds.
- RTTTL strings are also accepted.
- `-` and `P` denote silence/rest.

#### `music`

`music` is like `tones`, except the encoded sequence may contain up to two simultaneous notes.

```c
constexpr music my_song = music{ "assets/song.mid" };
```

#### `tilemap`

`tilemap` is a 2D array of tile indices.

```c
constexpr tilemap TM = tilemap{
    16x8
    18,19,146,59,134,155,170,6,171,37,37,38,27,17,19,43,
    35,161,162,163,28,134,155,154,7,7,7,135,74,52,52,75
    // ...
};

$draw_tilemap(x, y, sprites{ 16x16 "tiles.png" }, tilemap{ "world.tmx" });
```

Notes:

- Inline tilemaps use decimal width and height literals followed by row-major tile data.
- TMX imports can optionally name the tile layer. If no layer name is given, the first tile layer is used.

### Enums

ABC supports named and anonymous enums.

```c
enum foo_t
{
    BLAH,
    BLAH2,
};

enum
{
    BLAH3 = 17,
    BLAH4,
};
```

Rules:

- Enumerator names are introduced into the global namespace as compile-time constants.
- Enumerator values may be explicit or auto-incremented.
- Explicit values must be constant expressions.
- Enum values may be negative.
- The underlying integer type is chosen automatically to fit the values.

## Declarations

The basic declaration forms are:

```c
T x;
T x = expr;
constexpr T x = expr;
saved T x;
```

Multiple declarators can share one type:

```c
int x, y, z;
```

Declaration rules:

- `constexpr` and `prog` variables must be initialized.
- `prog` variables must be global.
- `saved` variables must be global.
- A local `saved` variable is invalid.
- A local `prog` variable is invalid.
- Non-`constexpr`, non-`prog` variables without initializers are zero-initialized.
- Top-level names share one namespace, so duplicate names across functions, globals, structs, enums, and imports are not allowed.

Functions are declared with the usual C-like syntax:

```c
u8 f(u8 x, i8 y) { return x + y + 2; }
```

User-defined functions are uniquely named; overloading is not supported.

### Imports

Imports are top-level statements:

```c
import math.fixed;
```

Rules:

- The import path is dot-separated identifiers.
- `import math.fixed;` loads `math/fixed.abc` relative to the current source file's directory.
- Imported files are compiled once, even if imported multiple times.
- Import loops are rejected.

### Compound Literals

Compound literals use braces and are positional.

```c
enemy_t e = { 42, 7, "blue" };
u8[4] a = { 1, 2, 3, 4 };
u8[2][3] m = { {1, 2}, {3, 4}, {5, 6} };
```

Rules:

- `{}` is allowed and zero-initializes where the target type permits it.
- Nested aggregates use nested braces.
- Missing trailing array or struct elements are zero-initialized when the type is copyable and does not require references.
- Reference members must be explicitly initialized.
- Unions accept only one element.
- Compound literals are positional; designated initializers are not supported.

## Expressions

### Operator Precedence

Higher rows bind tighter.

| Level | Operators | Associativity |
|---|---|---|
| 1 | postfix `()`, `[]`, `[:]`, `.`, `++`, `--` | left to right |
| 2 | prefix `++`, `--`, `!`, `-`, `~`, casts | right to left |
| 3 | `*`, `/`, `%` | left to right |
| 4 | `+`, `-` | left to right |
| 5 | `<<`, `>>` | left to right |
| 6 | `<`, `<=`, `>`, `>=` | left to right |
| 7 | `==`, `!=` | left to right |
| 8 | `&` | left to right |
| 9 | `^` | left to right |
| 10 | `|` | left to right |
| 11 | `&&` | left to right |
| 12 | `||` | left to right |
| 13 | `?:` | right to left |
| 14 | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` | right to left |

Notes:

- Logical operators short-circuit.
- Bitwise operators do not accept `bool` or floating-point operands.
- Right shift is arithmetic for signed values and logical for unsigned values.
- Mixed numeric expressions use C-like implicit conversions.

### Casts

Primitive type names used in call position are treated as casts.

```c
u8 x = u8(300);
float y = float(x);
```

Rules:

- Primitive casts take exactly one argument.
- Casts are only for primitive numeric types.
- Function-like syntax with a non-primitive name remains a normal function call.

### Indexing, Members, and Slices

Array indexing uses `[]`, struct/union member access uses `.`, and slices use either `start:stop` or `start+:length`.

Examples:

```c
u8[10] a;
u8 x = a[3];
u8 y = a[2:5][1];
u8 z = a[2+:3][0];
```

Rules:

- Array indices are zero-based.
- Array and slice access is bounds-checked.
- Constant indices into fixed arrays can be folded to a constant offset.
- Slice bounds must not be floating-point values.
- A slice with compile-time-known bounds can become a sized array reference.
- Slices on multidimensional arrays operate on the flattened contiguous storage.

### String Operations

String values are `char` arrays or references to `char` arrays.

Supported operations:

- `==` and `!=` compare strings lexicographically.
- `+` and `+=` concatenate strings in assignment contexts.
- Assigning one string to another copies the bytes, truncating or zero-filling as needed by the destination capacity.

For ordering comparisons, use `$strcmp` from the system functions.

### Constant Expressions

ABC folds many expressions at compile time.

Constant expressions are used for:

- `constexpr` initializers
- array dimensions
- enum values
- `switch` case values and ranges
- some slices and `len()` results

The compiler recognizes integer literals, floating-point literals, enum values, `constexpr` values, and `len()` on statically sized arrays as compile-time constants.

## Statements

### Blocks and Scope

Blocks are written with braces.

```c
{
    int x = 1;
    int y = x + 2;
}
```

Rules:

- A block introduces a new local scope.
- Locals declared without an initializer are zero-initialized.
- Declarations can appear alongside statements inside a block.

### Conditionals and Loops

ABC supports the usual C-like control flow forms:

- `if` / `else`
- `while`
- `do` / `while`
- `for`

Examples:

```c
if(t) foo();
else bar();

while(cond)
    step();

do
    step();
while(cond);

for(int i = 0; i < 10; ++i)
    step();
```

Rules:

- Conditions are converted to `bool`.
- `for` allows a declaration or an empty statement in the init clause.
- `for` bodies can be a single statement or a block.
- `break` and `continue` work as in C for loops.

### Switch

ABC's `switch` syntax is not C's syntax.

```c
switch(x)
{
case(1 ... 3, 7) f();
case(8 ... 10)   g();
default          h();
}
```

Rules:

- The switch expression must be a primitive non-floating value.
- Case values must be integral constant expressions.
- Multiple case values may appear in one `case(...)`.
- Inclusive ranges use `...`.
- `default` is optional and may appear anywhere, but only one `default` is allowed.
- Overlapping case values or ranges are errors.
- A case body is any single statement, including a block.
- There is no implicit fallthrough. Use `continue` to fall through to the next case.
- `break` exits the switch.

### Return, Break, and Continue

`return` may appear with or without an expression depending on the function return type.

Rules:

- Returning a value from a `void` function is an error.
- Omitting a value from a non-`void` function is an error.
- `break` and `continue` are only valid inside an active loop or `switch`.
- In a `switch`, `continue` means fall through to the next case, not "continue the enclosing loop".

## Program Structure

Every program must define:

```c
void main()
{
    // ...
}
```

Rules:

- `main` must return `void`.
- `main` must take no arguments.
- Global variables are initialized before the first call to `main`.
- After `main` returns, the runtime calls it again, so `main` naturally forms the game loop.

## Built-in Constants

ABC provides a small set of predefined `constexpr` globals.

Palette constants:

- `BLACK`
- `DARK_GRAY` and `DARK_GREY`
- `GRAY` and `GREY`
- `LIGHT_GRAY` and `LIGHT_GREY`
- `WHITE`

Other predefined constants:

- `SHADES` - the active grayscale mode count
- `A_BUTTON`
- `B_BUTTON`
- `UP_BUTTON`
- `DOWN_BUTTON`
- `LEFT_BUTTON`
- `RIGHT_BUTTON`
- `PI`

Notes:

- The palette constants are remapped by `#shades`.
- The button constants are bit masks.
- `PI` is a `float`.

## Compiler Directives

Compiler directives affect exported metadata and a few build-time settings.

Rules:

- Directives must appear before any non-directive top-level statement in the file.
- Directive values must be string literals.
- The compiler accepts the directives listed below.

### Metadata Directives

The metadata directives populate the exported `.arduboy` `info.json` fields:

- `#title`
- `#author`
- `#version`
- `#description`
- `#date`
- `#genre`
- `#publisher`
- `#idea`
- `#code`
- `#art`
- `#sound`
- `#url`
- `#sourceUrl`
- `#email`
- `#companion`

Defaults:

- `#title` defaults to `"Untitled Arduboy Game"`.
- `#author` defaults to `"Unknown Author"`.
- `#version` defaults to `"1.0"`.
- `#date` defaults to the current local date in `YYYY-MM-DD` form.
- The remaining metadata fields default to empty/unset.

### The `#shades` Directive

ABC has basic support for grayscale games through `#shades`.

Accepted values:

- `"2"` - the default classic black and white mode
- `"3"` - three-shade grayscale
- `"4"` - four-shade grayscale

Effects:

- The palette constants are remapped to match the active mode.
- Sprite data is encoded for the selected shade count.
- Available RAM for globals drops from 1024 bytes to 256 bytes in grayscale modes.

See [docs/system.md](system.md) for the per-function grayscale behavior of the built-in system functions.

## System Functions

System functions are the built-in `$`-prefixed functions such as `$draw_sprite`, `$save`, and `$assert`.

For the full catalog, signatures, and detailed behavior, see [docs/system.md](system.md).

## Intentionally Excluded

ABC intentionally leaves out several C/C++ features:

- Pointers
- Pointer arithmetic
- The address-of and dereference operators
- `goto` and labels
- Macros and preprocessor directives
- Variadic user-defined functions
- The comma operator
- C-style `const`
- User-defined function overloading

