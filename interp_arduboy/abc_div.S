.global __udivmodhi4

#if 1

#define NA r24
#define NB r25
#define DA r22
#define DB r23
#define RA r26
#define RB r27
#define CNT r21

__udivmodhi4:


    clr  RA
    clr  RB

    ; test for N < D (result is 0 r N)
    cp   NA, DA
    cpc  NB, DB
    brsh .+4
    movw r22, RA
    ret

    ; test if quotient bits is at least 8
    ; and use alternate code path if so
    cp   RA, NA
    cpc  DA, NB
    cpc  DB, RA
    brsh 1f
    ldi  CNT, 8
    rjmp __udivmodhi4_alt
1:

    ; compute quotient bit count
    ldi  CNT, 1
    sbrc DB, 7
    rjmp __udivmodhi4_loop
1:  inc  CNT
    lsl  DA
    rol  DB
    brmi __udivmodhi4_loop
    cp   DA, NA
    cpc  DB, NB
    brlo 1b

    ; compute quotient
__udivmodhi4_loop:
    lsl  RA
    rol  RB
    cp   NA, DA
    cpc  NB, DB
    brlo 1f
    sub  NA, DA
    sbc  NB, DB
    ori  RA, 1
1:  lsr  DB
    ror  DA
    dec  CNT
    brne __udivmodhi4_loop

    movw r22, RA
    ret

__udivmodhi4_alt:

    lsl  NA
    rol  NB

1:

    rol  RA
    rol  RB
    cp   RA, DA
    cpc  RB, DB
    brcs .+4
    sub  RA, DA
    sbc  RB, DB
    rol  NA
    rol  NB

    rol  RA
    rol  RB
    cp   RA, DA
    cpc  RB, DB
    brcs .+4
    sub  RA, DA
    sbc  RB, DB
    rol  NA
    rol  NB

    dec  CNT
    brne 1b

    com  NA
    com  NB

    movw r22, NA
    movw r24, RA

    ret

#undef NA
#undef NB
#undef DA
#undef DB
#undef RA
#undef RB
#undef CNT

#endif


#if 1

#define NA r22
#define NB r23
#define NC r24
#define ND r25

#define DA r18
#define DB r19
#define DC r20
#define DD r21

#define RA r26
#define RB r27
#define RC r30
#define RD r31

#define CNT r1

.global __udivmodsi4

__udivmodsi4:

    clr  RC
    clr  RD

    ; test for N < D (result is 0 r N)
    cp   NA, DA
    cpc  NB, DB
    cpc  NC, DC
    cpc  ND, DD
    brsh .+6
    movw r18, RC
    movw r20, RC
    ret

    ; test for u16/u16 division (use __udivmodhi4)
    cp   NC, RC
    cpc  ND, RC
    cpc  DC, RC
    cpc  DD, RC
    brne 1f
    movw r24, NA
    movw r22, DA
    call __udivmodhi4
    movw r18, r22
    movw r20, RC
    movw r22, r24
    movw r24, RC
    ret
1:

    ; test if quotient bits is at least 16
    ; and use alternate code path if so
    cp   RC, NA
    cpc  RC, NB
    cpc  DA, NC
    cpc  DB, ND
    cpc  DC, RC
    cpc  DD, RC
    brsh 1f
    ldi  RA, 32
    mov  CNT, RA
    movw RA, RC
    rjmp __udivmodsi4_alt
1:
    movw RA, RC

    ; compute quotient bit count
    clr  CNT
    inc  CNT
    sbrc DD, 7
    rjmp __udivmodsi4_loop
1:  inc  CNT
    lsl  DA
    rol  DB
    rol  DC
    rol  DD
    brmi __udivmodsi4_loop
    cp   DA, NA
    cpc  DB, NB
    cpc  DC, NC
    cpc  DD, ND
    brlo 1b

    ; compute quotient
__udivmodsi4_loop:
    lsl  RA
    rol  RB
    rol  RC
    rol  RD
    cp   NA, DA
    cpc  NB, DB
    cpc  NC, DC
    cpc  ND, DD
    brlo 1f
    sub  NA, DA
    sbc  NB, DB
    sbc  NC, DC
    sbc  ND, DD
    ori  RA, 1
1:  lsr  DD
    ror  DC
    ror  DB
    ror  DA
    dec  CNT
    brne __udivmodsi4_loop

    movw r18, RA
    movw r20, RC
    ret

__udivmodsi4_alt:

    lsl  NA
    rol  NB
    rol  NC
    rol  ND

1:

    rol  RA
    rol  RB
    rol  RC
    rol  RD
    cp   RA, DA
    cpc  RB, DB
    cpc  RC, DC
    cpc  RD, DD
    brcs .+8
    sub  RA, DA
    sbc  RB, DB
    sbc  RC, DC
    sbc  RD, DD
    rol  NA
    rol  NB
    rol  NC
    rol  ND

    dec  CNT
    brne 1b

    com  NA
    com  NB
    com  NC
    com  ND

    movw r18, NA
    movw r20, NC
    movw r22, RA
    movw r24, RC
    ret


#undef NA
#undef NB
#undef NC
#undef ND

#undef DA
#undef DB
#undef DC
#undef DD

#undef RA
#undef RB
#undef RC
#undef RD

#undef CNT

#endif
