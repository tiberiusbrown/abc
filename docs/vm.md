# ABC Bytecode VM

This document describes the bytecode virtual machine used by ABC programs. It is
based on the portable interpreter in `interp_generic/abc_interp.c`, the Arduboy
interpreter in `interp_arduboy/ards_vm.cpp` and `interp_arduboy/vm_sys.cpp`, and
the compiler/assembler metadata in `interp_arduboy/abc_instr.hpp` and
`src/abc_assembler.cpp`.

The VM is a byte-addressed stack machine. It has no typed runtime values; the
compiler decides how many bytes each value occupies and which opcode variant to
use. Integers, pointers, asset handles, array references, floats, and function
references are all represented as raw bytes on the same data stack.

## Implementations

There are two interpreter implementations for the same bytecode contract.

`interp_generic` is a portable C interpreter. The host supplies program-byte
reads, input, time, debug output, entropy, and optional save persistence through
`abc_host_t`. `abc_run()` executes one instruction and returns one of:

| Result | Meaning |
|---|---|
| `ABC_RESULT_NORMAL` | Continue executing. |
| `ABC_RESULT_IDLE` | Yield to the host for a short time or until frame timing catches up. |
| `ABC_RESULT_BREAK` | A debug break syscall was reached. |
| `ABC_RESULT_ERROR` | A runtime error halted execution. |

`interp_arduboy` is the Arduboy FX runtime. It uses the same opcode numbers, but
the main dispatch loop is AVR assembly that streams bytes from FX program data.
It keeps one top-of-stack byte cached in `r9` while executing, stores VM state at
fixed SRAM addresses, and displays runtime errors with file/line stack traces
using the compiled debug tables.

The opcode enum in `interp_arduboy/abc_instr.hpp` is the shared source of truth
for opcode order. The generic interpreter has a matching local enum and must stay
in sync.

## Binary Layout And Startup

Compiled binaries start with a 256-byte header. See `docs/binary_format.md` for
the full file layout. The fields most relevant to the VM are:

| Offset | Size | Meaning |
|---:|---:|---|
| `0x00` | 4 | Signature `AB C0 0A BC`. |
| `0x0a` | 2 | Saved-global byte count, little-endian. |
| `0x0c` | 1 | File table entry count. |
| `0x0d` | 3 | File table offset. |
| `0x10` | 3 | Line table offset. |
| `0x13` | 1 | Number of display shades, normally `2`, `3`, or `4`. |
| `0x14` | 12 | Startup bytecode. |
| `0x20` | 24 | Build date and short git hash text. |

The startup bytecode at `0x14` is:

```asm
call $globinit
call main
jmp  24
```

Execution therefore runs global initialization once, then repeatedly calls
`main`. Full-width program addresses are 24-bit offsets from the beginning of
the compiled binary. The Arduboy implementation adds the FX data-page base when
seeking, but bytecode still sees logical 24-bit binary offsets.

The portable interpreter treats `pc == 0` as reset. On reset it clears globals,
display buffers, stack metadata, audio state, input state, initializes frame
timing, reads the shade count from header offset `0x13`, initializes the PRNG,
and sets `pc = 0x14`. The Arduboy runtime performs equivalent initialization in
`vm_run()`, verifies the signature, then starts the assembly dispatch loop at
`0x14`.

All immediates in bytecode are little-endian unless specifically noted in a data
format outside the VM instruction stream.

## Values And Byte Order

The stack stores multi-byte values low byte first. A pushed `u16` value `0x1234`
occupies two stack bytes:

```text
lower address: 34 12 :higher address
```

The logical top of stack is the highest-addressed live byte. Pop helpers read the
highest byte first and reconstruct the little-endian value. In stack-effect
notation below, multi-byte values such as `u16`, `ref16`, and `pref24` are shown
as logical values even though they occupy multiple bytes.

Primitive sizes used by the compiler are:

| Source-level kind | VM bytes |
|---|---:|
| `u8`, `i8`, `bool`, `char`, `byte` | 1 |
| `u16`, `i16`, `uint`, `int` | 2 |
| `u24`, `i24` | 3 |
| `u32`, `i32`, `ulong`, `long` | 4 |
| `float` | 4 |
| RAM reference | 2 |
| Program reference, asset handle, function reference | 3 |
| RAM unsized array reference | 4: `ref16` then `len16` |
| Program unsized array reference | 6: `pref24` then `len24` |

Signed integer values use two's-complement representation. The VM only cares
about signedness in signed division/modulo, signed comparisons, arithmetic right
shift, sign extension, and float conversions.

## Runtime State And Memory Spaces

