# ABC Compiler and Interpreter for the Arduboy FX

ABC is a scripting language with an associated interpreter available for the Arduboy FX. It is primarily designed to be a gentle introduction to programming in a C-like statically typed language.

## ABC: Differences from C

ABC differs from C in several aspects, both to simplify the language and to avoid inheriting some "gotchas" from C.

### No bitwise operations with `bool`

Consider the following conditional.
```c
// check if the lowest three bits of x are cleared
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
Naturally, if you actually want to perform a bitwise operation on boolean types, you may first cast any boolean expressions to integer types.
