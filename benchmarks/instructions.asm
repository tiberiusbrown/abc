.global g 512

p:
    .b f 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
    .b f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f

call_long:
    sys debug_break
    ret

infinite:
    sys debug_break
    jmp infinite

main:

$L_main_0:
    jmp $L_main_start

    ; stuff for measuring branch latencies

$L_main_1:
    sys debug_break
    jmp $L_main_2

$L_main_4:
    sys debug_break
    jmp $L_main_5

$L_main_7:
    sys debug_break
    pop
    jmp $L_main_8

$L_main_15:
    sys debug_break
    pop
    jmp $L_main_16

$L_main_10:
    sys debug_break
    jmp $L_main_11

$L_main_start:

    ; measure break -> break latency
    sys debug_break
    sys debug_break
    sys debug_break
    sys debug_break
    
    sys debug_break
    p0
    sys debug_break
    pop
    
    sys debug_break
    p1
    sys debug_break
    pop
    
    sys debug_break
    p2
    sys debug_break
    pop
    
    sys debug_break
    p3
    sys debug_break
    pop
    
    sys debug_break
    p4
    sys debug_break
    pop
    
    sys debug_break
    p5
    sys debug_break
    pop
    
    sys debug_break
    p6
    sys debug_break
    pop
    
    sys debug_break
    p7
    sys debug_break
    pop
    
    sys debug_break
    p8
    sys debug_break
    pop
    
    sys debug_break
    p16
    sys debug_break
    pop
    
    sys debug_break
    p32
    sys debug_break
    pop
    
    sys debug_break
    p64
    sys debug_break
    pop
    
    sys debug_break
    p128
    sys debug_break
    pop
    
    sys debug_break
    p00
    sys debug_break
    pop2
    
    sys debug_break
    push 42
    sys debug_break
    pop
    
    sys debug_break
    push2 4242
    sys debug_break
    pop2
    
    sys debug_break
    push3 424242
    sys debug_break
    pop3
    
    sys debug_break
    push4 42424242
    sys debug_break
    pop4
    
    sys debug_break
    pzn 3
    sys debug_break
    pop3
    
    sys debug_break
    pzn 3
    sys debug_break
    pop4
    
    sys debug_break
    pzn 8
    sys debug_break
    popn 8
    
    sys debug_break
    pzn 16
    sys debug_break
    popn 16
    
    sys debug_break
    pzn 32
    sys debug_break
    popn 32

    push 0
    sys debug_break
    sext
    sys debug_break
    pop2

    push 128
    sys debug_break
    sext
    sys debug_break
    pop2

    push 0
    sys debug_break
    sext2
    sys debug_break
    pop3

    push 128
    sys debug_break
    sext2
    sys debug_break
    pop3

    push 0
    sys debug_break
    sext3
    sys debug_break
    pop4

    push 128
    sys debug_break
    sext3
    sys debug_break
    pop4
    
    sys debug_break
    dup
    sys debug_break
    pop
    
    sys debug_break
    dup2
    sys debug_break
    pop
    
    sys debug_break
    dup3
    sys debug_break
    pop
    
    sys debug_break
    dup4
    sys debug_break
    pop
    
    sys debug_break
    dup5
    sys debug_break
    pop
    
    sys debug_break
    dup6
    sys debug_break
    pop
    
    sys debug_break
    dup7
    sys debug_break
    pop
    
    sys debug_break
    dup8
    sys debug_break
    pop
    
    sys debug_break
    dupw
    sys debug_break
    pop
    
    sys debug_break
    dupw2
    sys debug_break
    pop
    
    sys debug_break
    dupw3
    sys debug_break
    pop
    
    sys debug_break
    dupw4
    sys debug_break
    pop
    
    sys debug_break
    dupw5
    sys debug_break
    pop
    
    sys debug_break
    dupw6
    sys debug_break
    pop
    
    sys debug_break
    dupw7
    sys debug_break
    pop
    
    sys debug_break
    dupw8
    sys debug_break
    pop
    
    sys debug_break
    getl 42
    sys debug_break
    pop
    
    sys debug_break
    getl2 42
    sys debug_break
    pop2
    
    sys debug_break
    getl4 42
    sys debug_break
    pop4
    
    push 5
    sys debug_break
    getln 42
    sys debug_break
    popn 5
    
    push 8
    sys debug_break
    getln 42
    sys debug_break
    popn 8
    
    push 16
    sys debug_break
    getln 42
    sys debug_break
    popn 16
    
    push 32
    sys debug_break
    getln 42
    sys debug_break
    popn 32
    
    p0
    push 42
    sys debug_break
    setl 1
    sys debug_break
    pop
    
    p00
    push 42
    push 42
    sys debug_break
    setl2 2
    sys debug_break
    pop2
    
    pzn 4
    push 42
    push 42
    push 42
    push 42
    sys debug_break
    setl4 4
    sys debug_break
    pop4
    
    pzn 5
    pzn 5
    push 5
    sys debug_break
    setln 5
    sys debug_break
    popn 5
    
    pzn 8
    pzn 8
    push 8
    sys debug_break
    setln 8
    sys debug_break
    popn 8
    
    pzn 16
    pzn 16
    push 16
    sys debug_break
    setln 16
    sys debug_break
    popn 16
    
    pzn 32
    pzn 32
    push 32
    sys debug_break
    setln 32
    sys debug_break
    popn 32
    
    sys debug_break
    getg g 0
    sys debug_break
    pop
    
    sys debug_break
    getg2 g 0
    sys debug_break
    pop2
    
    sys debug_break
    getg4 g 0
    sys debug_break
    pop4
    
    push 5
    sys debug_break
    getgn g 0
    sys debug_break
    popn 5
    
    push 8
    sys debug_break
    getgn g 0
    sys debug_break
    popn 8
    
    push 16
    sys debug_break
    getgn g 0
    sys debug_break
    popn 16
    
    push 32
    sys debug_break
    getgn g 0
    sys debug_break
    popn 32
    
    p0
    sys debug_break
    setg g 0
    sys debug_break
    
    p00
    sys debug_break
    setg2 g 0
    sys debug_break
    
    pzn 4
    sys debug_break
    setg4 g 0
    sys debug_break
    
    pzn 5
    push 5
    sys debug_break
    setgn g 0
    sys debug_break
    
    pzn 8
    push 8
    sys debug_break
    setgn g 0
    sys debug_break
    
    pzn 16
    push 16
    sys debug_break
    setgn g 0
    sys debug_break
    
    pzn 32
    push 32
    sys debug_break
    setgn g 0
    sys debug_break
    
    pushl p 0
    sys debug_break
    getp
    sys debug_break
    pop
    
    pushl p 0
    sys debug_break
    getpn 2
    sys debug_break
    pop2
    
    pushl p 0
    sys debug_break
    getpn 3
    sys debug_break
    pop3
    
    pushl p 0
    sys debug_break
    getpn 4
    sys debug_break
    pop4
    
    pushl p 0
    sys debug_break
    getpn 8
    sys debug_break
    popn 8
    
    pushl p 0
    sys debug_break
    getpn 16
    sys debug_break
    popn 16
    
    pushl p 0
    sys debug_break
    getpn 32
    sys debug_break
    popn 32
    
    p0
    sys debug_break
    pop
    sys debug_break
    
    p00
    sys debug_break
    pop2
    sys debug_break
    
    pzn 3
    sys debug_break
    pop3
    sys debug_break
    
    pzn 4
    sys debug_break
    pop4
    sys debug_break
    
    pzn 32
    sys debug_break
    popn 32
    sys debug_break
    
    p0
    sys debug_break
    refl 1
    sys debug_break
    pop3
    
    sys debug_break
    refg g 0
    sys debug_break
    pop2
    
    sys debug_break
    refg g 300
    sys debug_break
    pop2
    
    refg g 0
    sys debug_break
    getr
    sys debug_break
    pop
    
    refg g 0
    sys debug_break
    getr2
    sys debug_break
    pop2
    
    refg g 0
    sys debug_break
    getrn 3
    sys debug_break
    pop3
    
    refg g 0
    sys debug_break
    getrn 4
    sys debug_break
    pop4
    
    refg g 0
    sys debug_break
    getrn 8
    sys debug_break
    popn 8
    
    refg g 0
    sys debug_break
    getrn 16
    sys debug_break
    popn 16
    
    refg g 0
    sys debug_break
    getrn 32
    sys debug_break
    popn 32
    
    p0
    refg g 0
    sys debug_break
    setr
    sys debug_break
    
    p00
    refg g 0
    sys debug_break
    setr2
    sys debug_break
    
    pzn 3
    refg g 0
    sys debug_break
    setrn 3
    sys debug_break
    
    pzn 4
    refg g 0
    sys debug_break
    setrn 4
    sys debug_break
    
    pzn 8
    refg g 0
    sys debug_break
    setrn 8
    sys debug_break
    
    pzn 16
    refg g 0
    sys debug_break
    setrn 16
    sys debug_break
    
    pzn 32
    refg g 0
    sys debug_break
    setrn 32
    sys debug_break
    
    refg g 0
    push 42
    sys debug_break
    aixb1 50
    sys debug_break
    
    refg g 0
    push 42
    sys debug_break
    aidxb 1 50
    sys debug_break
    
    refg g 0
    push 42
    push 0
    sys debug_break
    aidx 1 50
    sys debug_break

    pushl p 0
    push 14
    sys debug_break
    pidxb 1 32
    sys debug_break

    pushl p 0
    push 14
    push 0
    push 0
    sys debug_break
    pidx 1 32
    sys debug_break
    
    refg g 0
    push 32
    p0
    p8
    p0
    sys debug_break
    uaidx 1
    sys debug_break

    pushl p 0
    push 32
    p00
    p8
    p00
    sys debug_break
    upidx 1
    sys debug_break
    
    p0
    sys debug_break
    inc
    sys debug_break
    pop
    
    p1
    sys debug_break
    dec
    sys debug_break
    pop
    
    p00
    sys debug_break
    linc 2
    sys debug_break
    pop2
    
    refg g 0
    sys debug_break
    pinc
    sys debug_break
    pop
    
    refg g 0
    sys debug_break
    pinc2
    sys debug_break
    pop2
    
    refg g 0
    sys debug_break
    pinc3
    sys debug_break
    pop3
    
    refg g 0
    sys debug_break
    pinc4
    sys debug_break
    pop4
    
    refg g 0
    sys debug_break
    pdec
    sys debug_break
    pop
    
    refg g 0
    sys debug_break
    pdec2
    sys debug_break
    pop2
    
    refg g 0
    sys debug_break
    pdec3
    sys debug_break
    pop3
    
    refg g 0
    sys debug_break
    pdec4
    sys debug_break
    pop4
    
    push 0
    push 0
    push 0
    push 0
    refg g 0
    setrn 4
    refg g 0
    sys debug_break
    pincf
    sys debug_break
    pop4
    
    push 0
    push 0
    push 128
    push 63
    refg g 0
    setrn 4
    refg g 0
    sys debug_break
    pincf
    sys debug_break
    pop4
    
    push 0
    push 36
    push 116
    push 73
    refg g 0
    setrn 4
    refg g 0
    sys debug_break
    pincf
    sys debug_break
    pop4
    
    push 0
    push 0
    push 0
    push 0
    refg g 0
    setrn 4
    refg g 0
    sys debug_break
    pdecf
    sys debug_break
    pop4
    
    push 0
    push 0
    push 128
    push 63
    refg g 0
    setrn 4
    refg g 0
    sys debug_break
    pdecf
    sys debug_break
    pop4
    
    push 0
    push 36
    push 116
    push 73
    refg g 0
    setrn 4
    refg g 0
    sys debug_break
    pdecf
    sys debug_break
    pop4
    
    p00
    sys debug_break
    add
    sys debug_break
    pop
    
    pzn 4
    sys debug_break
    add2
    sys debug_break
    pop2
    
    pzn 6
    sys debug_break
    add3
    sys debug_break
    pop3
    
    pzn 8
    sys debug_break
    add4
    sys debug_break
    pop4

    jmp infinite
    
    p00
    sys debug_break
    sub
    sys debug_break
    pop
    
    pzn 4
    sys debug_break
    sub2
    sys debug_break
    pop2
    
    pzn 6
    sys debug_break
    sub3
    sys debug_break
    pop3
    
    pzn 8
    sys debug_break
    sub4
    sys debug_break
    pop4
    
    pzn 3
    sys debug_break
    add2b
    sys debug_break
    pop2
    
    p00
    sys debug_break
    mul
    sys debug_break
    pop
    
    pzn 4
    sys debug_break
    mul2
    sys debug_break
    pop2
    
    pzn 6
    sys debug_break
    mul3
    sys debug_break
    pop3
    
    pzn 8
    sys debug_break
    mul4
    sys debug_break
    pop4
    
    push 48
    push 117
    push 7
    push 0
    sys debug_break
    udiv2
    sys debug_break
    pop2
    
    push 48
    push 117
    push 88
    push 27
    sys debug_break
    udiv2
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 7
    push 0
    push 0
    push 0
    sys debug_break
    udiv4
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 128
    push 29
    push 44
    push 4
    sys debug_break
    udiv4
    sys debug_break
    pop2
    
    push 48
    push 117
    push 7
    push 0
    sys debug_break
    div2
    sys debug_break
    pop2
    
    push 48
    push 117
    push 88
    push 27
    sys debug_break
    div2
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 7
    push 0
    push 0
    push 0
    sys debug_break
    div4
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 128
    push 29
    push 44
    push 4
    sys debug_break
    div4
    sys debug_break
    pop2
    
    push 48
    push 117
    push 7
    push 0
    sys debug_break
    umod2
    sys debug_break
    pop2
    
    push 48
    push 117
    push 88
    push 27
    sys debug_break
    umod2
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 7
    push 0
    push 0
    push 0
    sys debug_break
    umod4
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 128
    push 29
    push 44
    push 4
    sys debug_break
    umod4
    sys debug_break
    pop2
    
    push 48
    push 117
    push 7
    push 0
    sys debug_break
    mod2
    sys debug_break
    pop2
    
    push 48
    push 117
    push 88
    push 27
    sys debug_break
    mod2
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 7
    push 0
    push 0
    push 0
    sys debug_break
    mod4
    sys debug_break
    pop2
    
    push 0
    push 163
    push 225
    push 17
    push 128
    push 29
    push 44
    push 4
    sys debug_break
    mod4
    sys debug_break
    pop2
    
    push 0
    push 0
    sys debug_break
    lsl
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    lsl
    sys debug_break
    pop
    
    push 0
    push 4
    sys debug_break
    lsl
    sys debug_break
    pop
    
    push 0
    push 7
    sys debug_break
    lsl
    sys debug_break
    pop
    
    push 0
    push 8
    sys debug_break
    lsl
    sys debug_break
    pop
    
    push 0
    push 64
    sys debug_break
    lsl
    sys debug_break
    pop
    
    p00
    push 0
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    p00
    push 1
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    p00
    push 4
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    p00
    push 8
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    p00
    push 15
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    p00
    push 16
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    p00
    push 64
    sys debug_break
    lsl2
    sys debug_break
    pop2
    
    pzn 3
    push 0
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 1
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 4
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 8
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 16
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 23
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 24
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 3
    push 64
    sys debug_break
    lsl3
    sys debug_break
    pop3
    
    pzn 4
    push 0
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 1
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 4
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 8
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 16
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 24
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 31
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 32
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    pzn 4
    push 64
    sys debug_break
    lsl4
    sys debug_break
    pop4
    
    push 0
    push 0
    sys debug_break
    lsr
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    lsr
    sys debug_break
    pop
    
    push 0
    push 4
    sys debug_break
    lsr
    sys debug_break
    pop
    
    push 0
    push 7
    sys debug_break
    lsr
    sys debug_break
    pop
    
    push 0
    push 8
    sys debug_break
    lsr
    sys debug_break
    pop
    
    push 0
    push 64
    sys debug_break
    lsr
    sys debug_break
    pop
    
    p00
    push 0
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    p00
    push 1
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    p00
    push 4
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    p00
    push 8
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    p00
    push 15
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    p00
    push 16
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    p00
    push 64
    sys debug_break
    lsr2
    sys debug_break
    pop2
    
    pzn 3
    push 0
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 1
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 4
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 8
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 16
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 23
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 24
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 3
    push 64
    sys debug_break
    lsr3
    sys debug_break
    pop3
    
    pzn 4
    push 0
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 1
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 4
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 8
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 16
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 24
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 31
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 32
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    pzn 4
    push 64
    sys debug_break
    lsr4
    sys debug_break
    pop4
    
    push 0
    push 0
    sys debug_break
    asr
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    asr
    sys debug_break
    pop
    
    push 0
    push 4
    sys debug_break
    asr
    sys debug_break
    pop
    
    push 0
    push 7
    sys debug_break
    asr
    sys debug_break
    pop
    
    push 0
    push 8
    sys debug_break
    asr
    sys debug_break
    pop
    
    push 0
    push 64
    sys debug_break
    asr
    sys debug_break
    pop
    
    p00
    push 0
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    p00
    push 1
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    p00
    push 4
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    p00
    push 8
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    p00
    push 15
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    p00
    push 16
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    p00
    push 64
    sys debug_break
    asr2
    sys debug_break
    pop2
    
    pzn 3
    push 0
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 1
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 4
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 8
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 16
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 23
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 24
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 3
    push 64
    sys debug_break
    asr3
    sys debug_break
    pop3
    
    pzn 4
    push 0
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 1
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 4
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 8
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 16
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 24
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 31
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 32
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    pzn 4
    push 64
    sys debug_break
    asr4
    sys debug_break
    pop4
    
    p00
    sys debug_break
    and
    sys debug_break
    pop
    
    pzn 4
    sys debug_break
    and2
    sys debug_break
    pop2
    
    pzn 6
    sys debug_break
    and3
    sys debug_break
    pop3
    
    pzn 8
    sys debug_break
    and4
    sys debug_break
    pop4
    
    p00
    sys debug_break
    or
    sys debug_break
    pop
    
    pzn 4
    sys debug_break
    or2
    sys debug_break
    pop2
    
    pzn 6
    sys debug_break
    or3
    sys debug_break
    pop3
    
    pzn 8
    sys debug_break
    or4
    sys debug_break
    pop4
    
    p00
    sys debug_break
    xor
    sys debug_break
    pop
    
    pzn 4
    sys debug_break
    xor2
    sys debug_break
    pop2
    
    pzn 6
    sys debug_break
    xor3
    sys debug_break
    pop3
    
    pzn 8
    sys debug_break
    xor4
    sys debug_break
    pop4
    
    p0
    sys debug_break
    comp
    sys debug_break
    pop
    
    p00
    sys debug_break
    comp2
    sys debug_break
    pop2
    
    pzn 3
    sys debug_break
    comp3
    sys debug_break
    pop3
    
    pzn 4
    sys debug_break
    comp4
    sys debug_break
    pop4
    
    push 0
    sys debug_break
    bool
    sys debug_break
    pop
    
    push 1
    sys debug_break
    bool
    sys debug_break
    pop
    
    push 0
    push 0
    sys debug_break
    bool2
    sys debug_break
    pop
    
    push 1
    push 1
    sys debug_break
    bool2
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    sys debug_break
    bool3
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    sys debug_break
    bool3
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 0
    sys debug_break
    bool4
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 1
    sys debug_break
    bool4
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    cult
    sys debug_break
    pop
    
    push 1
    push 0
    sys debug_break
    cult
    sys debug_break
    pop
    
    push 0
    push 0
    push 1
    push 1
    sys debug_break
    cult2
    sys debug_break
    pop
    
    push 1
    push 1
    push 0
    push 0
    sys debug_break
    cult2
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    sys debug_break
    cult3
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    sys debug_break
    cult3
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    push 1
    sys debug_break
    cult4
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    push 0
    sys debug_break
    cult4
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    cslt
    sys debug_break
    pop
    
    push 1
    push 0
    sys debug_break
    cslt
    sys debug_break
    pop
    
    push 0
    push 0
    push 1
    push 1
    sys debug_break
    cslt2
    sys debug_break
    pop
    
    push 1
    push 1
    push 0
    push 0
    sys debug_break
    cslt2
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    sys debug_break
    cslt3
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    sys debug_break
    cslt3
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    push 1
    sys debug_break
    cslt4
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    push 0
    sys debug_break
    cslt4
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    cule
    sys debug_break
    pop
    
    push 1
    push 0
    sys debug_break
    cule
    sys debug_break
    pop
    
    push 0
    push 0
    push 1
    push 1
    sys debug_break
    cule2
    sys debug_break
    pop
    
    push 1
    push 1
    push 0
    push 0
    sys debug_break
    cule2
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    sys debug_break
    cule3
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    sys debug_break
    cule3
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    push 1
    sys debug_break
    cule4
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    push 0
    sys debug_break
    cule4
    sys debug_break
    pop
    
    push 0
    push 1
    sys debug_break
    csle
    sys debug_break
    pop
    
    push 1
    push 0
    sys debug_break
    csle
    sys debug_break
    pop
    
    push 0
    push 0
    push 1
    push 1
    sys debug_break
    csle2
    sys debug_break
    pop
    
    push 1
    push 1
    push 0
    push 0
    sys debug_break
    csle2
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    sys debug_break
    csle3
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    sys debug_break
    csle3
    sys debug_break
    pop
    
    push 0
    push 0
    push 0
    push 0
    push 1
    push 1
    push 1
    push 1
    sys debug_break
    csle4
    sys debug_break
    pop
    
    push 1
    push 1
    push 1
    push 1
    push 0
    push 0
    push 0
    push 0
    sys debug_break
    csle4
    sys debug_break
    pop
    
    push 0
    sys debug_break
    not
    sys debug_break
    pop
    
    push 1
    sys debug_break
    not
    sys debug_break
    pop
    
    push 1
    sys debug_break
    bz $L_main_0
    sys debug_break
    
    push 0
    sys debug_break
    bz $L_main_1
