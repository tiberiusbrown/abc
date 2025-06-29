; List of simplifications:
;    - no denormalization after mul/add/sub
;    - no rounding after mul/add/sub
;    - least order mantissa subproduct is skipped in mul (A0*B0)
;    - no inf/nan detection

#if 1

#define    _U(name)    name
#define MLIB_SECTION    .text.avr-libc.fplib
#define FUNC_SEGNAME    MLIB_SECTION

.macro    FUNCTION name
  .ifdef  .L__function_\name
    .err    ; FUNCTION is defined already.
  .endif
    .L__function_\name = 1
    .section  FUNC_SEGNAME, "ax", @progbits
    .type    _U(\name), "function"
    .size    _U(\name), .L__END_\name - .
.endm

.macro    ENTRY    name
  .ifndef .L__function_\name
FUNCTION \name
  .else
    .type    _U(\name), "function"
  .endif
    .global    _U(\name)
_U(\name):
.endm

.macro    ENDFUNC name
.L__END_\name:
.endm

#define rB0 r18
#define rB1 r19
#define rB2 r20
#define rB3 r21

#define rA0 r22
#define rA1 r23
#define rA2 r24
#define rA3 r25

#define rBE r26
#define rAE r27

ENTRY __fp_split3
    
    ; rA3[7] := sign(A) ^ sign(B)
    sbrc rB3, 7
    subi rA3, 0x80
    
    ; split B
    lsl  rB2
    rol  rB3
    cp   r1, rB3
    ror  rB2

ENTRY __fp_splitA

    bst  rA3, 7

    lsl  rA2
    rol  rA3
    cp   r1, rA3
    ror  rA2
    
    ; carry is cleared after previous instr
    ret

ENDFUNC __fp_splitA
ENDFUNC __fp_split3

ENTRY __fp_round

1:  lsl  rAE
    brcc 3f
    or   rAE, ZH
    brne 2f
    sbrs rA0, 0 ; round to even
    rjmp 3f
2:  subi rA0, -1
    sbci rA1, -1
    sbci rA2, -1
    sbci rA3, -1

3:  ret

ENDFUNC __fp_round

#if 1
FUNCTION __mulsf3x

1:  clr  r1
    jmp  __fp_zero

ENTRY __mulsf3
ENTRY __mulsf3x

    ; call __fp_split3

    ; __fp_split3 inlined
    sbrc rB3, 7
    subi rA3, 0x80
    lsl  rB2
    rol  rB3
    cp   r1, rB3
    ror  rB2
    bst  rA3, 7
    lsl  rA2
    rol  rA3
    cp   r1, rA3
    ror  rA2

ENTRY __mulsf3_pse

    ; check zero
    mul  rA3, rB3
    breq 1b

    ; check A or B denormal
    mov  r0, rA2
    or   r0, rB2
    brpl 1b

    ; add exponents: rB3.rA3 := rA3 + rB3
    add  rA3, rB3
    ldi  rB3, 0
    adc  rB3, rB3

#if 1
    ; multiplication:  rA2.rA1.rA0 * rB2.rB1.rB0  -->  rA2.rA1.rA0.rAE

    ; BE.ZH.ZL.AE

    ; approximation: ignore impact of A0*B0

    mul  rA0, rB1
    mov  rAE, r1

    mul  rA1, rB0
    add  rAE, r1
    clr  ZL
    adc  ZL, ZL

    mul  rA0, rB2
    add  rAE, r0
    adc  ZL, r1
    clr  ZH
    adc  ZH, ZH

    mul  rA1, rB1
    add  rAE, r0
    adc  ZL, r1
    clr  rBE
    adc  ZH, rBE

    mul  rA2, rB0
    add  rAE, r0
    adc  ZL, r1
    adc  ZH, rBE
    
    mul  rA1, rB2
    add  ZL, r0
    adc  ZH, r1
    adc  rBE, rBE

    mul  rA2, rB1
    add  ZL, r0
    adc  ZH, r1
    clr  rA0
    adc  rBE, rA0

    mul  rA2, rB2
    add  ZH, r0
    adc  rBE, r1

    movw rA0, ZL
    mov  rA2, rBE

    clr  r1

#else
    ; multiplication:  rA2.rA1.rA0 * rB2.rB1.rB0  -->  rA2.rA1.rA0.rAE.ZH.ZL

    ; ZH.ZL = rA0 * rB0
    mul  rA0, rB0
    movw ZL, r0
    ; rAE.ZH += rA1 * rB0
    mul  rA1, rB0
    clr  rAE
    add  ZH, r0
    adc  rAE, r1
    ; rBE.rAE.ZH = rAE.ZH + rA0 * rB1
    mul  rA0, rB1
    clr  rBE
    add  ZH, r0
    adc  rAE, r1
    adc  rBE, rBE
    ; rA0.rBE.rAE = rBE.rAE + rA0 * rB2
    mul  rA0, rB2
    clr  rA0
    add  rAE, r0
    adc  rBE, r1
    adc  rA0, rA0
    ; rA0.rBE.rAE += rA2 * rB0
    mul  rA2, rB0
    clr  rB0
    add  rAE, r0
    adc  rBE, r1
    adc  rA0, rB0
    ; rA0.rBE.rAE += rA1 * rB1
    mul  rA1, rB1
    add  rAE, r0
    adc  rBE, r1
    adc  rA0, rB0    ; rB0 == 0
    ; rB0.rA0.rBE = rA0.rBE + rA2 * rB1
    mul  rA2, rB1
    add  rBE, r0
    adc  rA0, r1
    adc  rB0, rB0    ; rB0 was 0
    ; rB0.rA0.rBE += rA1 * rB2
    mul  rA1, rB2
    clr  rB1
    add  rBE, r0
    adc  rA0, r1
    adc  rB0, rB1
    ; rB0.rA0 += rA2 * rB2
    mul  rA2, rB2
    add  rA0, r0
    adc  rB0, r1
    ; move result:  rA2.rA1.rA0.rAE.ZH.ZL = rB0.rA0.rBE.rAE.ZH.ZL
    mov  rA2, rB0
    mov  rA1, rA0
    mov  rA0, rBE
    ; __zero_reg__
    clr  r1