The generic interpreter stores its full state in `abc_interp_t`. The Arduboy
interpreter stores the bytecode-visible state in a linker-pinned `ards::vm_t`
object and keeps additional device state in Arduboy/FX/audio globals.

The Arduboy VM layout is intentionally address-sensitive:

| Offset | Field | Meaning |
|---:|---|---|
| `0x100` | `stack[256]` | Data stack bytes. |
| `0x200` | `globals[1024]` | Global RAM, or `globals[0x100]` plus two grayscale buffers. |
| `0x600` | `calls[16]` | 24-bit return-address call stack. |
| `0x630` | `sp` | 8-bit data stack pointer. |
| `0x631` | `pc` | 24-bit program counter. |
| `0x634` | `csp` | Call stack pointer. |
| `0x635` | `error` | Last Arduboy VM error code. |
| `0x636` | `frame_dur` | Target frame duration. |
| `0x637` | `frame_start` | Frame timing anchor. |
| `0x638` | `needs_render` | Grayscale render flag. |
| `0x639` | `text_mode` | Sprite/text draw mode. |
| `0x63a` | `text_font` | 24-bit current font asset address. |
| `0x63d` | `current_plane` | Current grayscale plane. |

### Program Space

Program space is read-only byte-addressed data accessed through 24-bit logical
addresses. It contains the header, program data/assets, bytecode, file table, and
line table. Program references are plain 24-bit addresses, not tagged pointers.
`GETP`/`GETPN`, program array indexing, asset syscalls, and indirect calls use
these addresses.

### Data Stack

The data stack is a fixed 256-byte byte array. The stack pointer is 8-bit, and
compiler frame offsets must fit in one byte. Runtime push helpers trap before
the stack pointer wraps. Bytecode is expected to be stack-balanced; stack
underflow is not uniformly checked in the portable interpreter.

Locals, temporaries, function arguments, return slots, references, and array
references all live on the same stack. There is no separate VM frame object.
Local instructions address bytes relative to the current stack top.

### Call Stack

The call stack stores return program counters only. It does not store data-stack
frame pointers.

| Implementation | Call stack depth |
|---|---:|
| `interp_generic` | 24 return addresses |
| `interp_arduboy` | 16 return addresses |

`CALL`, `CALL1`, `CALL2`, and `ICALL` push the current `pc` on this call stack.
`RET` pops it back into `pc`. Overflow is a runtime error; returning with an
empty call stack is an error in the portable interpreter.

### Globals

The VM has 1024 bytes of global RAM. The assembler lays out saved globals first,
then non-saved globals. Global references use the tagged address range
`0x0200..0x05ff`, which maps to `globals[0..1023]`.

In 2-shade builds, compiled global data may use all 1024 bytes. In 3-shade and
4-shade builds, compiled global data is capped at 256 bytes because the upper
768 bytes are used by grayscale render command buffers. The portable interpreter
mirrors this by treating `globals[256..1023]` as command-buffer storage in
grayscale paths.

### Saved Data

The binary header records the number of saved global bytes. Saved globals occupy
the prefix of global RAM. `save()` copies that prefix to persistent storage, and
`load()` copies it back if a save exists.

Maximum saved bytes follow the same global-memory constraint:

| Shade mode | Max saved bytes |
|---|---:|
| 2 shades | 1024 |
| 3 or 4 shades | 256 |

If the header save size is larger than the active mode supports, the portable
interpreter treats the save size as zero.

### Display And Host State

The portable VM has:

| Field | Size | Role |
|---|---:|---|
| `display_buffer` | 1024 | 128x64 one-bit draw buffer. |
| `display` | 8192 | 128x64 8-bit host-visible pixels. |
| audio fields | implementation-defined | Tone/music playback state. |
| input fields | a few bytes | Previous/current button masks. |

The Arduboy runtime draws to the Arduboy screen buffer and hardware/FX state
instead of exposing the generic `display` array.

## Reference Encoding

The VM has tagged RAM references and untagged program references.

| Reference | Bytes | Encoding |
|---|---:|---|
| Stack byte reference | 2 | `0x0100 + stack_index`, valid through `0x01ff`. |
| Global byte reference | 2 | `0x0200 + global_index`, valid through `0x05ff`. |
| Program byte reference | 3 | Logical program-space byte address. |

`GETR*`, `SETR*`, post-increment/decrement, and RAM unsized array operations use
tagged 16-bit RAM references. `GETP*`, program array operations, function
references, and asset handles use 24-bit program references.

`REFL imm8` creates a stack reference to the byte at `sp - imm8`.
`REFGB imm8` creates a global reference to one of the first 256 global bytes.
Wide global references are usually emitted as a 16-bit literal equal to
`0x0200 + global_offset`.

Invalid tagged RAM references are runtime errors when dereferenced. The compiler
and assembler are responsible for emitting valid global offsets and balanced
stack references.

