# ABC Compiler and Interpreter for the Arduboy FX

ABC is a scripting language with an associated interpreter available for the Arduboy FX. It is primarily designed to be a gentle introduction to programming in a C-like statically typed language.

- [ABC Compiler and Interpreter for the Arduboy FX](#abc-compiler-and-interpreter-for-the-arduboy-fx)
  - [ABC: Differences from C](#abc-differences-from-c)
    - [Primitive Numeric Types](#primitive-numeric-types)
    - [Array Type Syntax](#array-type-syntax)
    - [Multidimensional Array Syntax](#multidimensional-array-syntax)
    - [Array Decay Behavior](#array-decay-behavior)
    - [String Literals](#string-literals)
    - [References](#references)
    - [Unsized Array References](#unsized-array-references)
    - [Bitwise Operations with `bool`](#bitwise-operations-with-bool)

## ABC: Differences from C

ABC differs from C in several aspects, both to simplify the language and to avoid inheriting some "gotchas" from C.

### Primitive Numeric Types

The following keyword types are exposed in ABC.

| Keyword | Type     | Bit Width |
|:-------:|----------|:---------:|
| `bool`  | boolean  | 8         |
|  `i8`   | signed   | 8         |
| `i16`   | signed   | 16        |
| `i24`   | signed   | 24        |
| `i32`   | signed   | 32        |
| `char`  | signed   | 8         |
| `int`   | signed   | 16        |
| `long`  | signed   | 32        |
|  `u8`   | unsigned | 8         |
| `u16`   | unsigned | 16        |
| `u24`   | unsigned | 24        |
| `u32`   | unsigned | 32        |
| `uchar` | unsigned | 8         |
| `uint`  | unsigned | 16        |
| `ulong` | unsigned | 32        |

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
