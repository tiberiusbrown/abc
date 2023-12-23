# Building the Interpreter

The interpreter is located in the `ards_vm/` directory.

Building it requires a modified linker script: the `.beforedata` section must come before any other data section (that is, it must begin at 0x100, the origin of the data region).
This is to enable certain performance optimizations that require specific RAM addresses and alignment of the interpreter's data.

Making this change to the linker script (e.g., `avr5.xn`) will not affect other projects that do not also place data into a `.beforedata` section.

The linker script is part of the Arduino installation. On Windows, the linker script might be located at:

    `C:\Program Files (x86)\Arduino\hardware\tools\avr\avr\lib\ldscripts\avr5.xn`

You can modify the script to add the ".beforedata" section like this:

```
    ...
    
    .data : 
    {
        *(.beforedata)
        PROVIDE (__data_start = .) ;
        *(.data)
        *(.data*)
    
    ...
```

(Adding it before __data_start makes the ELF debug info make more sense.)