## Compiler Calling Convention

The bytecode VM itself only implements `CALL` and `RET`. The rest of the calling
convention is generated by the compiler.

For non-system functions:

1. The caller reserves space for the return value, if any.
2. The caller pushes arguments in reverse declaration order.
3. `CALL` stores only the return `pc`.
4. The callee treats arguments as locals at fixed offsets from the current stack
   size. Function arguments are entered into the compiler frame in reverse order
   so source argument names resolve to stable offsets.
5. On `return expr`, the callee evaluates `expr`, stores it into the reserved
   return slot with `SETLN`, pops the rest of its live frame/arguments with
   `POPN`, and executes `RET`.

For system functions:

1. The caller pushes arguments in reverse declaration order.
2. `SYS` dispatches to a native implementation.
3. The native implementation pops its own arguments and pushes any return value.

Format syscalls (`draw_textf`, `format`, and `debug_printf`) consume a raw
NUL-terminated format pointer plus explicitly pushed arguments. The interpreters
implement only a minimal `printf`-style parser for the conversions already used
by the runtime (`%%`, `%c`, `%s`, `%S`, `%d`, `%u`, `%x`, `%f`, plus the
existing single-digit zero-pad / precision forms). `format` now treats its
destination capacity as total buffer size and reserves space for a terminating
NUL whenever the capacity is nonzero.

## Instruction Set

Real bytecode opcodes are `0x00..0xba`. `NUM_INSTRS` ends the real instruction
set. `I_REMOVE`, `I_PUSH2`, and `I_PUSH3` are compiler/optimizer pseudo-
instructions and are not emitted as bytecode opcodes; the assembler lowers
`push2` to `I_PUSHG` and `push3` to `I_PUSHL`.

Stack notation uses `before -> after`, with the rightmost item as top of stack.
For binary operators, `a b -> r` means `b` was on top and is popped first.

### Constants, Stack, And Locals

| Opcode | Mnemonic | Immediate | Stack | Behavior |
|---:|---|---|---|---|
| `0x00` | `NOP` | none | unchanged | Do nothing. |
| `0x01` | `PUSH` | `u8` | `-> u8` | Push one immediate byte. |
| `0x02..0x0a` | `P0..P8` | none | `-> u8` | Push byte constants `0` through `8`. |
| `0x0b..0x0e` | `P16`, `P32`, `P64`, `P128` | none | `-> u8` | Push byte constants `16`, `32`, `64`, or `128`. |
| `0x0f` | `P00` | none | `-> 0 0` | Push two zero bytes. |
| `0x10` | `P000` | none | `-> 0 0 0` | Push three zero bytes. |
| `0x11` | `P0000` | none | `-> 0 0 0 0` | Push four zero bytes. |
| `0x12` | `PZ8` | none | `-> 8 bytes` | Push eight zero bytes. |
| `0x13` | `PZ16` | none | `-> 16 bytes` | Push sixteen zero bytes. |
| `0x14` | `PUSHG` | `u16` | `-> u16` | Push two immediate bytes. Also used for wide global refs. |
| `0x15` | `PUSHL` | `u24` | `-> u24` | Push three immediate bytes, usually a program label address. |
| `0x16` | `PUSH4` | `u32` | `-> u32` | Push four immediate bytes. |
| `0x17` | `SEXT` | none | `i8 -> i16` | Push `0xff` if the current top byte has bit 7 set, otherwise `0x00`. |
| `0x18` | `SEXT2` | none | `i8 -> i24` | Sign-extend by two bytes. |
| `0x19` | `SEXT3` | none | `i8 -> i32` | Sign-extend by three bytes. |
| `0x1a..0x21` | `DUP..DUP8` | none | `... -> ... byteN` | Copy the Nth byte below the top. `DUP` copies top byte, `DUP2` the next byte, etc. |
| `0x22..0x29` | `DUPW..DUPW8` | none | `... lo hi -> ... lo hi lo hi` | Copy a two-byte word. `DUPW` copies the top word; higher suffixes copy a word deeper in the stack. |
| `0x2a` | `GETL` | `u8 off` | `-> byte` | Copy one local byte at stack-top-relative offset `off`; `off == 1` means current top byte. |
| `0x2b` | `GETL2` | `u8 off` | `-> 2 bytes` | Copy two local bytes. |
| `0x2c` | `GETL4` | `u8 off` | `-> 4 bytes` | Copy four local bytes. |
| `0x2d` | `GETLN` | `u8 n, u8 off` | `-> n bytes` | Copy `n` local bytes. |
| `0x2e` | `SETL` | `u8 off` | `byte ->` | Pop one byte and store it to a local slot. |
| `0x2f` | `SETL2` | `u8 off` | `2 bytes ->` | Pop two bytes and store them to a local slot. |
| `0x30` | `SETL4` | `u8 off` | `4 bytes ->` | Pop four bytes and store them to a local slot. |
| `0x31` | `SETLN` | `u8 n, u8 off` | `n bytes ->` | Pop `n` bytes and store them to a local slot, preserving byte order. |
| `0x45..0x48` | `POP..POP4` | none | `n bytes ->` | Drop one to four bytes. |
| `0x49` | `POPN` | `u8 n` | `n bytes ->` | Drop `n` bytes. |
| `0x4a` | `ALLOC` | `u8 n` | `-> n bytes` | Reserve `n` stack bytes. The contract is undefined contents; the generic interpreter currently zero-fills, while the Arduboy interpreter just advances the stack. |

