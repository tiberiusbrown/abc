.global abc_memcpy
.global memcpy

memcpy:
abc_memcpy:
    movw r30, r22
    movw r26, r24

    lsr  r21
    ror  r20
    brcs 2f
    rjmp 3f

1:  ld   r0, Z+
    st   X+, r0
2:  ld   r0, Z+
    st   X+, r0
3:  subi r20, 1
    sbci r21, 0
    brcc 1b

    ret

.global abc_memset
.global memset

memset:
abc_memset:
    movw r26, r24

    lsr  r21
    ror  r20
    brcc .+2
    st   X+, r22

    lsr  r21
    ror  r20
    brcs 2f
    rjmp 3f

1:  st   X+, r22
    st   X+, r22
2:  st   X+, r22
    st   X+, r22
3:  subi r20, 1
    sbci r21, 0
    brcc 1b

    ret
