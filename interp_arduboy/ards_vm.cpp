#include "ards_vm.hpp"
#include "abc_instr.hpp"

#define ARDS_TONES_IMPLEMENTATION
#include "ards_tone.hpp"

#include "SpritesABC.hpp"

#ifndef EEPROM_h
#define EEPROM_h
#endif
#include <Arduboy2.h>
#include <Arduboy2Audio.h>

extern uint8_t draw_char(uint8_t x, uint8_t y, char c);
extern uint8_t draw_text(uint8_t x, uint8_t y, char const* t, bool prog);

using sys_func_t = void(*)();
extern sys_func_t const SYS_FUNCS[] PROGMEM;

extern volatile unsigned long timer0_millis;
extern volatile unsigned long timer0_overflow_count;

namespace ards
{

vm_t vm;

static char const ERRC_SIG[] PROGMEM = "Game data not found";
static char const ERRC_IDX[] PROGMEM = "Array index out of bounds";
static char const ERRC_DIV[] PROGMEM = "Division by zero";
static char const ERRC_ASS[] PROGMEM = "Assertion failed";
static char const ERRC_DST[] PROGMEM = "Data stack overflow";
static char const ERRC_CST[] PROGMEM = "Call stack overflow";
static char const ERRC_FRM[] PROGMEM = "Sprite frame outside of set";
static char const ERRC_CPY[] PROGMEM = "Sizes of memcpy dst/src differ";
static char const ERRC_FNT[] PROGMEM = "No font set for text operation";
static char const* const ERRC[NUM_ERR] PROGMEM =
{
    ERRC_SIG,
    ERRC_IDX,
    ERRC_DIV,
    ERRC_ASS,
    ERRC_DST,
    ERRC_CST,
    ERRC_FRM,
    ERRC_CPY,
    ERRC_FNT,
};

static void draw_pc_line(uint24_t pc, uint8_t y)
{
    // find and display file/line info
    uint8_t num_files = 0;
    uint24_t file_table = 0;
    uint24_t line_table = 0;
    FX::seekData(12);
    num_files = FX::readPendingUInt8();
    file_table |= ((uint24_t)FX::readPendingUInt8()     << 0);
    file_table |= ((uint24_t)FX::readPendingUInt8()     << 8);
    file_table |= ((uint24_t)FX::readPendingUInt8()     << 16);
    line_table |= ((uint24_t)FX::readPendingUInt8()     << 0);
    line_table |= ((uint24_t)FX::readPendingUInt8()     << 8);
    line_table |= ((uint24_t)FX::readPendingLastUInt8() << 16);
    uint8_t file = 0;
    uint16_t line = 0;
    uint24_t tpc = 0;
    FX::seekData(line_table);
    /*
    Line Table Command Encoding
    =========================================================
        0-127     Advance pc by N+1 bytes
        128-252   Advance line counter by N-127 lines
        253       Set file to next byte
        254       Set line counter to next two bytes
        255       Set pc to next three bytes
    */
    while(tpc < pc)
    {
        uint8_t t = FX::readPendingUInt8();
        if(t < 128)
            tpc += t + 1;
        else if(t < 253)
            line += (t - 127);
        else if(t == 253)
            file = FX::readPendingUInt8();
        else if(t == 254)
        {
            line = FX::readPendingUInt8();
            line |= ((uint16_t)FX::readPendingUInt8() << 8);
        }
        else if(t == 255)
        {
            tpc = FX::readPendingUInt8();
            tpc |= ((uint16_t)FX::readPendingUInt8() << 8);
            tpc |= ((uint16_t)FX::readPendingUInt8() << 16);
        }
    }
    (void)FX::readEnd();
    if(file < num_files)
    {
        char fname[32];
        FX::readDataBytes(file_table + file * 32, (uint8_t*)fname, 32);
        draw_text(24, y, fname, false);
        for(uint8_t i = 0; i < 5; ++i)
        {
            fname[4 - i] = char('0' + line % 10);
            line /= 10;
        }
        fname[5] = '\0';
        uint8_t i = 0;
        while(fname[i] == '0')
            ++i;
        draw_text(0, y, &fname[i], false);
    }
}

extern "C" __attribute__((used)) void vm_error(error_t e)
{
    vm.error = (uint8_t)e;
    Arduboy2Base::clear();
    FX::disable();

    ards::Tones::stop();

    uint8_t n = e > ERR_SIG ? vm.csp / 3 + 1 : 0;
    uint8_t curr = 0;
    bool redraw = true;
    constexpr uint8_t NUM_ROWS = 8;

    for(;;)
    {
        Arduboy2Base::pollButtons();

        if(curr > 0 && Arduboy2Base::justPressed(UP_BUTTON))
            --curr, redraw = true;
        if(uint8_t(curr + NUM_ROWS + 1) < n && Arduboy2Base::justPressed(DOWN_BUTTON))
            ++curr, redraw = true;

        if(redraw)
        {
            static char const ERROR[] PROGMEM = "ERROR";
            draw_text(54, 1, ERROR, true);
            for(uint8_t i = 0; i < 128; ++i)
                Arduboy2Base::sBuffer[i] ^= 0x7f;
            {
                char const* t = (char const*)pgm_read_ptr(&ERRC[e - 1]);
                draw_text(0, 8, t, true);
            }
            for(uint8_t i = 0; i < NUM_ROWS; ++i)
            {
                uint8_t j = curr + i;
                if(j >= n) break;
                draw_pc_line(j == 0 ? vm.pc : vm.calls[n - j - 1], i * 6 + 17);
            }
            FX::display(CLEAR_BUFFER);
        }

        redraw = false;
        uint8_t m = (uint8_t)millis() + 20;
        while(int8_t(m - (uint8_t)millis()) >= 0)
            Arduboy2Base::idle();
    }
}


// main assembly method containing optimized implementations for each instruction
// the alignment of 512 allows optimized dispatch: the prog address for ijmp can
// be computed by adding to the high byte only instead of adding a 2-byte offset

// HACK: place at .progmem directly to help avoid waste due to padding
static void __attribute__((used, naked, section(".progmem"))) vm_execute_func()
{
    asm volatile(
R"(
    
        .align 7
    
vm_execute:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Registers
; 
;     r0-r1        - scratch regs
;     r2           - constant value 0
;     r3           - constant value 32 (used for dispatch)
;     r4           - constant value 1
;     r5           - constant value hi8(vm_execute) TODO :/
;     r6-r8        - pc
;     r9           - reserved for TOS (future optimization)
;     r10-r27      - scratch regs
;     r28:29       - &vm.stack[sp] (sp is r28)
;     r30-r31      - scratch regs
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; helper macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    .macro dispatch_delay
    lpm
    lpm
    nop
    .endm
    
    .macro fx_disable
    sbi  %[fxport], %[fxbit]
    .endm
    
    .macro fx_enable
    cbi  %[fxport], %[fxbit]
    .endm
    
    .macro read_end
    in   r0, %[spsr]
    sbrs r0, 7
    rjmp .-6
    sbi  %[fxport], %[fxbit]
    .endm
    
    .macro read_byte
    in   r0, %[spdr]
    out  %[spdr], r2
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    .endm
    
    ; needs 16 cycles since the last write to SPDR
    .macro dispatch_noalign
    read_byte
    mul  r0, r3
    movw r30, r0
    add  r31, r5
    ijmp
    .endm
    
    ; need to copy macro here because max nesting is 2 apparently
    ; needs 16 cycles since the last write to SPDR
    .macro dispatch
    read_byte
    mul  r0, r3
    movw r30, r0
    add  r31, r5
    ijmp
    .align 6
    .endm

    ; this macro can speed up dispatch by one cycle
    ; needs 13 cycles since the last write to SPDR (3 fewer)
    .macro dispatch_noalign_reverse
    add  r6, r4
    adc  r7, r2
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    sei
    adc  r8, r2
    mul  r0, r3
    movw r30, r0
    add  r31, r5
    ijmp
    .endm

    .macro dispatch_reverse
    dispatch_noalign_reverse
    .align 6
    .endm

    .macro dispatch_noalign_reverse_noinc
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    sei
    nop
    mul  r0, r3
    movw r30, r0
    add  r31, r5
    ijmp
    .endm

    .macro dispatch_reverse_noinc
    dispatch_noalign_reverse_noinc
    .align 6
    .endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; instructions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

I_NOP:
    rjmp .+0
    rjmp .+0
    dispatch_noalign_reverse

    .align 5
    ; put in a tag here: 32 bytes after start of instructions
    ; this helps tools find where the instructions are for
    ; things like breakpoints/profiling

    ;        0123456789abcdef0123456789abcdef
    .ascii " ABC Interpreter by Peter Brown "

    .align 6

I_PUSH:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    add  r6, r4
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r9, %[spdr]
    out  %[sreg], r10
    adc  r7, r2
    adc  r8, r2
    lpm
    lpm
    lpm
    dispatch_noalign_reverse
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_P0:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 0
    mov  r9, r16
    dispatch

I_P1:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 1
    mov  r9, r16
    dispatch

I_P2:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 2
    mov  r9, r16
    dispatch

I_P3:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 3
    mov  r9, r16
    dispatch

I_P4:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 4
    mov  r9, r16
    dispatch

I_P5:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 5
    mov  r9, r16
    dispatch

I_P6:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 6
    mov  r9, r16
    dispatch

I_P7:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 7
    mov  r9, r16
    dispatch

I_P8:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 8
    mov  r9, r16
    dispatch

I_P16:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 16
    mov  r9, r16
    dispatch

I_P32:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 32
    mov  r9, r16
    dispatch

I_P64:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 64
    mov  r9, r16
    dispatch

I_P128:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  ldi  r16, 128
    mov  r9, r16
    dispatch_noalign
push2_dispatch:
push3_dispatch:
push4_dispatch:
    dispatch_reverse

I_P00:
    cpi  r28, 254
    brsh 1f
    st   Y+, r9
    st   Y+, r2
    clr  r9
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_P000:
    cpi  r28, 253
    brsh 1f
    st   Y+, r9
    st   Y+, r2
    st   Y+, r2
    clr  r9
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_P0000:
    cpi  r28, 252
    brsh 1f
    st   Y+, r9
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    clr  r9
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_PZ8:
    cpi  r28, 248
    brsh 1f
    st   Y+, r9
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    clr  r9
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_PZ16:
    cpi  r28, 248
    brsh 1f
    st   Y+, r9
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    clr  r9
    dispatch_noalign
push2_error:
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_PUSH2:
    st   Y+, r9
    cpi  r28, 254
    brsh push2_error
    ldi  r16, 2
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    st   Y+, r0
    rcall pushg_delay_13
    out  %[spdr], r2
    in   r9, %[spdr]
    out  %[sreg], r10
    add  r6, r16
    adc  r7, r2
    adc  r8, r2
    lpm
    lpm
    rjmp push2_dispatch
pushg_delay_13:
    nop
pushg_delay_12:
    nop
pushg_delay_11:
    nop
pushg_delay_10:
    nop
pushg_delay_9:
    rjmp .+0
pushg_delay_7:
    ret
    .align 6

I_PUSH3:
    st   Y+, r9
    cpi  r28, 253
    brsh push3_error
    ldi  r16, 3
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    st   Y+, r0
    rcall pushg_delay_13
    out  %[spdr], r2
    in   r0, %[spdr]
    st   Y+, r0
    rcall pushg_delay_13
    out  %[spdr], r2
    in   r9, %[spdr]
    out  %[sreg], r10
    add  r6, r16
    adc  r7, r2
    adc  r8, r2
    lpm
    lpm
    rjmp push3_dispatch
push3_error:
push4_error:
    ldi  r24, 5
    call call_vm_error
    .align 6

I_PUSH4:
    st   Y+, r9
    cpi  r28, 252
    brsh push4_error
    ldi  r16, 4
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    st   Y+, r0
    add  r6, r16
    adc  r7, r2
    adc  r8, r2
    rcall pushg_delay_10
    out  %[spdr], r2
    in   r0, %[spdr]
    st   Y+, r0
    rcall pushg_delay_13
    out  %[spdr], r2
    in   r0, %[spdr]
    st   Y+, r0
    rcall pushg_delay_13
    out  %[spdr], r2
    in   r9, %[spdr]
    out  %[sreg], r10
    rcall pushg_delay_9
    rjmp push4_dispatch
    .align 6

I_SEXT:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    lsl  r9
    sbc  r9, r9
    nop
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_SEXT2:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    lsl  r9
    sbc  r9, r9
    st   Y+, r9
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_SEXT3:
    st   Y+, r9
    cpi  r28, 253
    brsh 1f
    lsl  r9
    sbc  r9, r9
    st   Y+, r9
    st   Y+, r9
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    dispatch_noalign_reverse
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP2:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 2
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP3:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 3
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP4:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 4
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP5:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 5
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP6:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 6
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP7:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 7
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUP8:
    st   Y+, r9
    cpi  r28, 255
    breq 1f
    movw r26, r28
    subi r26, 8
    ld   r9, X
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 2
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW2:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 3
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW3:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 4
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW4:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 5
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW5:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 6
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW6:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 7
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW7:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 8
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_DUPW8:
    st   Y+, r9
    cpi  r28, 254
    brsh 1f
    movw r26, r28
    subi r26, 9
    ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    dispatch_noalign
1:  ldi  r24, 5
    call call_vm_error
    .align 6

I_GETL:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    movw r26, r28
    sub  r26, r0
    ld   r9, X
    rjmp .+0
    rjmp .+0
    dispatch_reverse
    
I_GETL2:
    st   Y+, r9
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    movw r26, r28
    sub  r26, r0
    ld   r0, X+
    ld   r9, X
    st   Y+, r0
    dispatch_reverse
    
I_GETL4:
    st   Y+, r9
    cpi  r28, 252
    brlo 1f
getln_error:
    ldi  r24, 5
    call call_vm_error
1:  add  r6, r4
    movw r26, r28
    in   r0, %[spdr]
    out  %[spdr], r2
    adc  r7, r2
    adc  r8, r2
    sub  r26, r0
    ld   r16, X+
    ld   r17, X+
    ld   r18, X+
    ld   r9, X+
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    dispatch

I_GETLN:
    mov  r16, r9
    add  r9, r28
    brcs getln_error
1:  movw r26, r28
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    in   r0, %[spdr]
    out  %[spdr], r2
    sub  r26, r0
    lsr  r16
    brcc 2f
    ld   r0, X+
    st   Y+, r0
2:  ld   r0, X+
    st   Y+, r0
    ld   r9, X+
    st   Y+, r9
    dec  r16
    brne 2b
    dec  r28
    dispatch

I_SETL:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    mov  r16, r9
    movw r26, r28
    sub  r26, r0
    st   X, r16
    ld   r9, -Y
    nop
getg_dispatch:
getg2_dispatch:
    dispatch_reverse

I_SETL2:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    mov  r17, r9
    ld   r16, -Y
    movw r26, r28
    sub  r26, r0
    st   X+, r16
    st   X+, r17
    ld   r9, -Y
getg4_dispatch:
    dispatch

I_SETL4:
    mov  r19, r9
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    read_byte
    movw r26, r28
    sub  r26, r0
    st   X+, r16
    st   X+, r17
    st   X+, r18
    st   X+, r19
    ld   r9, -Y
    nop
setln_dispatch:
    dispatch
 
I_SETLN:
    mov  r16, r9
    subi r16, 2
    movw r26, r28
    ld   r17, -Y
    ld   r18, -Y
    read_byte
    sub  r26, r0
    st   -X, r17
    st   -X, r18
    lsr  r16
    brcc 1f
    ld   r0, -Y
    st   -X, r0
    breq 2f
1:  ld   r0, -Y
    st   -X, r0
    ld   r0, -Y
    st   -X, r0
    dec  r16
    brne 1b
2:  ld   r9, -Y
    rjmp setln_dispatch
    .align 6

I_GETG:
    st   Y+, r9
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    call call_vm_error
1:  in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    ldi  r20, 2
    add  r6, r20
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_11
    out  %[spdr], r2 
    in   r27, %[spdr]
    out  %[sreg], r10
    ld   r9, X
    rcall getg_delay_7
    rjmp getg_dispatch
getg_delay_17:
    nop
getg_delay_16:
    nop
getg_delay_15:
    nop
getg_delay_14:
    nop
getg_delay_13:
    nop
getg_delay_12:
    nop
getg_delay_11:
    nop
getg_delay_10:
    rjmp .+0
getg_delay_8:
    nop
getg_delay_7:
    ret
    .align 6

I_GETG2:
    st   Y+, r9
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  nop
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    ldi  r20, 2
    add  r6, r20
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_11
    out  %[spdr], r2 
    in   r27, %[spdr]
    sei
    ld   r16, X+
    ld   r9, X+
    st   Y+, r16
    lpm
    rjmp getg2_dispatch
    .align 6

I_GETG4:
    st   Y+, r9
    cpi  r28, 252
    brlo 1f
getgn_error:
    ldi  r24, 5
    call call_vm_error
1:  rjmp .+0
    in   r26, %[spdr]
    out  %[spdr], r2
    ldi  r20, 2
    add  r6, r20
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_12
    in   r27, %[spdr]
    out  %[spdr], r2 
    ld   r16, X+
    ld   r17, X+
    ld   r18, X+
    ld   r9, X+
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    rjmp getg4_dispatch
    .align 6

I_GETGN:
    mov  r18, r9
    dec r9
    add  r9, r28
    brcs getgn_error
    lpm
    in   r26, %[spdr]
    out  %[spdr], r2
    ldi  r20, 2
    add  r6, r20
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_12
    in   r27, %[spdr]
    out  %[spdr], r2
    lsr  r18
    brcc 1f
    ld   r0, X+
    st   Y+, r0
1:  ld   r0, X+
    st   Y+, r0
    ld   r0, X+
    st   Y+, r0
    dec  r18
    brne 1b
    ld   r9, -Y
    rjmp getg4_dispatch
    .align 6

I_GTGB:
    cpi  r28, 255
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  st   Y+, r9
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    out  %[sreg], r10
    ldi  r27, 2
    ld   r9, X
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    lpm
    rjmp .+0
    dispatch_noalign_reverse
gtgb_delay_11:
    lpm
gtgb_delay_8:
    nop
    ret
    .align 6

I_GTGB2:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  st   Y+, r9
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    out  %[sreg], r10
    ldi  r27, 2
    ld   r9, X+
    st   Y+, r9
    ld   r9, X
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    nop
    dispatch_noalign_reverse
gtgb2_delay_7:
    ret
    .align 6

I_GTGB4:
    cpi  r28, 252
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  st   Y+, r9
    ldi  r27, 2
    add  r6, r4
    in   r26, %[spdr]
    out  %[spdr], r2
    adc  r7, r2
    adc  r8, r2
    ld   r9, X+
    st   Y+, r9
    ld   r9, X+
    st   Y+, r9
    ld   r9, X+
    st   Y+, r9
    ld   r9, X
    dispatch

I_SETG:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_11
    out  %[spdr], r2
    in   r27, %[spdr]
    out  %[sreg], r10
    st   X, r9
    ld   r9, -Y
    rcall getg_delay_7
    dispatch_reverse

I_SETG2:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_11
    out  %[spdr], r2
    in   r27, %[spdr]
    out  %[sreg], r10
    ld   r17, -Y
    st   X+, r17
    st   X+, r9
    ld   r9, -Y
    lpm
    dispatch_reverse

I_SETG4:
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    mov  r19, r9
    ldi  r20, 2
    add  r6, r20
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_10
    out  %[spdr], r2
    in   r27, %[spdr]
    sei
    st   X+, r16
    st   X+, r17
    st   X+, r18
    st   X+, r19
    ld   r9, -Y
    nop
    dispatch_reverse

I_SETGN:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r26, %[spdr]
    out  %[sreg], r10
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    rcall getg_delay_10
    in   r27, %[spdr]
    out  %[spdr], r2
    add  r26, r9
    adc  r27, r2
    lsr  r9
    brcc 1f
    ld   r0, -Y
    st   -X, r0
1:  ld   r0, -Y
    st   -X, r0
    ld   r0, -Y
    st   -X, r0
    dec  r9
    brne 1b
    ld   r9, -Y
    rjmp setgn_dispatch
    .align 6

I_GETP:
    mov  r18, r9
    ld   r17, -Y
    ld   r16, -Y
    fx_disable
    ldi  r19, 3
    fx_enable
    out  %[spdr], r19
    lds  r10, %[data_page]+0
    lds  r11, %[data_page]+1
    add  r10, r17
    adc  r11, r18
    rcall getg_delay_11
    out  %[spdr], r11
    rcall getg_delay_17
    out  %[spdr], r10
    rcall getg_delay_17
    out  %[spdr], r16
    rcall getg_delay_16
    out  %[spdr], r2
    rcall getg_delay_16
    in   r9, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r19
    jmp  jump_to_pc_delayed
getpn_delay_10:
    nop
getpn_delay_9:
    rjmp .+0
getpn_delay_7:
    ret
    .align 6

I_GETPN:
    mov  r18, r9
    ld   r17, -Y
    ld   r16, -Y
    ldi  r19, 3
    nop
    in   r1, %[spdr]
    rjmp getpn_seek_to_addr
getpn_resume:
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    lpm
1:  lpm
    lpm
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    st   Y+, r0
    dec  r1
    cp   r1, r4
    brne 1b
    rcall getpn_delay_9
    in   r9, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r19
    jmp  jump_to_pc_delayed
    .align 6

I_GETR:
    mov  r27, r9
    ld   r26, -Y
    ld   r9, X+
    rjmp .+0
setgn_dispatch:
    dispatch_noalign
getpn_seek_to_addr:
    fx_disable
    ldi  r19, 3
    fx_enable
    out  %[spdr], r19
    lds  r10, %[data_page]+0
    lds  r11, %[data_page]+1
    add  r10, r17
    adc  r11, r18
    rcall getg_delay_11
    out  %[spdr], r11
    rcall getg_delay_17
    out  %[spdr], r10
    rcall getg_delay_17
    out  %[spdr], r16
    rcall getg_delay_16
    out  %[spdr], r2
    rjmp getpn_resume
    .align 6

I_GETR2:
    mov  r27, r9
    ld   r26, -Y
    ld   r1, X+
    st   Y+, r1
    ld   r9, X+
    dispatch

I_GETRN:
    mov  r27, r9
    ld   r26, -Y
    mov  r18, r28
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    in   r0, %[spdr]
    out  %[spdr], r2
    add  r18, r0
    brcs getrn_error
    lsr  r0
    brcc 1f
    ld   r9, X+
    st   Y+, r9
1:  ld   r9, X+
    st   Y+, r9
    ld   r9, X+
    st   Y+, r9
    dec  r0
    brne 1b
    dec  r28
    dispatch

I_SETR:
    mov  r27, r9
    ld   r26, -Y
    ld   r1, -Y
    st   X, r1
    ld   r9, -Y
    dispatch_noalign
getrn_error:
    ldi  r24, 5
    call call_vm_error
    .align 6
    ; TODO: SPACE HERE

I_SETR2:
    mov  r27, r9
    ld   r26, -Y
    ld   r1, -Y
    ld   r0, -Y
    ld   r9, -Y
    st   X+, r0
    st   X, r1
    dispatch

I_SETRN:
    mov  r27, r9
    ld   r26, -Y
    nop
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    in   r0, %[spdr]
    out  %[spdr], r2
    add  r26, r0
    adc  r27, r2
    lsr  r0
    brcc 1f
    ld   r1, -Y
    st   -X, r1
1:  ld   r1, -Y
    st   -X, r1
    ld   r1, -Y
    st   -X, r1
    dec  r0
    brne 1b
    ld   r9, -Y
    dispatch

I_POP:
    dec  r28
    ld   r9, Y
    nop
    dispatch_reverse
    ; TODO: SPACE HERE

I_POP2:
    subi r28, 2
    ld   r9, Y
    nop
    dispatch_reverse
    ; TODO: SPACE HERE

I_POP3:
    subi r28, 3
    ld   r9, Y
    nop
    dispatch_noalign_reverse
pop3_delay_16:
    nop
pop3_delay_15:
    rjmp .+0
pop3_delay_13:
    nop
pop3_delay_12:
    nop
pop3_delay_11:
    nop
pop3_delay_10:
    nop
pop3_delay_9:
    nop
pop3_delay_8:
    nop
pop3_delay_7:
    ret
    .align 6

I_POP4:
    subi r28, 4
    ld   r9, Y
    nop
    dispatch_noalign_reverse
aidx_part2:
    ld   r20, -Y
    add  r22, r20
    adc  r23, r21
    st   Y+, r22
    mov  r9, r23
aidxb_dispatch:
    dispatch

I_POPN:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    sub  r28, r0
    ld   r9, Y
    lpm
    rjmp .+0
    dispatch_reverse

I_AIXB1:
    mov  r20, r9
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    rjmp .+0
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    ; r0:  num elems
    ; r20: index
    cp   r20, r0
    brsh aidx_error
    ld   r9, -Y
    ld   r22, -Y
    add  r22, r20
    adc  r9, r2
    st   Y+, r22
    nop
    dispatch_noalign_reverse
aidx_error:
    ldi  r24, 2
    call call_vm_error
    .align 6

I_AIDXB:
    mov  r20, r9
    ldi  r17, 2
    add  r6, r17
    lpm
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    adc  r7, r2
    adc  r8, r2
    rcall pop3_delay_13
    out  %[spdr], r2
    in   r17, %[spdr]
    sei

    ; r16: elem size
    ; r17: num elems
    ; r20: index
    cp   r20, r17
    brsh aidx_error
    mul  r16, r20
    ld   r21, -Y
    ld   r20, -Y
    add  r0, r20
    adc  r1, r21
    st   Y+, r0
    mov  r9, r1
    nop
    dispatch

I_AIDX:
    mov  r21, r9
    ld   r20, -Y
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    ldi  r17, 4
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    rcall pop3_delay_11
    out  %[spdr], r2
    in   r17, %[spdr]
    rcall pop3_delay_15
    out  %[spdr], r2
    in   r18, %[spdr]
    rcall pop3_delay_15
    out  %[spdr], r2
    in   r19, %[spdr]
    out  %[sreg], r10

    cp   r20, r18
    cpc  r21, r19
    brsh aidx_error
    ; A1 A0 : r21 r20
    ; B1 B0 : r17 r16
    ; C1 C0 : r23 r22
    ;
    ;    A1 A0
    ;    B1 B0
    ;    =====
    ;    A0*B0
    ; A1*B0
    ; A0*B1
    ; ========
    ;    C1 C0
    ;
    mul  r16, r20
    movw r22, r0
    mul  r16, r21
    add  r23, r0
    mul  r17, r20
    add  r23, r0
    ld   r21, -Y
    rjmp aidx_part2
    .align 6

I_PIDXB:
    ; load index into r10
    mov  r10, r9
    ; load progref into r13:r14:r9
    ld   r9, -Y
    ld   r14, -Y
    in   r0, %[sreg]
    cli
    ; load elem size into r16
    ; load elem count into r17
    out  %[spdr], r2
    in   r16, %[spdr]
    ld   r13, -Y
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    rcall pop3_delay_9
    out  %[spdr], r2
    in   r17, %[spdr]
    out  %[sreg], r0
    ; bounds check index against elem count
    cp   r10, r17
    brsh pidxb_error
    ; compute prog ref + index * elem_size
1:  mul  r10, r16 ; index * elem size
    add  r13, r0
    adc  r14, r1
    adc  r9, r2
    ; push prog ref
    st   Y+, r13
    st   Y+, r14
    nop
    rjmp pidxb_dispatch
pidxb_error:
pidx_error:
    ldi  r24, 2
    jmp call_vm_error
    .align 6

I_PIDX:

    rcall pop3_delay_7

    ; load elem size into r20:r21
    in   r20, %[spdr]
    out  %[spdr], r2
    ldi  r17, 5
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    
    ; load index into r10:r12
    mov  r12, r9
    ld   r11, -Y
    ld   r10, -Y
    rcall pop3_delay_7

    in   r21, %[spdr]
    out  %[spdr], r2
    
    ; load progref into r13:r15
    ld   r15, -Y
    ld   r14, -Y
    ld   r13, -Y
    rcall pop3_delay_10

    ; load elem count into r16:r18
    in   r16, %[spdr]
    out  %[spdr], r2
    rcall pop3_delay_16
    in   r17, %[spdr]
    out  %[spdr], r2
    rcall pop3_delay_16
    in   r18, %[spdr]
    out  %[spdr], r2

    ; bounds check index against elem count
    cp   r10, r16
    cpc  r11, r17
    cpc  r12, r18
    brsh pidx_error

    jmp pidx_part2
    .align 6

I_UAIDX:
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    sei
    mov  r21, r9
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    cp   r20, r18
    cpc  r21, r19
    brsh pidxb_error
    ; A1 A0 : r21 r20
    ; B1 B0 : r17 r16
    ; C1 C0 : r23 r22
    ;
    ;    A1 A0
    ;    B1 B0
    ;    =====
    ;    A0*B0
    ; A1*B0
    ; A0*B1
    ; ========
    ;    C1 C0
    ;
1:  mul  r16, r20
    ld   r25, -Y
    ld   r24, -Y
    out  %[spdr], r2
    in   r17, %[spdr]
    movw r22, r0
    add  r22, r24
    adc  r23, r25
    mul  r16, r21
    add  r23, r0
    mul  r17, r20
    add  r23, r0
    st   Y+, r22
    mov  r9, r23
    nop
    rjmp uaidx_dispatch
    .align 6

I_UPIDX:
    jmp upidx_impl
aslc_error:
    ldi  r24, 2
    jmp call_vm_error
pslc_part2:
    add  r11, r0
    adc  r12, r1
    mul  r18, r24
    add  r12, r0
    mul  r17, r25
    add  r12, r0

    ; compute length = stop - start and test for negative length
    sub  r19, r16
    sbc  r20, r17
    sbc  r9, r18
    brlt aslc_error

    ldi  r27, 2
    add  r6, r27
    adc  r7, r2
    adc  r8, r2

    ; push addr and length
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    st   Y+, r19
    st   Y+, r20

    rjmp slc_dispatch
    
aslc_part2:
    ; push addr and length
    st   Y+, r16
    st   Y+, r17
    st   Y+, r22
    rjmp slc_dispatch
    .align 6

I_ASLC:

    ; addr:   r16:r17
    ; length: r18:r19
    ; start:  r20:r21
    ; stop:   r22:r9
    ; imm:    r24:r25

    ld   r22, -Y
    ld   r21, -Y
    ld   r20, -Y
    ld   r19, -Y
    in   r24, %[spdr]
    out  %[spdr], r2
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y

    ; test start: error if start >= length
    cp   r20, r18
    cpc  r21, r19
    brsh aslc_error
    ; test stop: error if length < stop
    cp   r18, r22
    cpc  r19, r9
    brlo aslc_error

    ; addr += start*imm
    mul  r20, r24
    add  r16, r0
    adc  r17, r1
    in   r25, %[spdr]
    out  %[spdr], r2
    mul  r21, r24
    add  r17, r1
    mul  r20, r25
    add  r17, r1

    ; compute length = stop - start and test for negative length
    sub  r22, r20
    sbc  r9, r21
    brlt aslc_error

    ldi  r27, 2
    add  r6, r27
    adc  r7, r2
    adc  r8, r2

    rjmp aslc_part2
    .align 6

I_PSLC:

    ; addr:   r10:r11:r12
    ; length: r13:r14:r15
    ; start:  r16:r17:r18
    ; stop:   r19:r20:r9
    ; imm:    r24:r25

    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    in   r24, %[spdr]
    out  %[spdr], r2
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y

    ; test start: error if start >= length
    cp   r16, r13
    cpc  r17, r14
    cpc  r18, r15
    brsh pslc_error

    in   r25, %[spdr]
    out  %[spdr], r2

    ; test stop: error if length < stop
    cp   r13, r19
    cpc  r14, r20
    cpc  r15, r9
    brlo pslc_error

    ; addr += start*imm
    ;    A2 A1 A0 start
    ;       B1 B0 imm
    ;    ========
    ;       A0*B0
    ;    A1*B0
    ;    A0*B1
    ; A2*B0
    ; A1*B1
    ; ===========
    ;    C2 C1 C0
    mul  r16, r24
    add  r10, r0
    adc  r11, r1
    adc  r12, r2
    mul  r17, r24
    add  r11, r0
    adc  r12, r1
    mul  r16, r25
    rjmp pslc_part2
    .align 6

I_REFL:
    st   Y+, r9
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    movw r16, r28
    sub  r16, r0
    st   Y+, r16
    mov  r9, r17
    rjmp .+0
    rjmp .+0
    rjmp refl_dispatch
pslc_error:
    ldi  r24, 2
    jmp call_vm_error
refl_delay_11:
    lpm
refl_delay_8:
    nop
    ret
    .align 6

I_REFGB:
    st   Y+, r9
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    ldi  r17, 2
    st   Y+, r0
    mov  r9, r17
    rjmp .+0
    rjmp refgb_dispatch
    .align 6

I_INC:
    inc  r9
    lpm
dec_dispatch:
refgb_dispatch:
    dispatch_reverse

I_DEC:
    dec r9
    nop
    rjmp dec_dispatch
    ; TODO: SPACE HERE
    .align 6

I_LINC:
    rjmp .+0
    movw r26, r28
    adiw r26, 1
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    out  %[sreg], r10
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    sub  r26, r0
    ld   r0, X
    inc  r0
    st   X, r0
    rjmp .+0
    dispatch_reverse

I_PINC:
    mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    mov  r9, r16
    inc  r16
    st   -X, r16
slc_dispatch:
pidxb_dispatch:
refl_dispatch:
uaidx_dispatch:
    dispatch
    ; TODO: SPACE HERE

I_PINC2:
    mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    ld   r17, X+
    st   Y+, r16
    mov  r9, r17
    add  r16, r4
    adc  r17, r2
    st   -X, r17
    st   -X, r16
    dispatch

I_PINC3:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    ld   r17, X+
    ld   r18, X+
    st   Y+, r16
    st   Y+, r17
    mov  r9, r18
    add  r16, r4
    adc  r17, r2
    adc  r18, r2
    st   -X, r18
    st   -X, r17
    st   -X, r16
    dispatch

I_PINC4:
    cpi  r28, 253
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    ld   r17, X+
    ld   r18, X+
    ld   r19, X+
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    mov  r9, r19
    add  r16, r4
    adc  r17, r2
    adc  r18, r2
    adc  r19, r2
    st   -X, r19
    st   -X, r18
    st   -X, r17
    st   -X, r16
    dispatch

I_PDEC:
    mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    mov  r9, r16
    dec  r16
    st   -X, r16
    dispatch
    ; TODO: SPACE HERE

I_PDEC2:
    mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    ld   r17, X+
    st   Y+, r16
    mov  r9, r17
    sub  r16, r4
    sbc  r17, r2
    st   -X, r17
    st   -X, r16
    dispatch

I_PDEC3:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    ld   r17, X+
    ld   r18, X+
    st   Y+, r16
    st   Y+, r17
    mov  r9, r18
    sub  r16, r4
    sbc  r17, r2
    sbc  r18, r2
    st   -X, r18
    st   -X, r17
    st   -X, r16
    dispatch

I_PDEC4:
    cpi  r28, 253
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  mov  r27, r9
    ld   r26, -Y
    ld   r16, X+
    ld   r17, X+
    ld   r18, X+
    ld   r19, X+
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    mov  r9, r19
    sub  r16, r4
    sbc  r17, r2
    sbc  r18, r2
    sbc  r19, r2
    st   -X, r19
    st   -X, r18
    st   -X, r17
    st   -X, r16
pincf_dispatch:
pdecf_dispatch:
    dispatch

I_PINCF:
    cpi  r28, 253
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  mov  r27, r9
    ld   r26, -Y
    ld   r22, X+
    ld   r23, X+
    ld   r24, X+
    ld   r25, X+
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    movw r16, r26
    clr  r1
    ldi  r18, 0
    ldi  r19, 0
    ldi  r20, 128
    ldi  r21, 63
    call __addsf3
    movw r26, r16
    st   -X, r25
    st   -X, r24
    st   -X, r23
    st   -X, r22
    rjmp pincf_dispatch
    .align 6

I_PDECF:
    cpi  r28, 253
    brlo 1f
    ldi  r24, 5
    call call_vm_error
1:  mov  r27, r9
    ld   r26, -Y
    ld   r22, X+
    ld   r23, X+
    ld   r24, X+
    ld   r25, X+
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    movw r16, r26
    clr  r1
    ldi  r18, 0
    ldi  r19, 0
    ldi  r20, 128
    ldi  r21, 63
    call __subsf3
    movw r26, r16
    st   -X, r25
    st   -X, r24
    st   -X, r23
    st   -X, r22
    rjmp pdecf_dispatch
    .align 6

I_ADD:
    ld   r14, -Y
    add  r9, r14
    nop
    dispatch_reverse
    ; TODO: SPACE HERE

I_ADD2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r10, r14
    adc  r9, r15
    st   Y+, r10
    dispatch

I_ADD3:
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r10, r14
    adc  r11, r15
    adc  r9, r16
    st   Y+, r10
    st   Y+, r11
    dispatch

I_ADD4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r10, r14
    adc  r11, r15
    adc  r12, r16
    adc  r9, r17
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    dispatch

I_SUB:
    ld   r14, -Y
    sub  r14, r9
    mov  r9, r14
    dispatch_reverse
    ; TODO: SPACE HERE

I_SUB2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    sub  r14, r10
    sbc  r15, r9
    st   Y+, r14
    mov  r9, r15
    dispatch

I_SUB3:
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    sub  r14, r10
    sbc  r15, r11
    sbc  r16, r9
    st   Y+, r14
    st   Y+, r15
    mov  r9, r16
    dispatch

I_SUB4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    sub  r14, r10
    sbc  r15, r11
    sbc  r16, r12
    sbc  r17, r9
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    mov  r9, r17
    dispatch

I_ADD2B:
    ld   r15, -Y
    ld   r14, -Y
    add  r14, r9
    adc  r15, r2
    st   Y+, r14
    mov  r9, r15
    dispatch

I_ADD3B:
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r14, r9
    adc  r15, r2
    adc  r16, r2
    st   Y+, r14
    st   Y+, r15
    mov  r9, r16
    dispatch

I_SUB2B:
    ld   r15, -Y
    ld   r14, -Y
    sub  r14, r9
    sbc  r15, r2
    st   Y+, r14
    mov  r9, r15
    dispatch

I_MUL2B:
    ;
    ;    A1 A0
    ;       B0
    ;    =====
    ;    A0*B0
    ; A1*B0
    ; ========
    ;    C1 C0
    ;
    ld   r15, -Y
    ld   r14, -Y
    mul  r14, r9 ; A0*B0
    movw r18, r0
    mul  r15, r9 ; A1*B0
    add  r19, r0
    st   Y+, r18
    mov  r9, r19
    dispatch

I_MUL:
    ld   r14, -Y
    mul  r14, r9
    mov  r9, r0
    rjmp .+0
    dispatch
    ; TODO: SPACE HERE

I_MUL2:
    ;
    ;    A1 A0
    ;    B1 B0
    ;    =====
    ;    A0*B0
    ; A1*B0
    ; A0*B1
    ; ========
    ;    C1 C0
    ;
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    mul  r14, r10 ; A0*B0
    movw r18, r0
    mul  r14, r9 ; A0*B1
    add  r19, r0
    mul  r15, r10 ; A1*B0
    add  r19, r0
    st   Y+, r18
    mov  r9, r19
    dispatch

I_MUL3:
    ;   
    ;    A2 A1 A0
    ;    B2 B1 B0
    ;    ========
    ;       A0*B0
    ;    A1*B0
    ;    A0*B1
    ; A2*B0
    ; A1*B1
    ; A0*B2
    ; ===========
    ;    C2 C1 C0
    ;   
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    mul  r14, r10 ; A0*B0
    movw r18, r0
    mul  r16, r10 ; A2*B0
    mov  r9, r0
    mul  r15, r10 ; A1*B0
    add  r19, r0
    adc  r9, r1
    mul  r14, r11 ; A0*B1
    add  r19, r0
    adc  r9, r1
    mul  r15, r11 ; A1*B1
    add  r9, r0
    mul  r14, r9 ; A0*B2
    add  r9, r0
    st   Y+, r18
    st   Y+, r19
    dispatch

I_MUL4:
    jmp  instr_mul4
    ; TODO: SPACE HERE
    .align 6

I_UDIV2:
    mov  r23, r9
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __udivmodhi4
    st   Y+, r22
    mov  r9, r23
udiv4_dispatch:
    dispatch

I_UDIV4:
    mov   r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    cp   r18, r2
    cpc  r19, r2
    cpc  r20, r2
    cpc  r21, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __udivmodsi4
    movw r28, r16
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    mov  r9, r21
    rjmp udiv4_dispatch
    .align 6

I_DIV2:
    mov  r23, r9
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __divmodhi4
    st   Y+, r22
    mov  r9, r23
div4_dispatch:
    dispatch

I_DIV4:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    cp   r18, r2
    cpc  r19, r2
    cpc  r20, r2
    cpc  r21, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __divmodsi4
    movw r28, r16
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    mov  r9, r21
    rjmp div4_dispatch
    .align 6

I_UMOD2:
    mov  r23, r9
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __udivmodhi4
    st   Y+, r24
    mov  r9, r25
umod4_dispatch:
    dispatch

I_UMOD4:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    cp   r18, r2
    cpc  r19, r2
    cpc  r20, r2
    cpc  r21, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __udivmodsi4
    movw r28, r16
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    rjmp umod4_dispatch
    .align 6

I_MOD2:
    mov  r23, r9
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __divmodhi4
    st   Y+, r24
    mov  r9, r25
mod4_dispatch:
    dispatch

I_MOD4:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    cp   r18, r2
    cpc  r19, r2
    cpc  r20, r2
    cpc  r21, r2
    brne 1f
    ldi  r24, 3
    call call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __divmodsi4
    movw r28, r16
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    rjmp mod4_dispatch
    .align 6

I_LSL:
    mov  r20, r9
    ld   r16, -Y
    
    cpi  r20, 8
    brsh 2f

    sbrc r20, 2
    swap r16
    sbrc r20, 2
    andi r16, 0xf0
    sbrc r20, 1
    lsl  r16
    sbrc r20, 1
    lsl  r16
    sbrc r20, 0
    lsl  r16

    mov  r9, r16
1:  dispatch_noalign
2:  clr  r9
    rjmp 1b
    .align 6

I_LSL2:
    mov  r20, r9
    ld   r17, -Y
    ld   r16, -Y
    cpi  r20, 8
    breq 4f
    cpi  r20, 16
    brlo 2f
    st   Y+, r2
    clr  r9
    rjmp 3f
1:  lsl  r16
    rol  r17
2:  dec  r20
    brpl 1b
    st   Y+, r16
    mov  r9, r17
3:  dispatch_noalign
4:  st   Y+, r2
    mov  r9, r16
    rjmp 3b
    .align 6

I_LSL4:
    mov  r20, r9
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    cpi  r20, 32
    brlo 2f
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    clr  r9
    rjmp 3f
1:  lsl  r16
    rol  r17
    rol  r18
    rol  r19
2:  dec  r20
    brpl 1b
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    mov  r9, r19
3:  dispatch

I_LSR:
    mov  r20, r9
    ld   r16, -Y
    
    cpi  r20, 8
    brsh 2f

    sbrc r20, 2
    swap r16
    sbrc r20, 2
    andi r16, 0x0f
    sbrc r20, 1
    lsr  r16
    sbrc r20, 1
    lsr  r16
    sbrc r20, 0
    lsr  r16

    mov  r9, r16
1:  dispatch_noalign
2:  mov  r9, r2
    rjmp 1b
    .align 6

I_LSR2:
    mov  r20, r9
    ld   r17, -Y
    ld   r16, -Y
    cpi  r20, 16
    brlo 2f
    st   Y+, r2
    mov  r9, r2
    rjmp 3f
1:  lsr  r17
    ror  r16
2:  dec  r20
    brpl 1b
    st   Y+, r16
    mov  r9, r17
3:  dispatch

I_LSR4:
    mov  r20, r9
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    cpi  r20, 32
    brlo 2f
    st   Y+, r2
    st   Y+, r2
    st   Y+, r2
    mov  r9, r2
    rjmp 3f
1:  lsr  r19
    ror  r18
    ror  r17
    ror  r16
2:  dec  r20
    brpl 1b
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    mov  r9, r19
3:  dispatch

I_ASR:
    mov  r20, r9
    ld   r16, -Y
    cpi  r20, 7
    brlo 2f
    ldi  r21, 0x00
    sbrc r16, 7
    ldi  r21, 0xff
    mov  r9, r21
    rjmp 3f
1:  asr  r16
2:  dec  r20
    brpl 1b
    mov  r9, r16
3:  dispatch

I_ASR2:
    mov  r20, r9
    ld   r17, -Y
    ld   r16, -Y
    cpi  r20, 16
    brlo 2f
    ldi  r21, 0x00
    sbrc r17, 7
    ldi  r21, 0xff
    st   Y+, r21
    mov  r9, r21
    rjmp 3f
1:  asr  r17
    ror  r16
2:  dec  r20
    brpl 1b
    st   Y+, r16
    mov  r9, r17
dispatch_for_asr4:
3:  dispatch

I_ASR4:
    mov  r20, r9
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    cpi  r20, 32
    brlo 2f
    ldi  r21, 0x00
    sbrc r18, 7
    ldi  r21, 0xff
    st   Y+, r21
    st   Y+, r21
    st   Y+, r21
    mov  r9, r21
    rjmp dispatch_for_asr4
1:  asr  r19
    ror  r18
    ror  r17
    ror  r16
2:  dec  r20
    brpl 1b
    st   Y+, r16
    st   Y+, r17
    st   Y+, r18
    mov  r9, r19
3:  rjmp dispatch_for_asr4
    .align 6

I_AND:
    ld   r14, -Y
    and  r9, r14
    nop
    dispatch_reverse

I_AND2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    and  r10, r14
    and  r9, r15
    st   Y+, r10
    dispatch

I_AND4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    and  r10, r14
    and  r11, r15
    and  r12, r16
    and  r9, r17
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    dispatch

I_OR:
    ld   r14, -Y
    or   r9, r14
    nop
    dispatch_reverse

I_OR2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    or   r10, r14
    or   r9, r15
    st   Y+, r10
    dispatch

I_OR4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    or   r10, r14
    or   r11, r15
    or   r12, r16
    or   r9, r17
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    dispatch

I_XOR:
    ld   r14, -Y
    eor  r9, r14
    nop
    dispatch_reverse

I_XOR2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    eor  r10, r14
    eor  r9, r15
    st   Y+, r10
    dispatch

I_XOR4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    eor  r10, r14
    eor  r11, r15
    eor  r12, r16
    eor  r9, r17
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    dispatch

I_COMP:
    com  r9
    lpm
    dispatch_reverse

I_COMP2:
    ld   r10, -Y
    com  r10
    com  r9
    st   Y+, r10
    nop
    dispatch

I_COMP4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    com  r10
    com  r11
    com  r12
    com  r9
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    dispatch

I_BOOL:
    ldi  r16, 0
    cpse r9, r2
    ldi  r16, 1
    mov  r9, r16
    dispatch_reverse

I_BOOL2:
    ld   r1, -Y
    or   r9, r1
    ldi  r16, 0
    cpse r9, r2
    ldi  r16, 1
    mov  r9, r16
    dispatch

I_BOOL3:
    ld   r10, -Y
    or   r10, r9
    ld   r11, -Y
    or   r10, r11
    clr  r9
    cpse r10, r2
    mov  r9, r4
    dispatch

I_BOOL4:
    ld   r10, -Y
    or   r10, r9
    ld   r11, -Y
    or   r10, r11
    ld   r11, -Y
    or   r10, r11
    clr  r9
    cpse r10, r2
    mov  r9, r4
    dispatch

I_CULT:
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r9
    brlo 1f
    ldi  r18, 0
1:  mov  r9, r18
    dispatch

I_CULT2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    cp   r14, r10
    cpc  r15, r9
    brlo 1f
    clr  r9
    dispatch_noalign
1:  mov  r9, r4
    dispatch

I_CULT3:
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r9
    brlo 1f
    clr  r9
    dispatch_noalign
1:  mov  r9, r4
    dispatch

I_CULT4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r12
    cpc  r17, r9
    brlo 1f
    clr  r9
    dispatch_noalign
1:  mov  r9, r4
    dispatch

I_CSLT:
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r9
    brlt 1f
    ldi  r18, 0
1:  mov  r9, r18
    dispatch

I_CSLT2:
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    cp   r14, r10
    cpc  r15, r9
    brlt 1f
    clr  r9
    dispatch_noalign
1:  mov  r9, r4
    dispatch

I_CSLT3:
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r9
    brlt 1f
    clr  r9
    dispatch_noalign
1:  mov  r9, r4
    dispatch

I_CSLT4:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r12
    cpc  r17, r9
    brlt 1f
    clr  r9
    dispatch_noalign
1:  mov  r9, r4
    dispatch

I_CFEQ:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    call __eqsf2
    mov  r9, r4
    cpse r24, __zero_reg__
    clr  r9
    dispatch

I_CFLT:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    call __ltsf2
    rol  r24
    clr  r24
    rol  r24
    mov  r9, r24
    dispatch

I_NOT:
    ldi  r16, 1
    cpse r9, r2
    ldi  r16, 0
    mov  r9, r16
bz_dispatch:
bnz_dispatch:
bz1_dispatch:
bnz1_dispatch:
bzp1_dispatch:
bnzp1_dispatch:
bzp_dispatch:
bnzp_dispatch:
    dispatch_reverse

I_FADD:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    ; clobbers: r26/r27/r30/r31
    call __addsf3
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_FSUB:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    ; clobbers: r26/r27/r30/r31
    call __subsf3
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_FMUL:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    ; clobbers: r26/r27/r30/r31
    call __mulsf3
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_FDIV:
    mov  r21, r9
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r25, -Y
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    ; clobbers: r26/r27/r30/r31
    call __divsf3
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_F2I:
    mov  r25, r9
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    call __fixsfsi
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_F2U:
    mov  r25, r9
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    call __fixunssfsi
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_I2F:
    mov  r25, r9
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    call __floatsisf
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch

I_U2F:
    mov  r25, r9
    ld   r24, -Y
    ld   r23, -Y
    ld   r22, -Y
    clr  r1
    call __floatunsisf
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    mov  r9, r25
    dispatch_noalign
branch_delay_16:
    nop
branch_delay_15:
    nop
branch_delay_14:
    nop
branch_delay_13:
    nop
branch_delay_12:
    nop
branch_delay_11:
    rjmp .+0
branch_delay_9:
    rjmp .+0
branch_delay_7:
    ret
    .align 6

I_BZ:
    mov  r1, r9
    ldi  r18, 3
    add  r6, r18
    adc  r7, r2
    ld   r9, -Y
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    adc  r8, r2
    rcall branch_delay_14
    out  %[spdr], r2
    in   r17, %[spdr]
    sei
    rcall branch_delay_11
    cp   r1, r2
    brne 1f
    movw r6, r16
    in   r8, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    rjmp jump_to_pc_delayed
1:  out  %[spdr], r2
    rcall branch_delay_11
    rjmp  bz_dispatch
    .align 6

I_BZ1:
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    cp   r9, r2
    brne 1f
    ldi  r18, 3
    mov  r16, r9
    in   r0, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    ld   r9, -Y
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc_delayed2
1:  ld   r9, -Y
    out  %[spdr], r2
    rcall branch_delay_11
    rjmp bz1_dispatch
    .align 6

I_BNZ:
    mov  r1, r9
    ldi  r18, 3
    add  r6, r18
    adc  r7, r2
    ld   r9, -Y
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    adc  r8, r2
    rcall branch_delay_14
    out  %[spdr], r2
    in   r17, %[spdr]
    sei
    rcall branch_delay_11
    cp   r1, r2
    breq 1f
    movw r6, r16
    in   r8, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    rjmp jump_to_pc_delayed
1:  out  %[spdr], r2
    rcall branch_delay_11
    rjmp bnz_dispatch
    .align 6

I_BNZ1:
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    cp   r9, r2
    breq 1f
    ldi  r18, 3
    mov  r16, r9
    in   r0, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    ld   r9, -Y
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc_delayed2
1:  ld   r9, -Y
    out  %[spdr], r2
    rcall branch_delay_11
    rjmp bz1_dispatch
    .align 6

I_BZP:
    ldi  r18, 3
    add  r6, r18
    adc  r7, r2
    adc  r8, r2
    rjmp .+0
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    rcall branch_delay_15
    out  %[spdr], r2
    in   r17, %[spdr]
    sei
    rcall branch_delay_11
    cp   r9, r2
    brne 1f
    movw r6, r16
    in   r8, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    rjmp jump_to_pc_delayed
1:  out  %[spdr], r2
    ld   r9, -Y
    rcall branch_delay_9
    rjmp bzp_dispatch
    .align 6

I_BZP1:
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    ldi  r18, 3
    cp   r9, r2
    brne 1f
    nop
    in   r0, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc_delayed2
1:  out  %[spdr], r2
    ld   r9, -Y
    rcall branch_delay_9
    rjmp  bzp1_dispatch
    .align 6

I_BNZP:
    ldi  r16, 3
    add  r6, r16
    adc  r7, r2
    adc  r8, r2
    rjmp .+0
    cli
    out  %[spdr], r2
    in   r16, %[spdr]
    rcall branch_delay_15
    out  %[spdr], r2
    in   r17, %[spdr]
    sei
    rcall branch_delay_11
    cp   r9, r2
    breq 1f
    movw r6, r16
    in   r8, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    rjmp jump_to_pc_delayed
1:  out  %[spdr], r2
    ld   r9, -Y
    rcall branch_delay_9
    rjmp bnzp_dispatch
    .align 6

I_BNZP1:
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    ldi  r18, 3
    cp   r9, r2
    breq 1f
    nop
    in   r0, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc_delayed2
1:  out  %[spdr], r2
    ld   r9, -Y
    rcall branch_delay_9
    rjmp  bnzp1_dispatch
    .align 6

I_JMP:
    lpm
    rjmp .+0
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r6, %[spdr]
    ldi  r18, 3
    rcall branch_delay_14
    out  %[spdr], r2
    in   r7, %[spdr]
    out  %[sreg], r10
    rcall branch_delay_14
    in   r8, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    rjmp  jump_to_pc_delayed
    .align 6

I_JMP1:
    lpm
    lpm
    ldi  r18, 3
    in   r0, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    inc  r0
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc_delayed2
call_error:
    ldi  r24, 6
    call call_vm_error
    .align 6

I_CALL:
    lds  r26, %[vm_csp]
    cpi  r26, %[MAX_CALLS] * 3 - 3
    brsh call_error
    ldi  r27, 0x06
    in   r10, %[sreg]
    cli
    out  %[spdr], r2
    in   r0, %[spdr]
    ldi  r16, 3
    add  r16, r6
    mov  r6, r0
    adc  r7, r2
    adc  r8, r2
    st   X+, r16
    st   X+, r7
    st   X+, r8
    rjmp .+0
    rjmp .+0
    out  %[spdr], r2
    in   r7, %[spdr]
    out  %[sreg], r10
    sts  %[vm_csp], r26
    rcall branch_delay_11
    ldi  r18, 3
    in   r8, %[spdr]
    fx_disable
    fx_enable
    out  %[spdr], r18
    rjmp jump_to_pc_delayed
    .align 6

I_CALL1:
    lds  r26, %[vm_csp]
    cpi  r26, %[MAX_CALLS] * 3 - 3
    brsh 1f
    ldi  r27, 0x06
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    st   X+, r6
    st   X+, r7
    st   X+, r8
    sts  %[vm_csp], r26
    in   r0, %[spdr]
    ldi  r18, 3
    fx_disable
    fx_enable
    out  %[spdr], r18
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc_delayed2
1:  ldi  r24, 6
    call call_vm_error
    .align 6

I_RET:
    lds  r26, %[vm_csp]
    ldi  r27, 0x06
    ld   r8, -X
    ldi  r18, 3
    fx_disable
    fx_enable
    out %[spdr], r18
    ld   r7, -X
    ld   r6, -X
    sts  %[vm_csp], r26
    rjmp jump_to_pc_delayed2
    .align 6

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; NOTE! I_SYS impl is too long, but it's the last one so it's OK
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

I_SYS:
    ldi  r30, lo8(%[sys_funcs])
    ldi  r31, hi8(%[sys_funcs])
    add  r6, r4
    adc  r7, r2
    adc  r8, r2

    ; pc
    ; these regs are call-saved, but we store them anyway to
    ; allow finding file/line info in error handling
    sts  %[vm_pc]+0, r6

    in   r0, %[spdr]
    out  %[spdr], r2

    sts  %[vm_pc]+1, r7
    sts  %[vm_pc]+2, r8
    
    add  r30, r0
    adc  r31, r2
    lpm  r0, Z+
    lpm  r31, Z
    mov  r30, r0

    ; stack pointer: stack always begins at 0x100
    ; these regs are call-saved but are needed by sys funcs
    st   Y+, r9
    sts  %[vm_sp], r28

    clr  r1

    icall

    ; restore stack setup
    lds  r28, %[vm_sp]
    ld   r9, -Y

    dispatch_noalign

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ; NOTE! I_SYS impl is too long, but it's the last one so it's OK
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; helper methods
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

instr_mul4:
    ;   
    ;    A3 A2 A1 A0
    ;    B3 B2 B1 B0
    ;    ===========
    ;          A0*B0
    ;       A1*B0
    ;       A0*B1
    ;    A2*B0
    ;    A1*B1
    ;    A0*B2
    ; A3*B0
    ; A2*B1
    ; A1*B2
    ; A0*B3
    ;    ===========
    ;    C3 C2 C1 C0
    ;   
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    mul  r14, r10 ; A0*B0
    movw r18, r0
    mul  r16, r10 ; A2*B0
    movw r20, r0
    mul  r15, r10 ; A1*B0
    add  r19, r0
    adc  r20, r1
    adc  r21, r2
    mul  r14, r11 ; A0*B1
    add  r19, r0
    adc  r20, r1
    adc  r21, r2
    mul  r15, r11 ; A1*B1
    add  r20, r0
    adc  r21, r1
    mul  r14, r12 ; A0*B2
    add  r20, r0
    adc  r21, r1
    mul  r17, r10 ; A3*B0
    add  r21, r0
    mul  r16, r11 ; A2*B1
    add  r21, r0
    mul  r15, r12 ; A1*B2
    add  r21, r0
    mul  r14, r9 ; A0*B3
    add  r21, r0
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    mov  r9, r21
seek_dispatch:
    dispatch_noalign

pidx_part2:
    
    ; compute prog ref + index * elem_size
    ;
    ; A2 A1 A0 : r12 r11 r10  index
    ;    B1 B0 :     r21 r20  elem size
    ; C2 C1 C0 : r15 r14 r13  progref
    ;
    ;    A2 A1 A0
    ;       B1 B0
    ;    ========
    ;       A0*B0
    ;    A0*B1
    ;    A1*B0
    ; A1*B1
    ; A2*B0
    ;    ========
    ;    C2 C1 C0
    ;
    mul  r10, r20 ; A0*B0
    add  r13, r0
    adc  r14, r1
    adc  r15, r2
    mul  r10, r21 ; A0*B1
    add  r14, r0
    adc  r15, r1
    mul  r11, r20 ; A1*B0
    add  r14, r0
    adc  r15, r1
    mul  r11, r21 ; A1*B1
    add  r15, r0
    mul  r12, r20 ; A2*B0
    add  r15, r0
    
    ; push prog ref
    st   Y+, r13
    st   Y+, r14
    mov  r9, r15
    dispatch_noalign

upidx_delay_12:
    rjmp .+0
upidx_delay_10:
    lpm
upidx_delay_7:
    ret
    
upidx_impl:
    
    ; load elem size into r20:r21
    lpm
    rjmp .+0
    in   r20, %[spdr]
    out  %[spdr], r2
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    rcall upidx_delay_12
    in   r21, %[spdr]
    out  %[spdr], r2

    ; load index into r10:r12
    mov  r12, r9
    ld   r11, -Y
    ld   r10, -Y
    
    ; load elem count into r16:r18
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    
    ; load progref into r13:r14:r9
    ld   r9, -Y
    ld   r14, -Y
    ld   r13, -Y
    
    ; bounds check index against elem count
    cp   r10, r16
    cpc  r11, r17
    cpc  r12, r18
    brsh upidx_error
    
    ; compute prog ref + index * elem_size
    ;
    ; A2 A1 A0 : r12 r11 r10  index
    ;    B1 B0 :     r21 r20  elem size
    ; C2 C1 C0 : r9  r14 r13  progref
    ;
    ;    A2 A1 A0
    ;       B1 B0
    ;    ========
    ;       A0*B0
    ;    A0*B1
    ;    A1*B0
    ; A1*B1
    ; A2*B0
    ;    ========
    ;    C2 C1 C0
    ;
    mul  r10, r20 ; A0*B0
    add  r13, r0
    adc  r14, r1
    adc  r9, r2
    mul  r10, r21 ; A0*B1
    add  r14, r0
    adc  r9, r1
    mul  r11, r20 ; A1*B0
    add  r14, r0
    adc  r9, r1
    mul  r11, r21 ; A1*B1
    add  r9, r0
    mul  r12, r20 ; A2*B0
    add  r9, r0
    
    ; push prog ref
    st   Y+, r13
    st   Y+, r14
    dispatch_noalign

upidx_error:
    ldi  r24, 2
    rcall call_vm_error
    
jump_to_pc:
    fx_disable
1:  ldi  r18, 3
    fx_enable
    out  %[spdr], r18
    rjmp .+0

jump_to_pc_delayed:

    rjmp .+0
    rjmp .+0
    nop

jump_to_pc_delayed2:

    ; see if we need to call ards::Tones::update()
    lds  r16, %[tones_size]
    cpi  r16, %[tones_maxsize]
    brlo 2f

    lds  r16, %[data_page]+0
    lds  r17, %[data_page]+1
    add  r16, r7
    adc  r17, r8
    
    out  %[spdr], r17
    rcall seek_delay_17
    out  %[spdr], r16
    rcall seek_delay_17
    out  %[spdr], r6
    rcall seek_delay_16
    out  %[spdr], r2
    rcall seek_delay_13
    dispatch_noalign_reverse
    
2:  clr  r1
    lpm
    rjmp .+0
    fx_disable
    call %x[tones_update]
    rjmp 1b

seek_delay_17:
    nop
seek_delay_16:
    lpm
seek_delay_13:
    rjmp .+0
seek_delay_11:
    lpm
seek_delay_8:
    nop
seek_delay_7:
    ret

call_vm_error:

    ; pc
    sts  %[vm_pc]+0, r6
    sts  %[vm_pc]+1, r7
    sts  %[vm_pc]+2, r8
    
    ; stack pointer: stack always begins at 0x100
    st   Y+, r9
    sts  %[vm_sp], r28

    clr  r1

    call vm_error
    jmp  0x0000

)"

    :
    : [spdr]          "i" (_SFR_IO_ADDR(SPDR))
    , [spsr]          "i" (_SFR_IO_ADDR(SPSR))
    , [sreg]          "i" (_SFR_IO_ADDR(SREG))
    , [fxport]        "i" (_SFR_IO_ADDR(FX_PORT))
    , [fxbit]         "i" (FX_BIT)
    , [data_page]     ""  (&FX::programDataPage)
    , [vm_calls]      ""  (&ards::vm.calls[0])
    , [vm_calls_end]  ""  (&ards::vm.calls[ards::MAX_CALLS])
    , [vm_sp]         ""  (&ards::vm.sp)
    , [vm_pc]         ""  (&ards::vm.pc)
    , [vm_csp]        ""  (&ards::vm.csp)
    , [sys_funcs]     ""  (&SYS_FUNCS[0])
    , [tones_update]  ""  (ards::Tones::update)
    , [tones_size]    ""  (&ards::detail::buffer_size)
    , [tones_maxsize] ""  (sizeof(ards::detail::buffer))
    , [MAX_CALLS]     ""  (MAX_CALLS)
    );
}