### Globals, Program Reads, And References

| Opcode | Mnemonic | Immediate | Stack | Behavior |
|---:|---|---|---|---|
| `0x32` | `GETG` | `u16 gref` | `-> byte` | Push `globals[gref - 0x0200]`. |
| `0x33` | `GETG2` | `u16 gref` | `-> 2 bytes` | Push two global bytes. |
| `0x34` | `GETG4` | `u16 gref` | `-> 4 bytes` | Push four global bytes. |
| `0x35` | `GETGN` | `u8 n, u16 gref` | `-> n bytes` | Push `n` global bytes. |
| `0x36` | `GTGB` | `u8 goff` | `-> byte` | Short form for global offsets in the first 256 bytes. |
| `0x37` | `GTGB2` | `u8 goff` | `-> 2 bytes` | Short two-byte global load. |
| `0x38` | `GTGB4` | `u8 goff` | `-> 4 bytes` | Short four-byte global load. |
| `0x39` | `SETG` | `u16 gref` | `byte ->` | Pop one byte into globals. |
| `0x3a` | `SETG2` | `u16 gref` | `2 bytes ->` | Pop two bytes into globals. |
| `0x3b` | `SETG4` | `u16 gref` | `4 bytes ->` | Pop four bytes into globals. |
| `0x3c` | `SETGN` | `u8 n, u16 gref` | `n bytes ->` | Pop `n` bytes into globals. |
| `0x3d` | `GETP` | none | `pref24 -> byte` | Pop a program address and push one program byte. |
| `0x3e` | `GETPN` | `u8 n` | `pref24 -> n bytes` | Pop a program address and push `n` consecutive program bytes. |
| `0x3f` | `GETR` | none | `ref16 -> byte` | Pop a tagged RAM reference and push one byte. |
| `0x40` | `GETR2` | none | `ref16 -> 2 bytes` | Load two bytes from a tagged RAM reference. |
| `0x41` | `GETRN` | `u8 n` | `ref16 -> n bytes` | Load `n` bytes from a tagged RAM reference. |
| `0x42` | `SETR` | none | `byte ref16 ->` | Pop a reference and one byte, then store the byte. |
| `0x43` | `SETR2` | none | `2 bytes ref16 ->` | Store two bytes through a tagged RAM reference. |
| `0x44` | `SETRN` | `u8 n` | `n bytes ref16 ->` | Store `n` bytes through a tagged RAM reference. |
| `0x54` | `REFL` | `u8 off` | `-> ref16` | Push stack reference `0x0100 + sp - off`. |
| `0x55` | `REFGB` | `u8 off` | `-> ref16` | Push global reference `0x0200 + off`. |

Global `GETG*` and `SETG*` immediates are encoded as tagged references
(`0x0200 + offset`) by the assembler. The short `GTGB*` forms use the raw
0..255 offset.

### Array Indexing And Slicing

Array operations compute references and check index bounds. On a failed bounds
check, the portable interpreter returns `ABC_RESULT_ERROR` and the Arduboy
runtime reports `ERR_IDX`.

| Opcode | Mnemonic | Immediate | Stack | Behavior |
|---:|---|---|---|---|
| `0x4b` | `AIXB1` | `u8 count` | `ref16 i8 -> ref16` | RAM array index with element size 1 and 8-bit count. |
| `0x4c` | `AIDXB` | `u8 elem_size, u8 count` | `ref16 i8 -> ref16` | RAM array index with 8-bit element size and count. |
| `0x4d` | `AIDX` | `u16 elem_size, u16 count` | `ref16 i16 -> ref16` | RAM array index with 16-bit element size and count. |
| `0x4e` | `PIDXB` | `u8 elem_size, u8 count` | `pref24 i8 -> pref24` | Program array index with 8-bit index/count. |
| `0x4f` | `PIDX` | `u16 elem_size, u24 count` | `pref24 i24 -> pref24` | Program array index with 24-bit index/count. |
| `0x50` | `UAIDX` | `u16 elem_size` | `ref16 len16 i16 -> ref16` | RAM unsized-array index. |
| `0x51` | `UPIDX` | `u16 elem_size` | `pref24 len24 i24 -> pref24` | Program unsized-array index. |

