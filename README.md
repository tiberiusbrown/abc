# ABC Compiler and Interpreter for the Arduboy FX

ABC is a scripting language with an associated interpreter available for the Arduboy FX. It is primarily designed to be a gentle introduction to programming in a C-like statically typed language.

## ABC: Differences from C

ABC differs from C in several aspects, sometimes to simplify the language and sometimes to disallow common coding mistakes to occur.

### No bitwise operations with `bool`

Consider the following conditional.
```c
if(x & 0x7 == 0)
    f();
```
Despite the obvious intention behind the code, due to operator precedence, this conditional always evaluates false -- it's equivalent to the form below.
```c
if(x & (0x7 == 0))
    f();
```
To avoid this mistake, ABC could adjust operator precedence to differ from C. However, this could prove poor training for someone who is learning to eventually move to C. Instead, ABC disallows performing bitwise operations with boolean types, so the original code would produce a compiler error. If you wish to perform a bitwise operation on boolean types, you may first cast to an integer type.
```c
if(x & u8(y == 2))
    f();
```
