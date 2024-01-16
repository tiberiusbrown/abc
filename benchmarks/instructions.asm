.global g 32

p:
    .b f 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
    .b f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f

main:

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
    pzn 4
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
    getg g 0
    sys debug_break
    
    p00
    sys debug_break
    getg2 g 0
    sys debug_break
    
    pzn 4
    sys debug_break
    getg4 g 0
    sys debug_break
    
    pzn 5
    push 5
    sys debug_break
    getgn g 0
    sys debug_break
    
    pzn 8
    push 8
    sys debug_break
    getgn g 0
    sys debug_break
    
    pzn 16
    push 16
    sys debug_break
    getgn g 0
    sys debug_break
    
    pzn 32
    push 32
    sys debug_break
    getgn g 0
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

    ret

$globinit:
    ret
