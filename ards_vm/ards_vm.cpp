#include "ards_vm.hpp"
#include "ards_instr.hpp"

#include <Arduboy2.h>

extern uint8_t draw_text(uint8_t x, uint8_t y, char const* t, bool prog);

using sys_func_t = void(*)();
extern sys_func_t const SYS_FUNCS[] PROGMEM;

namespace ards
{

vm_t vm;

static uint8_t const IMG_ERR[33] PROGMEM =
{
    0xff, 0xff, 0xdb, 0xdb, 0xc3, 0x00, 0xff, 0xff, 0x1b, 0x1b, 0xff, 0xee,
    0x00, 0xff, 0xff, 0x1b, 0x1b, 0xff, 0xee, 0x00, 0x7e, 0xff, 0xc3, 0xc3,
    0xff, 0x7e, 0x00, 0xff, 0xff, 0x1b, 0x1b, 0xff, 0xee, 
};

static char const ERRC_SIG[] PROGMEM = "Game data not found";
static char const ERRC_IDX[] PROGMEM = "Array index out of bounds";
static char const ERRC_DIV[] PROGMEM = "Division by zero";
static char const ERRC_ASS[] PROGMEM = "Assertion failed";
static char const ERRC_DST[] PROGMEM = "Data stack overflow";
static char const ERRC_CST[] PROGMEM = "Call stack overflow";
static char const* const ERRC[NUM_ERR] PROGMEM =
{
    ERRC_SIG,
    ERRC_IDX,
    ERRC_DIV,
    ERRC_ASS,
    ERRC_DST,
    ERRC_CST,
};

extern "C" __attribute__((used)) void vm_error(error_t e)
{
    (void)FX::readEnd();
    vm.error = (uint8_t)e;
    Arduboy2Base::clear();
    memcpy_P(&Arduboy2Base::sBuffer[0], IMG_ERR, sizeof(IMG_ERR));
    for(uint8_t i = 0; i < 128; ++i)
        Arduboy2Base::sBuffer[128+i] = 0x0c;

    char const* t = pgm_read_ptr(&ERRC[e - 1]);
    uint8_t w = draw_text(0, 64, t, true);
    draw_text(0, 16, t, true);
    
    // find and display file/line info
    {
        uint24_t pc = vm.pc;
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
            static char const STR_FILE[] PROGMEM = "File:";
            static char const STR_LINE[] PROGMEM = "Line:";
            char fname[32];
            FX::readDataBytes(file_table + file * 32, fname, 32);
            draw_text(0, 32, STR_FILE, true);
            draw_text(20, 32, fname, false);
            draw_text(0, 40, STR_LINE, true);
            for(uint8_t i = 0; i < 5; ++i)
            {
                fname[4 - i] = char('0' + line % 10);
                line /= 10;
            }
            fname[5] = '\0';
            uint8_t i = 0;
            while(fname[i] == '0')
                ++i;
            draw_text(20, 40, &fname[i], false);
        }
    }

    FX::display();
    for(;;)
        Arduboy2Base::idle();
}


// main assembly method containing optimized implementations for each instruction
// the alignment of 512 allows optimized dispatch: the prog address for ijmp can
// be computed by adding to the high byte only instead of adding a 2-byte offset