Opcode slots `0x52` and `0x53` are reserved in the abc-clang-only VM profile.
Older slice opcodes were removed; slice bytecode is no longer part of the
supported ISA.

### Increment And Decrement

| Opcode | Mnemonic | Immediate | Stack | Behavior |
|---:|---|---|---|---|
| `0x56` | `INC` | none | `byte -> byte` | Increment the current top byte in place. |
| `0x57` | `DEC` | none | `byte -> byte` | Decrement the current top byte in place. |
| `0x58` | `LINC` | `u8 off` | unchanged | Increment one local byte at stack-top-relative offset `off`. |
| `0x59..0x5c` | `PINC..PINC4` | none | `ref16 -> old_value` | Post-increment 1-, 2-, 3-, or 4-byte integer through a RAM reference, pushing the old value. |
| `0x5d..0x60` | `PDEC..PDEC4` | none | `ref16 -> old_value` | Post-decrement 1-, 2-, 3-, or 4-byte integer through a RAM reference, pushing the old value. |
| `0x61` | `PINCF` | none | `ref16 -> old_float` | Post-increment 32-bit float by `1.0`. |
| `0x62` | `PDECF` | none | `ref16 -> old_float` | Post-decrement 32-bit float by `1.0`. |

The post-increment/decrement opcodes update memory but leave the original value
on the stack, matching source-language post-operator behavior.

### Integer Arithmetic And Bitwise Operations

All integer arithmetic wraps to the destination width. For binary operators,
`b` is the top operand and `a` is below it.

| Opcode | Mnemonic | Stack | Behavior |
|---:|---|---|---|
| `0x63..0x66` | `ADD`, `ADD2`, `ADD3`, `ADD4` | `a b -> a+b` | Add 1, 2, 3, or 4-byte integers. |
| `0x67..0x6a` | `SUB`, `SUB2`, `SUB3`, `SUB4` | `a b -> a-b` | Subtract 1, 2, 3, or 4-byte integers. |
| `0x6b` | `ADD2B` | `a16 b8 -> u16` | Add a byte to a 16-bit value. |
| `0x6c` | `ADD3B` | `a24 b8 -> u24` | Add a byte to a 24-bit value. |
| `0x6d` | `SUB2B` | `a16 b8 -> u16` | Subtract a byte from a 16-bit value. |
| `0x6e` | `MUL2B` | `a16 b8 -> u16` | Multiply a 16-bit value by a byte. |
| `0x6f..0x72` | `MUL`, `MUL2`, `MUL3`, `MUL4` | `a b -> a*b` | Multiply 1, 2, 3, or 4-byte integers. |
| `0x73` | `UDIV2` | `a16 b16 -> u16` | Unsigned 16-bit division. Division by zero is an error. |
| `0x74` | `UDIV4` | `a32 b32 -> u32` | Unsigned 32-bit division. Division by zero is an error. |
| `0x75` | `DIV2` | `a16 b16 -> i16` | Signed 16-bit division. Division by zero is an error. |
| `0x76` | `DIV4` | `a32 b32 -> i32` | Signed 32-bit division. Division by zero is an error. |
| `0x77` | `UMOD2` | `a16 b16 -> u16` | Unsigned 16-bit remainder. Division by zero is an error. |
| `0x78` | `UMOD4` | `a32 b32 -> u32` | Unsigned 32-bit remainder. Division by zero is an error. |
| `0x79` | `MOD2` | `a16 b16 -> i16` | Signed 16-bit remainder. Division by zero is an error. |
| `0x7a` | `MOD4` | `a32 b32 -> i32` | Signed 32-bit remainder. Division by zero is an error. |
| `0x7b` | `LSL` | `a8 shift8 -> u8` | Logical left shift. Counts greater than or equal to width produce zero. |
| `0x7c` | `LSL2` | `a16 shift8 -> u16` | 16-bit logical left shift. |
| `0x7d` | `LSL4` | `a32 shift8 -> u32` | 32-bit logical left shift. |
| `0x7e` | `LSR` | `a8 shift8 -> u8` | Logical right shift. Counts greater than or equal to width produce zero. |
| `0x7f` | `LSR2` | `a16 shift8 -> u16` | 16-bit logical right shift. |
| `0x80` | `LSR4` | `a32 shift8 -> u32` | 32-bit logical right shift. |
| `0x81` | `ASR` | `a8 shift8 -> i8` | Arithmetic right shift. Large counts produce all sign bits. |
| `0x82` | `ASR2` | `a16 shift8 -> i16` | 16-bit arithmetic right shift. |
| `0x83` | `ASR4` | `a32 shift8 -> i32` | 32-bit arithmetic right shift. |
| `0x84`, `0x85`, `0x86` | `AND`, `AND2`, `AND4` | `a b -> a&b` | Bitwise AND for 1, 2, or 4 bytes. |
| `0x87`, `0x88`, `0x89` | `OR`, `OR2`, `OR4` | `a b -> a\|b` | Bitwise OR for 1, 2, or 4 bytes. |
| `0x8a`, `0x8b`, `0x8c` | `XOR`, `XOR2`, `XOR4` | `a b -> a^b` | Bitwise XOR for 1, 2, or 4 bytes. |
| `0x8d`, `0x8e`, `0x8f` | `COMP`, `COMP2`, `COMP4` | `a -> ~a` | Bitwise complement for 1, 2, or 4 bytes. |