void vm_run()
{
    ards::Tones::stop();
    Arduboy2Base::pollButtons();

    memset(&vm, 0, sizeof(vm));
    vm.text_mode = SpritesABC::MODE_SELFMASK;
    vm.text_font = 0xffffff;
    vm.frame_dur = 50;
    
    // read signature and refuse to run if it's not present
    {
        FX::seekData(0);
        uint32_t sig = FX::readPendingLastUInt32();
        if(sig != 0xABC00ABC)
            vm_error(ERR_SIG);
    }
    
    // entry point in header
    *(volatile uint24_t*)&vm.pc = 20;
    *(volatile uint8_t*)&vm.sp = 1;
    
    FX::seekData(20);

    // kick off execution
    asm volatile(R"(
        ; 0 constant
        clr  r2
        
        ; 32 constant
        ldi  r16, 32
        mov  r3, r16
        
        ; 1 constant
        ldi  r16, 1
        mov  r4, r16
        
        ; pm_hi8(vm_execute) constant
        ldi  r16, pm_hi8(vm_execute)
        mov  r5, r16
        
        ; pc
        lds  r6, %[vm_pc]+0
        lds  r7, %[vm_pc]+1
        lds  r8, %[vm_pc]+2
        
        ; stack pointer: stack always begins at 0x100
        lds  r28, %[vm_sp]
        ldi  r29, 0x01
        ld   r9, -Y

        in   r0, %[spdr]
        out  %[spdr], r2
        add  r6, r4
        adc  r7, r2
        adc  r8, r2
        mul  r0, r3
        movw r30, r0
        add  r31, r5
        ijmp
    )"
    :
    : [spdr]          "i" (_SFR_IO_ADDR(SPDR))
    , [vm_sp]         ""  (&ards::vm.sp)
    , [vm_pc]         ""  (&ards::vm.pc)
    );

}

}