#endif

    ; normalize
    subi rA3, lo8(127)
	sbci rB3, hi8(127)
    brmi 1b              ; denormalization needed: return 0
    breq 3f              ; normalization is impossible

2:  tst  rA2
    brmi 3f
    ; mantissa <<= 1
    lsl  rAE
    rol  rA0
    rol  rA1
    rol  rA2
    ; exponent -= 1
    subi rA3, lo8(1)
    sbci rB3, hi8(1)

3:  or   Zh, ZL
    ; pack
    lsl  rA2
    adc  rA3, r1
    lsr  rA3
    ror  rA2
    bld  rA3, 7
    ret

ENDFUNC __mulsf3_pse
ENDFUNC __mulsf3x
ENDFUNC __mulsf3

#endif

#if 1

ENTRY __subsf3

    subi rB3, 0x80

ENTRY __addsf3

    clr  rAE
    clr  rBE

ENTRY __addsf3x

    ; save sign of A
    mov  ZL, rA3

    ; call __fp_split3

    ; __fp_split3 inlined
    sbrc rB3, 7
    subi rA3, 0x80
    lsl  rB2
    rol  rB3
    cp   r1, rB3
    ror  rB2
    bst  rA3, 7
    lsl  rA2
    rol  rA3
    cp   r1, rA3
    ror  rA2

    ; compare A and B
    cp   rAE, rBE
    cpc  rA0, rB0
    cpc  rA1, rB1
    cpc  rA2, rB2
    cpc  rA3, rB3
    brlo 1f       ; fabs(A) < fabs(B)
    brne 3f       ; fabs(A) > fabs(B)
    brtc .L_add
    jmp  __fp_zero

    ; swap A and B
1:  brtc 2f
    com  ZL
2:  mov  r0, rAE
    mov  rAE, rBE
    mov  rBE, r0
    movw r0, rA0
	movw rA0, rB0
	movw rB0, r0
	movw r0, rA2
	movw rA2, rB2
	movw rB2, r0
	clr  r1
3:  clr  ZH
    sub  rB3, rA3
4:  breq 6f ; shift is not needed
    cpi  rB3, -7
    brsh 5f
	cpi  rB3, -32
	brlo .L_ret
	cp   r1, rBE
	sbci ZH, 0
	mov  rBE, rB0
	mov  rB0, rB1
	mov  rB1, rB2
	clr  rB2
	subi rB3, -8
	rjmp 4b
5:  lsr  rB2
	ror  rB1
	ror  rB0
	ror  rBE
	sbci ZH, 0
	inc  rB3
	brne 5b
6:  ; direction ?
	brtc .L_add
    ; A -= B
	cp   r1, ZH
	sbc  rAE, rBE
	sbc  rA0, rB0
	sbc  rA1, rB1
	sbc  rA2, rB2
	brmi .L_ret
7:  subi  rA3, 1
	breq  8f
	lsl  ZH
	rol  rAE
	rol  rA0
	rol  rA1
	rol  rA2
	brpl 7b
	rjmp .L_ret

.L_add:
	add  rAE, rBE
	adc  rA0, rB0
	adc  rA1, rB1
	adc  rA2, rB2
	brcc .L_ret
	ror  rA2
	ror  rA1
	ror  rA0
	ror  rAE
	ror  ZH
8:  inc  rA3

.L_ret:
    lsl  rA2
    brcs 1f
    clr  rA3
1:  lsl  ZL
    ror  rA3
    ror  rA2

    ret

ENDFUNC __addsf3x
ENDFUNC __addsf3
ENDFUNC __subsf3

#endif

#if 1
ENTRY __fp_cmp

    lsl  rA3
    sbc  r0, r0
    lsl  rB3
    sbc  rBE, rBE

    ; compare
    sub  rA0, rB0
    sbc  rA1, rB1
    sbc  rA2, rB2
    sbc  rA3, rB3  ; C is set if A < B
    brne 1f

    ; absolute values are equal, check signs
    eor  r0, rBE
    breq 3f        ; if branch, rA2 = 0, C = 0

    ; force -0.0 == +0.0
    or   rB0, rB1
    or   rB0, rB2
    or   rB0, rB3
    brne 2f        ; evaluate sign(B)
    ret

    ; view argument signs
1:  eor  r0, rBE
    brne 2f
    sbci rBE, 1
2:  lsr  rBE
    ; build return value
    ldi  rA2, -1
    adc  rA2, r1
    adc  rA2, r1
3:  ret

ENDFUNC __fp_cmp
#endif


#undef rB0
#undef rB1
#undef rB2
#undef rB3

#undef rA0
#undef rA1
#undef rA2
#undef rA3

#undef rBE
#undef rAE


#endif