There are no 24-bit division/modulo or 24-bit bitwise opcodes. The compiler
uses widening, narrowing, or specialized sequences when needed.

### Boolean, Comparison, And Float Operations

| Opcode | Mnemonic | Stack | Behavior |
|---:|---|---|---|
| `0x90..0x93` | `BOOL`, `BOOL2`, `BOOL3`, `BOOL4` | `value -> bool8` | Pop a 1-, 2-, 3-, or 4-byte value and push `1` if nonzero, else `0`. |
| `0x94..0x97` | `CULT`, `CULT2`, `CULT3`, `CULT4` | `a b -> bool8` | Unsigned less-than comparison. |
| `0x98..0x9b` | `CSLT`, `CSLT2`, `CSLT3`, `CSLT4` | `a b -> bool8` | Signed less-than comparison. `CSLT3` sign-extends 24-bit values. |
| `0x9c` | `CFEQ` | `a_float b_float -> bool8` | Floating-point equality comparison. |
| `0x9d` | `CFLT` | `a_float b_float -> bool8` | Floating-point less-than comparison. |
| `0x9e` | `NOT` | `bool8 -> bool8` | Logical not: nonzero becomes `0`, zero becomes `1`. |
| `0x9f` | `FADD` | `a b -> float` | 32-bit float addition. |
| `0xa0` | `FSUB` | `a b -> float` | 32-bit float subtraction. |
| `0xa1` | `FMUL` | `a b -> float` | 32-bit float multiplication. |
| `0xa2` | `FDIV` | `a b -> float` | 32-bit float division. |
| `0xa3` | `F2I` | `float -> i32` | Convert float to signed 32-bit integer. |
| `0xa4` | `F2U` | `float -> u32` | Convert float to unsigned 32-bit integer. |
| `0xa5` | `I2F` | `i32 -> float` | Convert signed 32-bit integer to float. |
| `0xa6` | `U2F` | `u32 -> float` | Convert unsigned 32-bit integer to float. |

Float bytes are stored in the platform `float` representation used by the
interpreter. The generic build uses C `float`; the Arduboy build calls avr-libc
math functions.

### Branching, Calls, And Return

Full-width branch/call targets are absolute 24-bit program addresses. Short
branch/call offsets are signed and relative to the `pc` after reading the
immediate.

| Opcode | Mnemonic | Immediate | Stack | Behavior |
|---:|---|---|---|---|
| `0xa7` | `BZ` | `u24 addr` | `cond8 ->` | Branch to `addr` if `cond == 0`. |
| `0xa8` | `BZ1` | `i8 rel` | `cond8 ->` | Short branch if zero. |
| `0xa9` | `BZ2` | `i16 rel` | `cond8 ->` | Medium branch if zero. |
| `0xaa` | `BNZ` | `u24 addr` | `cond8 ->` | Branch to `addr` if `cond != 0`. |
| `0xab` | `BNZ1` | `i8 rel` | `cond8 ->` | Short branch if nonzero. |
| `0xac` | `BNZ2` | `i16 rel` | `cond8 ->` | Medium branch if nonzero. |
| `0xad` | `BZP` | `u24 addr` | `cond8 -> cond8 if taken, otherwise ->` | Branch if zero and preserve the condition byte only on the taken path. |
| `0xae` | `BZP1` | `i8 rel` | same | Short preserving branch if zero. |
| `0xaf` | `BNZP` | `u24 addr` | `cond8 -> cond8 if taken, otherwise ->` | Branch if nonzero and preserve the condition byte only on the taken path. |
| `0xb0` | `BNZP1` | `i8 rel` | same | Short preserving branch if nonzero. |
| `0xb1` | `JMP` | `u24 addr` | unchanged | Set `pc = addr`. |
| `0xb2` | `JMP1` | `i8 rel` | unchanged | Add signed 8-bit offset to `pc`. |
| `0xb3` | `JMP2` | `i16 rel` | unchanged | Add signed 16-bit offset to `pc`. |
| `0xb4` | `IJMP` | none | `pref24 ->` | Pop program address into `pc`. |
| `0xb5` | `CALL` | `u24 addr` | unchanged | Push return `pc`, then set `pc = addr`. |
| `0xb6` | `CALL1` | `i8 rel` | unchanged | Relative call with signed 8-bit offset. |
| `0xb7` | `CALL2` | `i16 rel` | unchanged | Relative call with signed 16-bit offset. |
| `0xb8` | `ICALL` | none | `pref24 ->` | Pop program address and call it. |
| `0xb9` | `RET` | none | unchanged | Pop return address into `pc`. |