static void __attribute__((used, naked, aligned(512))) vm_execute()
{
    asm volatile(
    
R"(

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
    
    .macro dispatch_noalign
    read_byte
    mul  r0, r3
    movw r30, r0
    add  r31, r5
    ijmp
    .endm
    
    ; need to copy macro here because max nesting is 2 apparently
    .macro dispatch
    read_byte
    mul  r0, r3
    movw r30, r0
    add  r31, r5
    ijmp
    .align 6
    .endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; instructions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

I_NOP:
    dispatch_delay
    dispatch

I_PUSH:
    dispatch_delay
    read_byte
    call delay_8
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  st   Y+, r0
    dispatch

I_P0:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  st   Y+, r2
    rjmp .+0
    dispatch

I_P1:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  ldi  r16, 1
    st   Y+, r16
    nop
    dispatch

I_P2:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  ldi  r16, 2
    st   Y+, r16
    nop
    dispatch

I_P3:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  ldi  r16, 3
    st   Y+, r16
    nop
    dispatch

I_P4:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  ldi  r16, 4
    st   Y+, r16
    nop
    dispatch

I_P00:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  st   Y+, r2
    st   Y+, r2
    dispatch

I_PUSHG:
    dispatch_delay
    in   r0, %[spdr]
    out  %[spdr], r2
    st   Y+, r0
    call delay_14
    in   r0, %[spdr]
    out  %[spdr], r2
    st   Y+, r0
    ldi  r16, 2
    add  r6, r16
    adc  r7, r2
    adc  r8, r2
    call delay_10
    dispatch

I_PUSHL:
    dispatch_delay
    in   r0, %[spdr]
    out  %[spdr], r2
    st   Y+, r0
    call delay_14
    in   r0, %[spdr]
    out  %[spdr], r2
    st   Y+, r0
    call delay_14
    in   r0, %[spdr]
    out  %[spdr], r2
    st   Y+, r0
    ldi  r16, 3
    add  r6, r16
    adc  r7, r2
    adc  r8, r2
    call delay_10
    dispatch

I_SEXT:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  ld   r0, -Y
    inc  r28
    ldi  r16, 0xff
    sbrs r0, 7
    ldi  r16, 0x00
    st   Y+, r16
    dispatch
 
I_DUP:
    cpi  r28, 255
    breq 1f
    ld   r0, -Y
    st   Y+, r0
    st   Y+, r0
    dispatch_noalign
1:  ldi  r24, 5
    jmp  call_vm_error
    .align 6
   
I_GETL:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  lpm
    movw r26, r28
    read_byte
    sub  r26, r0
    ld   r0, X
    st   Y+, r0
    lpm
    lpm
    nop
    dispatch
    
I_GETL2:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  lpm
    movw r26, r28
    read_byte
    sub  r26, r0
    ld   r0, X+
    ld   r1, X
    st   Y+, r0
    st   Y+, r1
    lpm
    dispatch

I_GETLN:
    ld   r16, -Y
    mov  r17, r16
    add  r17, r28
    brcc 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  dec  r16
    read_byte
    movw r26, r28
    sub  r26, r0
    ld   r0, X+
    st   Y+, r0
2:  ld   r0, X+
    st   Y+, r0
    dec  r16
    brne 2b
    ; call delay_8 ; TODO: remove this when GETLN(1) is not allowed
    dispatch

I_SETL:
    dispatch_delay
    read_byte
    ld   r16, -Y
    movw r26, r28
    sub  r26, r0
    st   X, r16
    lpm
    lpm
    nop
    dispatch
 
I_SETLN:
    ld   r16, -Y
    dec  r16
    rjmp .+0
    rjmp .+0
    read_byte
    movw r26, r28
    sub  r26, r0
    ld   r0, -Y
    st   -X, r0
1:  ld   r0, -Y
    st   -X, r0
    dec  r16
    brne 1b
    ; call delay_8 ; TODO: remove this when SETLN(1) is not allowed
    dispatch

I_GETG:
    cpi  r28, 255
    brne 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  call read_2_bytes_nodelay
    movw r26, r16
    subi r27, -2
    ld   r16, X
    st   Y+, r16
    call delay_11
    dispatch

I_GETGN:
    ld   r18, -Y
    mov  r19, r18
    add  r19, r28
    brcc 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  call read_2_bytes_nodelay
    movw r26, r16
    subi r27, -2
2:  ld   r0, X+
    st   Y+, r0
    dec  r18
    brne 2b
    ; call delay_8 ; TODO: remove this when GETGN(1) is not allowed
    dispatch

I_SETG:
    call read_2_bytes
    movw r26, r16
    subi r27, -2
    ld   r16, -Y
    st   X, 16
    call delay_11
    dispatch

I_SETGN:
    call read_2_bytes
    movw r26, r16
    subi r27, -2
    ld   r16, -Y
    add  r26, r16
    adc  r27, r2
1:  ld   r0, -Y
    st   -X, r0
    dec  r16
    brne 1b
    ; call delay_8 ; TODO: remove this when SETGN(1) is not allowed
    dispatch

I_GETP:
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    call seek_to_addr
    call delay_12
    in   r0, %[spdr]
    st   Y+, r0
    jmp  jump_to_pc
    .align 6

I_GETPN:
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    nop
    in   r1, %[spdr]
    call seek_to_addr
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
1:  call delay_11
    in   r0, %[spdr]
    out  %[spdr], r2
    st   Y+, r0
    dec  r1
    brne 1b
    call delay_9
    jmp  jump_to_pc
    .align 6

I_GETR:
    ld   r27, -Y
    ld   r26, -Y
    ld   r1, X+
    st   Y+, r1
    dispatch

I_GETR2:
    ld   r27, -Y
    ld   r26, -Y
    ld   r1, X+
    st   Y+, r1
    ld   r1, X+
    st   Y+, r1
    dispatch

I_GETRN:
    ld   r27, -Y
    ld   r26, -Y
    mov  r18, r28
    rjmp .+0
    read_byte
    add  r18, r0
    brcc 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  ld   r1, X+
    st   Y+, r1
    dec  r0
    brne 1b
    ; lpm ; TODO: remove this when GETRN(1) is not allowed
    dispatch

I_SETR:
    ld   r27, -Y
    ld   r26, -Y
    ld   r1, -Y
    st   X, r1
    dispatch

I_SETR2:
    ld   r27, -Y
    ld   r26, -Y
    ld   r1, -Y
    ld   r0, -Y
    st   X+, r0
    st   X, r1
    dispatch

I_SETRN:
    ld   r27, -Y
    ld   r26, -Y
    lpm
    read_byte
    add  r26, r0
    adc  r27, r2
1:  ld   r1, -Y
    st   -X, r1
    dec  r0
    brne 1b
    ; nop ; TODO: remove this when SETRN(1) is not allowed
    dispatch

I_POP:
    dec  r28
    lpm
    lpm
    dispatch

I_POP2:
    subi r28, 2
    lpm
    lpm
    dispatch

I_POP3:
    subi r28, 3
    lpm
    lpm
    dispatch

I_POP4:
    subi r28, 4
    lpm
    lpm
    dispatch

I_AIDXB:
    ld   r20, -Y
    nop
    call read_2_bytes_nodelay
    ; r16: elem size
    ; r17: num elems
    ; r20: index
    cp   r20, r17
    brlo 1f
    ldi  r24, 2
    jmp  call_vm_error
1:  mul  r16, r20
    movw r22, r0
    ld   r21, -Y
    ld   r20, -Y
    add  r22, r20
    adc  r23, r21
    st   Y+, r22
    st   Y+, r23
    dispatch

I_AIDX:
    ld   r21, -Y
    ld   r20, -Y
    call read_4_bytes_nodelay
    cp   r20, r18
    cpc  r21, r19
    brlo 1f
    ldi  r24, 2
    jmp  call_vm_error
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
    movw r22, r0
    mul  r16, r21
    add  r23, r0
    mul  r17, r20
    add  r23, r0
    ld   r21, -Y
    ld   r20, -Y
    add  r22, r20
    adc  r23, r21
    st   Y+, r22
    st   Y+, r23
    dispatch

I_PIDXB:
    dispatch_delay
    dispatch

I_PIDX:
    jmp pidx_impl
    .align 6

I_REFL:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  rjmp .+0
    rjmp .+0
    read_byte
    movw r16, r28
    sub  r16, r0
    st   Y+, r16
    st   Y+, r17
    lpm
    lpm
    nop
    dispatch

I_REFG:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  call read_2_bytes_nodelay
    subi r17, -2
    st   Y+, r16
    st   Y+, r17
    lpm
    lpm
    nop
    dispatch

I_REFGB:
    cpi  r28, 254
    brlo 1f
    ldi  r24, 5
    jmp  call_vm_error
1:  rjmp .+0
    rjmp .+0
    read_byte
    ldi  r17, 2
    st   Y+, r0
    st   Y+, r17
    lpm
    lpm
    rjmp .+0
    dispatch

I_INC:
    ld   r0, -Y
    inc  r0
    st   Y+, r0
    rjmp .+0
    dispatch

I_ADD:
    ld   r10, -Y
    ld   r14, -Y
    add  r14, r10
    st   Y+, r14
    dispatch

I_ADD2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r14, r10
    adc  r15, r11
    st   Y+, r14
    st   Y+, r15
    dispatch

I_ADD3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r14, r10
    adc  r15, r11
    adc  r16, r12
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    dispatch

I_ADD4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    add  r14, r10
    adc  r15, r11
    adc  r16, r12
    adc  r17, r13
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    st   Y+, r17
    dispatch

I_SUB:
    ld   r10, -Y
    ld   r14, -Y
    sub  r14, r10
    st   Y+, r14
    dispatch

I_SUB2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    sub  r14, r10
    sbc  r15, r11
    st   Y+, r14
    st   Y+, r15
    dispatch

I_SUB3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    sub  r14, r10
    sbc  r15, r11
    sbc  r16, r12
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    dispatch

I_SUB4:
    ld   r13, -Y
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
    sbc  r17, r13
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    st   Y+, r17
    dispatch

I_MUL:
    ld   r10, -Y
    ld   r14, -Y
    mul  r14, r10
    st   Y+, r0
    dispatch

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
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    mul  r14, r10 ; A0*B0
    movw r18, r0
    mul  r14, r11 ; A0*B1
    add  r19, r0
    mul  r15, r10 ; A1*B0
    add  r19, r0
    st   Y+, r18
    st   Y+, r19
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
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    mul  r14, r10 ; A0*B0
    movw r18, r0
    mul  r16, r10 ; A2*B0
    mov  r20, r0
    mul  r15, r10 ; A1*B0
    add  r19, r0
    adc  r20, r1
    mul  r14, r11 ; A0*B1
    add  r19, r0
    adc  r20, r1
    mul  r15, r11 ; A1*B1
    add  r20, r0
    mul  r14, r12 ; A0*B2
    add  r20, r0
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    dispatch

I_MUL4:
    jmp  instr_mul4
    .align 6

I_UDIV2:
    ld   r23, -Y
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    jmp  call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __udivmodhi4
    st   Y+, r22
    st   Y+, r23
    dispatch

I_UDIV4:
    ld   r21, -Y
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
    jmp  call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __udivmodsi4
    movw r28, r16
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    st   Y+, r21
    jmp dispatch_func
    .align 6

I_DIV2:
    ld   r23, -Y
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    jmp  call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __divmodhi4
    st   Y+, r22
    st   Y+, r23
    dispatch

I_DIV4:
    ld   r21, -Y
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
    jmp  call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __divmodsi4
    movw r28, r16
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    st   Y+, r21
    jmp dispatch_func
    .align 6

I_UMOD2:
    ld   r23, -Y
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    jmp  call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __udivmodhi4
    st   Y+, r24
    st   Y+, r25
    dispatch

I_UMOD4:
    ld   r21, -Y
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
    jmp  call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __udivmodsi4
    movw r28, r16
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    st   Y+, r25
    jmp dispatch_func
    .align 6

I_MOD2:
    ld   r23, -Y
    ld   r22, -Y
    ld   r25, -Y
    ld   r24, -Y
    cp   r22, r2
    cpc  r23, r2
    brne 1f
    ldi  r24, 3
    jmp  call_vm_error
    ; dividend / remainder: r24:r25
    ; divisor  / quotient:  r22:r23
    ; clobbers:             r21, r26:r27
1:  call __divmodhi4
    st   Y+, r24
    st   Y+, r25
    dispatch

I_MOD4:
    ld   r21, -Y
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
    jmp  call_vm_error
1:  movw r16, r28
    ; dividend / remainder: r22:r25
    ; divisor  / quotient:  r18:r21
    ; clobbers:             r21, r26:r31
    call __divmodsi4
    movw r28, r16
    st   Y+, r22
    st   Y+, r23
    st   Y+, r24
    st   Y+, r25
    jmp dispatch_func
    .align 6

I_LSL:
    ld   r20, -Y
    ld   r16, -Y
    andi r20, 7
    rjmp 2f
1:  lsl  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSL2:
    ld   r20, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 15
    rjmp 2f
1:  lsl  r17
    rol  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSL3:
    ld   r20, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 31
    rjmp 2f
1:  lsl  r18
    rol  r17
    rol  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSL4:
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 31
    rjmp 2f
1:  lsl  r19
    rol  r18
    rol  r17
    rol  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSR:
    ld   r20, -Y
    ld   r16, -Y
    andi r20, 7
    rjmp 2f
1:  lsr  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSR2:
    ld   r20, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 15
    rjmp 2f
1:  lsr  r17
    ror  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSR3:
    ld   r20, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 31
    rjmp 2f
1:  lsr  r18
    ror  r17
    ror  r16
2:  dec  r20
    brpl 1b
    dispatch

I_LSR4:
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 31
    rjmp 2f
1:  lsr  r19
    ror  r18
    ror  r17
    ror  r16
2:  dec  r20
    brpl 1b
    dispatch

I_ASR:
    ld   r20, -Y
    ld   r16, -Y
    andi r20, 7
    rjmp 2f
1:  asr  r16
2:  dec  r20
    brpl 1b
    dispatch

I_ASR2:
    ld   r20, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 15
    rjmp 2f
1:  asr  r17
    ror  r16
2:  dec  r20
    brpl 1b
    dispatch

I_ASR3:
    ld   r20, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 31
    rjmp 2f
1:  asr  r18
    ror  r17
    ror  r16
2:  dec  r20
    brpl 1b
    dispatch

I_ASR4:
    ld   r20, -Y
    ld   r19, -Y
    ld   r18, -Y
    ld   r17, -Y
    ld   r16, -Y
    andi r20, 31
    rjmp 2f
1:  asr  r19
    ror  r18
    ror  r17
    ror  r16
2:  dec  r20
    brpl 1b
    dispatch

I_AND:
    ld   r10, -Y
    ld   r14, -Y
    and  r14, r10
    st   Y+, r14
    dispatch

I_AND2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    and  r14, r10
    and  r15, r11
    st   Y+, r14
    st   Y+, r15
    dispatch

I_AND3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    and  r14, r10
    and  r15, r11
    and  r16, r12
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    dispatch

I_AND4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    and  r14, r10
    and  r15, r11
    and  r16, r12
    and  r17, r13
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    st   Y+, r17
    dispatch

I_OR:
    ld   r10, -Y
    ld   r14, -Y
    or   r14, r10
    st   Y+, r14
    dispatch

I_OR2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    or   r14, r10
    or   r15, r11
    st   Y+, r14
    st   Y+, r15
    dispatch

I_OR3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    or   r14, r10
    or   r15, r11
    or   r16, r12
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    dispatch

I_OR4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    or   r14, r10
    or   r15, r11
    or   r16, r12
    or   r17, r13
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    st   Y+, r17
    dispatch

I_XOR:
    ld   r10, -Y
    ld   r14, -Y
    eor  r14, r10
    st   Y+, r14
    dispatch

I_XOR2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    eor  r14, r10
    eor  r15, r11
    st   Y+, r14
    st   Y+, r15
    dispatch

I_XOR3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    eor  r14, r10
    eor  r15, r11
    eor  r16, r12
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    dispatch

I_XOR4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    eor  r14, r10
    eor  r15, r11
    eor  r16, r12
    eor  r17, r13
    st   Y+, r14
    st   Y+, r15
    st   Y+, r16
    st   Y+, r17
    dispatch

I_COMP:
    ld   r10, -Y
    com  r10
    st   Y+, r10
    rjmp .+0
    dispatch

I_COMP2:
    ld   r11, -Y
    ld   r10, -Y
    com  r10
    com  r11
    st   Y+, r10
    st   Y+, r11
    dispatch

I_COMP3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    com  r10
    com  r11
    com  r12
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    dispatch

I_COMP4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    com  r10
    com  r11
    com  r12
    com  r13
    st   Y+, r10
    st   Y+, r11
    st   Y+, r12
    st   Y+, r13
    dispatch

I_BOOL:
    ld   r0, -Y
    ldi  r16, 0
    cpse r0, r2
    ldi  r16, 1
    st   Y+, r16
    dispatch

I_BOOL2:
    ld   r0, -Y
    ld   r1, -Y
    or   r0, r1
    ldi  r16, 0
    cpse r0, r2
    ldi  r16, 1
    st   Y+, r16
    dispatch

I_BOOL3:
    ld   r0, -Y
    ld   r1, -Y
    or   r0, r1
    ld   r1, -Y
    or   r0, r1
    ldi  r16, 0
    cpse r0, r2
    ldi  r16, 1
    st   Y+, r16
    dispatch

I_BOOL4:
    ld   r0, -Y
    ld   r1, -Y
    or   r0, r1
    ld   r1, -Y
    or   r0, r1
    ld   r1, -Y
    or   r0, r1
    ldi  r16, 0
    cpse r0, r2
    ldi  r16, 1
    st   Y+, r16
    dispatch

I_CULT:
    ld   r10, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    brlo 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULT2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    cpc  r15, r11
    brlo 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULT3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r12
    brlo 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULT4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r12
    cpc  r17, r13
    brlo 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLT:
    ld   r10, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    brlt 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLT2:
    ld   r11, -Y
    ld   r10, -Y
    ld   r15, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    cpc  r15, r11
    brlt 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLT3:
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r12
    brlt 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLT4:
    ld   r13, -Y
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    ld   r17, -Y
    ld   r16, -Y
    ld   r15, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r14, r10
    cpc  r15, r11
    cpc  r16, r12
    cpc  r17, r13
    brlt 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULE:
    ld   r10, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r10, r14
    brsh 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULE2:
    ld   r10, -Y
    ld   r11, -Y
    ld   r14, -Y
    ld   r15, -Y
    ldi  r18, 1
    cp   r10, r14
    cpc  r11, r15
    brsh 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULE3:
    ld   r10, -Y
    ld   r11, -Y
    ld   r12, -Y
    ld   r14, -Y
    ld   r15, -Y
    ld   r16, -Y
    ldi  r18, 1
    cp   r10, r14
    cpc  r11, r15
    cpc  r12, r16
    brsh 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CULE4:
    ld   r10, -Y
    ld   r11, -Y
    ld   r12, -Y
    ld   r13, -Y
    ld   r14, -Y
    ld   r15, -Y
    ld   r16, -Y
    ld   r17, -Y
    ldi  r18, 1
    cp   r10, r14
    cpc  r11, r15
    cpc  r12, r16
    cpc  r13, r17
    brsh 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLE:
    ld   r10, -Y
    ld   r14, -Y
    ldi  r18, 1
    cp   r10, r14
    brge 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLE2:
    ld   r10, -Y
    ld   r11, -Y
    ld   r14, -Y
    ld   r15, -Y
    ldi  r18, 1
    cp   r10, r14
    cpc  r11, r15
    brge 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLE3:
    ld   r10, -Y
    ld   r11, -Y
    ld   r12, -Y
    ld   r14, -Y
    ld   r15, -Y
    ld   r16, -Y
    ldi  r18, 1
    cp   r10, r14
    cpc  r11, r15
    cpc  r12, r16
    brge 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_CSLE4:
    ld   r10, -Y
    ld   r11, -Y
    ld   r12, -Y
    ld   r13, -Y
    ld   r14, -Y
    ld   r15, -Y
    ld   r16, -Y
    ld   r17, -Y
    ldi  r18, 1
    cp   r10, r14
    cpc  r11, r15
    cpc  r12, r16
    cpc  r13, r17
    brge 1f
    ldi  r18, 0
1:  st   Y+, r18
    dispatch

I_NOT:
    ld   r0, -Y
    ldi  r16, 1
    cpse r0, r2
    ldi  r16, 0
    st   Y+, r16
    dispatch

I_BZ:
    ld   r0, -Y
    nop
    call read_3_bytes_end_nodelay
    cp   r0, r2
    brne 1f
    movw r6, r16
    mov  r8, r18
    jmp  jump_to_pc
1:  out  %[spdr], r2
    call delay_17
    dispatch

I_BZ1:
    ld   r16, -Y
    add  r6, r4
    adc  r7, r2
    adc  r7, r2
    rjmp .+0
    in   r0, %[spdr]
    cp   r16, r2
    brne 1f
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    jmp  jump_to_pc
1:  out  %[spdr], r2
    call delay_14
    jmp  dispatch_func
    .align 6

I_BNZ:
    ld   r0, -Y
    nop
    call read_3_bytes_end_nodelay
    cp   r0, r2
    breq 1f
    movw r6, r16
    mov  r8, r18
    jmp  jump_to_pc
1:  out  %[spdr], r2
    call delay_17
    dispatch

I_BNZ1:
    ld   r16, -Y
    add  r6, r4
    adc  r7, r2
    adc  r7, r2
    rjmp .+0
    in   r0, %[spdr]
    cp   r16, r2
    breq 1f
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    jmp  jump_to_pc
1:  out  %[spdr], r2
    call delay_14
    jmp  dispatch_func
    .align 6

I_JMP:
    call read_3_bytes_end
    movw r6, r16
    mov  r8, r18
    jmp  jump_to_pc
    .align 6

I_JMP1:
    dispatch_delay
    in   r0, %[spdr]
    inc  r0
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    jmp  jump_to_pc
    .align 6

I_CALL:
    lds  r26, 0x0664
    cpi  r26, 93
    brlo 1f
    ldi  r24, 6
    jmp  call_vm_error
1:  ldi  r27, 0x06
    call read_3_bytes_end_nodelay
    st   X+, r6
    st   X+, r7
    st   X+, r8
    sts  0x0664, r26
    movw r6, r16
    mov  r8, r18
    jmp  jump_to_pc
    .align 6

I_CALL1:
    lds  r26, 0x0664
    cpi  r26, 93
    brlo 1f
    ldi  r24, 6
    jmp  call_vm_error
1:  ldi  r27, 0x06
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    st   X+, r6
    st   X+, r7
    st   X+, r8
    sts  0x0664, r26
    in   r0, %[spdr]
    mov  r1, r0
    lsl  r1
    sbc  r1, r1
    add  r6, r0
    adc  r7, r1
    adc  r8, r1
    rjmp jump_to_pc
    .align 6

I_RET:
    lds  r26, 0x0664
    ldi  r27, 0x06
    ld   r8, -X
    ld   r7, -X
    ld   r6, -X
    sts  0x0664, r26
    rjmp jump_to_pc
    .align 6

I_SYS:
    ldi  r31, hi8(%[sys_funcs])
    rjmp .+0
    call read_2_bytes_nodelay
    mov  r30, r16
    add  r31, r17
    lpm  r0, Z+
    lpm  r31, Z
    mov  r30, r0
    rcall store_vm_state
    icall
    rcall restore_vm_state
    dispatch

I_SYSB:
    ldi  r31, hi8(%[sys_funcs])
    add  r6, r4
    adc  r7, r2
    adc  r8, r2
    lpm
    in   r0, %[spdr]
    out  %[spdr], r2
    mov  r30, r0
    lpm  r0, Z+
    lpm  r31, Z
    mov  r30, r0
    rcall store_vm_state
    icall
    rcall restore_vm_state
    dispatch

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
    ld   r13, -Y
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
    mul  r14, r13 ; A0*B3
    add  r21, r0
    st   Y+, r18
    st   Y+, r19
    st   Y+, r20
    st   Y+, r21
    dispatch_noalign

read_4_bytes:
    lpm
read_4_bytes_nodelay:
    in   r16, %[spdr]
    out  %[spdr], r2
    ldi  r17, 4
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    call delay_12
    in   r17, %[spdr]
    out  %[spdr], r2
    call delay_16
    in   r18, %[spdr]
    out  %[spdr], r2
    call delay_16
    in   r19, %[spdr]
    out  %[spdr], r2
    ret

read_3_bytes:
    lpm
read_3_bytes_nodelay:
    in   r16, %[spdr]
    out  %[spdr], r2
    ldi  r17, 3
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    call delay_12
    in   r17, %[spdr]
    out  %[spdr], r2
    call delay_16
    in   r18, %[spdr]
    out  %[spdr], r2
    ret

read_3_bytes_end:
    lpm
read_3_bytes_end_nodelay:
    in   r16, %[spdr]
    out  %[spdr], r2
    ldi  r17, 3
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    call delay_12
    in   r17, %[spdr]
    out  %[spdr], r2
    call delay_16
    in   r18, %[spdr]
    ret

read_2_bytes:
    lpm
read_2_bytes_nodelay:
    in   r16, %[spdr]
    out  %[spdr], r2
    ldi  r17, 2
    add  r6, r17
    adc  r7, r2
    adc  r8, r2
    call delay_12
    in   r17, %[spdr]
    out  %[spdr], r2
    ret

    ; these two methods are used by the SYS instruction
store_vm_state:

    ; pc
    sts  0x0661, r6
    sts  0x0662, r7
    sts  0x0663, r8
    
    ; stack pointer: stack always begins at 0x100
    sts  0x0660, r28

    clr r1

    ret
    
restore_vm_state:

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
    lds  r6, 0x0661
    lds  r7, 0x0662
    lds  r8, 0x0663
    
    ; stack pointer: stack always begins at 0x100
    lds  r28, 0x0660
    ldi  r29, 0x01

    ret

    ; the following delays assume a call via call, NOT rcall
    ; make sure you compile/link WITHOUT linker relaxing!
delay_17:
    nop
delay_16:
    nop
delay_15:
    nop
delay_14:
    nop
delay_13:
    nop
delay_12:
    nop
delay_11:
    nop
delay_10:
    nop
delay_9:
    nop
delay_8:
    ret
    
pidx_impl:
    
    ; load elem size into r20:r21
    call read_2_bytes_nodelay
    movw r20, r16

    ; load index into r10:r12
    ld   r12, -Y
    ld   r11, -Y
    ld   r10, -Y
    
    ; load progref into r13:r15
    ld   r15, -Y
    ld   r14, -Y
    ld   r13, -Y
    
    ; load elem count into r16:r18
    call read_3_bytes_nodelay
    
    ; bounds check index against elem count
    cp   r10, r16
    cpc  r11, r17
    cpc  r12, r18
    brlo 1f
    ldi  r24, 2
    rjmp call_vm_error
    
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
1:  mul  r10, r20 ; A0*B0
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
    st   Y+, r15
    dispatch
    
jump_to_pc:
    fx_disable
    ldi  r18, 3
    fx_enable
    out  %[spdr], r18
    lds  r16, %[data_page]+0
    lds  r17, %[data_page]+1
    add  r16, r7
    adc  r17, r8
    call delay_11
    out  %[spdr], r17
    call delay_17
    out  %[spdr], r16
    call delay_17
    out  %[spdr], r6
    call delay_17
    out  %[spdr], r2
    call delay_17
    dispatch
    
    ; addr to seek to in r16:r18
seek_to_addr:
    fx_disable
    ldi  r19, 3
    fx_enable
    out  %[spdr], r19
    lds  r10, %[data_page]+0
    lds  r11, %[data_page]+1
    add  r10, r17
    adc  r11, r18
    call delay_11
    out  %[spdr], r11
    call delay_17
    out  %[spdr], r10
    call delay_17
    out  %[spdr], r16
    call delay_17
    out  %[spdr], r2
    ret
    
dispatch_func:
    dispatch_noalign

call_vm_error:
    rcall store_vm_state
    call vm_error
    jmp  0x0000

)"

    :
    : [spdr]       "i" (_SFR_IO_ADDR(SPDR))
    , [spsr]       "i" (_SFR_IO_ADDR(SPSR))
    , [fxport]     "i" (_SFR_IO_ADDR(FX_PORT))
    , [fxbit]      "i" (FX_BIT)
    , [data_page]  ""  (&FX::programDataPage)
    , [sys_funcs]  ""  (&SYS_FUNCS[0])
    );
}

void vm_run()
{
    Arduboy2Base::setFrameDuration(32);
    
    memset(&vm, 0, sizeof(vm));
    
    // read signature and refuse to run if it's not present
    {
        FX::seekData(0);
        uint32_t sig = FX::readPendingLastUInt32();
        if(sig != 0xABC00ABC)
            vm_error(ERR_SIG);
    }
    
    // entry point in header
    *(volatile uint24_t*)&vm.pc = 20;

    // kick off execution
    asm volatile(R"(
    
        call restore_vm_state
        jmp  jump_to_pc
    )"
    :
    : [spdr]       "i" (_SFR_IO_ADDR(SPDR))
    , [spsr]       "i" (_SFR_IO_ADDR(SPSR))
    , [fxport]     "i" (_SFR_IO_ADDR(FX_PORT))
    , [fxbit]      "i" (FX_BIT)
    , [data_page]  ""  (&FX::programDataPage)
    );

}

}