$L_main_2:

    push 1
    sys debug_break
    bz $L_main_3
    sys debug_break

    push 0
    sys debug_break
    bz $L_main_3
$L_main_3:
    sys debug_break
    
    push 0
    sys debug_break
    bnz $L_main_0
    sys debug_break
    
    push 1
    sys debug_break
    bnz $L_main_4
$L_main_5:

    push 0
    sys debug_break
    bnz $L_main_6
    sys debug_break

    push 1
    sys debug_break
    bnz $L_main_6
    nop
$L_main_6:
    sys debug_break
    
    push 1
    sys debug_break
    bzp $L_main_0
    sys debug_break
    
    push 0
    sys debug_break
    bzp $L_main_7
$L_main_8:
    
    push 1
    sys debug_break
    bzp $L_main_14
    sys debug_break
    
    push 0
    sys debug_break
    bzp $L_main_14
$L_main_14:
    sys debug_break
    pop
    
    push 0
    sys debug_break
    bnzp $L_main_0
    sys debug_break
    
    push 1
    sys debug_break
    bnzp $L_main_15
$L_main_16:
    
    push 0
    sys debug_break
    bnzp $L_main_17
    sys debug_break
    
    push 1
    sys debug_break
    bnzp $L_main_17
$L_main_17:
    sys debug_break
    pop

    sys debug_break
    jmp $L_main_10
$L_main_11:

    sys debug_break
    jmp $L_main_9
$L_main_9:
    sys debug_break
    
    sys debug_break
    call call_long
    
    sys debug_break
    call $L_main_12
    jmp $L_main_13
    
$L_main_12:
    sys debug_break
    ret
$L_main_13:

    call $L_main_12
    sys debug_break

    ret

$globinit:
    ret