The assembler relaxes full `JMP`, `CALL`, `BZ`, `BNZ`, `BZP`, and `BNZP` forms
to shorter relative forms when the target is close enough. It only emits the
two-byte conditional forms for `BZ`/`BNZ`, not for the preserving variants.

### System Calls

| Opcode | Mnemonic | Immediate | Stack | Behavior |
|---:|---|---|---|---|
| `0xba` | `SYS` | `u8 sys_id_times_2` | syscall-specific | Dispatch to a native system function. |

The immediate byte is `sys_id * 2`, not the raw ID. The generic interpreter
shifts it right by one. The Arduboy interpreter uses it directly as a byte offset
into a PROGMEM table of 16-bit function pointers. This is why real sysfunc IDs
must remain below 128.

Real syscalls are:

| ID | Name | Category |
|---:|---|---|
| `0x00` | `display` | Graphics |
| `0x01` | `display_noclear` | Graphics |
| `0x02` | `get_pixel` | Graphics |
| `0x03` | `draw_pixel` | Graphics |
| `0x04` | `draw_hline` | Graphics |
| `0x05` | `draw_vline` | Graphics |
| `0x06` | `draw_line` | Graphics |
| `0x07` | `draw_rect` | Graphics |
| `0x08` | `draw_filled_rect` | Graphics |
| `0x09` | `draw_circle` | Graphics |
| `0x0a` | `draw_filled_circle` | Graphics |
| `0x0b` | `draw_sprite` | Graphics |
| `0x0c` | `draw_sprite_selfmask` | Graphics |
| `0x0d` | `draw_sprite_array` | Graphics |
| `0x0e` | `draw_sprite_array_P` | Graphics |
| `0x0f` | `draw_sprite_array16` | Graphics |
| `0x10` | `draw_sprite_array16_P` | Graphics |
| `0x11` | `draw_tilemap` | Graphics |
| `0x12` | `draw_text` | Graphics |
| `0x13` | `draw_text_P` | Graphics |
| `0x14` | `draw_textf` | Graphics |
| `0x15` | `text_width` | Graphics |
| `0x16` | `text_width_P` | Graphics |
| `0x17` | `wrap_text` | Graphics |
| `0x18` | `set_text_font` | Graphics |
| `0x19` | `set_text_color` | Graphics |
| `0x1a` | `set_frame_rate` | Graphics |
| `0x1b` | `idle` | Utility |
| `0x1c` | `debug_break` | Utility |
| `0x1d` | `debug_printf` | Utility |
| `0x1e` | `assert` | Utility |
| `0x1f` | `buttons` | Buttons |
| `0x20` | `just_pressed` | Buttons |
| `0x21` | `just_released` | Buttons |
| `0x22` | `pressed` | Buttons |
| `0x23` | `any_pressed` | Buttons |
| `0x24` | `not_pressed` | Buttons |
| `0x25` | `millis` | Utility |
| `0x26` | `memset` | Utility |
| `0x27` | `memcpy` | Utility |
| `0x28` | `memcpy_P` | Utility |
| `0x29` | `strnlen` | Strings |
| `0x2a` | `strnlen_P` | Strings |
| `0x2b` | `strncmp` | Strings |
| `0x2c` | `strncmp_P` | Strings |
| `0x2d` | `strncmp_PP` | Strings |
| `0x2e` | `strncpy` | Strings |
| `0x2f` | `strncpy_P` | Strings |
| `0x30` | `strncat` | Strings |
| `0x31` | `strncat_P` | Strings |
| `0x32` | `format` | Strings |
| `0x33` | `music_play` | Sound |
| `0x34` | `music_playing` | Sound |
| `0x35` | `music_stop` | Sound |
| `0x36` | `tones_play` | Sound |
| `0x37` | `tones_play_primary` | Sound |
| `0x38` | `tones_play_auto` | Sound |
| `0x39` | `tones_playing` | Sound |
| `0x3a` | `tones_stop` | Sound |
| `0x3b` | `audio_enabled` | Sound |
| `0x3c` | `audio_toggle` | Sound |
| `0x3d` | `audio_playing` | Sound |
| `0x3e` | `audio_stop` | Sound |
| `0x3f` | `save_exists` | Save/Load |
| `0x40` | `save` | Save/Load |
| `0x41` | `load` | Save/Load |
| `0x42` | `sin` | Math |
| `0x43` | `cos` | Math |
| `0x44` | `tan` | Math |
| `0x45` | `atan2` | Math |
| `0x46` | `floor` | Math |
| `0x47` | `ceil` | Math |
| `0x48` | `round` | Math |
| `0x49` | `mod` | Math |
| `0x4a` | `pow` | Math |
| `0x4b` | `sqrt` | Math |
| `0x4c` | `log` | Math |
| `0x4d` | `log10` | Math |
| `0x4e` | `generate_random_seed` | Random |
| `0x4f` | `init_random_seed` | Random |
| `0x50` | `set_random_seed` | Random |
| `0x51` | `random` | Random |
| `0x52` | `random_range` | Random |
| `0x53` | `tilemap_get` | Graphics |

`sprites_width`, `sprites_height`, `sprites_frames`, `tilemap_width`, and
`tilemap_height` appear as virtual sysfuncs in compiler metadata. They are not
runtime `SYS` calls; codegen implements them with direct program-data reads.

Detailed source-level syscall behavior is documented in `docs/system.md`. At the
VM level, most syscalls consume their declared argument bytes from the data stack
and push the declared return bytes. Several can also return interpreter results:

| Syscall | Special VM result |
|---|---|
| `idle` | `ABC_RESULT_IDLE` in the generic runtime; Arduboy idles directly. |
| `display`, `display_noclear` | May yield until frame timing catches up. |
| `debug_break` | `ABC_RESULT_BREAK` in the generic runtime; AVR `break` on Arduboy. |
| `assert` | Runtime error if the popped condition byte is zero. |

## Runtime Errors And Constraints

The VM is small and assumes compiler-generated bytecode. It checks the conditions
needed to keep normal programs safe, but malformed hand-written bytecode can
still produce undefined or implementation-specific behavior.

Common runtime errors include:

| Condition | Generic behavior | Arduboy error |
|---|---|---|
| Missing/invalid program signature | Cannot start normally | `ERR_SIG` |
| Array index out of bounds | `ABC_RESULT_ERROR` | `ERR_IDX` |
| Integer division or modulo by zero | `ABC_RESULT_ERROR` | `ERR_DIV` |
| Failed `$assert` | `ABC_RESULT_ERROR` | `ERR_ASS` |
| Data stack overflow | `ABC_RESULT_ERROR` | `ERR_DST` |
| Call stack overflow | `ABC_RESULT_ERROR` | `ERR_CST` |
| Sprite frame outside set | `ABC_RESULT_ERROR` where checked | `ERR_FRM` |
| `memcpy` UAR size mismatch | `ABC_RESULT_ERROR` | `ERR_CPY` |
| Text operation with no font set | `ABC_RESULT_ERROR` where checked | `ERR_FNT` |

Important limits:

| Limit | Value |
|---|---:|
| Data stack storage | 256 bytes, 8-bit offsets |
| Compiler local/frame offset | `< 256` bytes |
| Global RAM | 1024 bytes |
| Compiled globals in 2-shade mode | 1024 bytes |
| Compiled globals in 3/4-shade mode | 256 bytes |
| Saved data in 2-shade mode | 1024 bytes |
| Saved data in 3/4-shade mode | 256 bytes |
| Generic call stack | 24 calls |
| Arduboy call stack | 16 calls |
| Program address width | 24 bits |
| Real sysfunc IDs | `< 128` |

The assembler also enforces global RAM limits and relaxes jumps. The compiler
keeps stack-frame offsets in range, lowers pseudo-instructions, emits `SYS`
immediates as `sys_id * 2`, and arranges all non-system function stack frames.

## Debug Tables

The file table and line table are not executed by the VM, but the Arduboy error
screen and host tooling use them to map program counters back to source.

Line table command bytes are:

| Byte range | Meaning |
|---|---|
| `0..127` | Advance tracked `pc` by `N + 1` bytes. |
| `128..252` | Advance line number by `N - 127`. |
| `253` | Set current file to the next byte. |
| `254` | Set line number to the next two bytes. |
| `255` | Set tracked `pc` to the next three bytes. |

The Arduboy runtime walks this table when displaying an error stack trace.
