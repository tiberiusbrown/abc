	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"arith:%d|%d|%d|%d"
	.section	.text,"ax",@progbits
	.globl	basic_sum
	.type	basic_sum,@function
basic_sum:
p00
basic_sum.entry:
getl2	4
getl2	2
add2
setl2	0
popn	4
ret
	.globl	basic_min
	.type	basic_min,@function
basic_min:
p00
basic_min.entry:
getl2	2
getl2	4
cslt2
bnz	.Ltmp0
getl2	4
jmp	.Ltmp1
.Ltmp0:
getl2	2
.Ltmp1:
setl2	0
popn	4
ret
	.globl	basic_max
	.type	basic_max,@function
basic_max:
p00
basic_max.entry:
getl2	2
getl2	4
cslt2
bnz	.Ltmp2
getl2	2
jmp	.Ltmp3
.Ltmp2:
getl2	4
.Ltmp3:
setl2	0
popn	4
ret
	.globl	basic_clamp
	.type	basic_clamp,@function
basic_clamp:
p00
basic_clamp.entry:
getl2	2
getl2	4
cslt2
bnz	.Ltmp4
getl2	6
getl2	2
cslt2
bnz	.Ltmp6
getl2	2
jmp	.Ltmp7
.Ltmp6:
getl2	6
.Ltmp7:
jmp	.Ltmp5
.Ltmp4:
getl2	4
.Ltmp5:
setl2	0
popn	6
ret
	.globl	array_sum
	.type	array_sum,@function
array_sum:
p00
alloc	10
array_sum.entry:
p00
setl2	10
p00
setl2	8
p00
getl2	14
cslt2
bnz	array_sum.for.body
p00
setl2	12
jmp	array_sum.for.cond.cleanup
array_sum.for.cond.cleanup:
getl2	10
setl2	10
popn	14
ret
array_sum.for.body:
getl2	12
getl2	8
push2	2
mul2
add2
getr2
getl2	6
add2
setl2	6
getl2	8
push2	1
add2
setl2	4
getl2	4
setl2	12
getl2	2
getl2	14
sub2
bool2
not
bnz	array_sum.for.cond.cleanup
getl2	2
setl2	10
getl2	4
setl2	8
jmp	array_sum.for.body
	.globl	array_sum_prog
	.type	array_sum_prog,@function
array_sum_prog:
p00
alloc	10
array_sum_prog.entry:
p00
setl2	10
p00
setl2	8
p00
getl2	15
cslt2
bnz	array_sum_prog.for.body
p00
setl2	12
jmp	array_sum_prog.for.cond.cleanup
array_sum_prog.for.cond.cleanup:
getl2	10
setl2	10
popn	15
ret
array_sum_prog.for.body:
getln	3, 12
getl2	8
p0
push3	2
mul3
add3
getpn	2
getl2	6
add2
setl2	6
getl2	8
push2	1
add2
setl2	4
getl2	4
setl2	12
getl2	2
getl2	15
sub2
bool2
not
bnz	array_sum_prog.for.cond.cleanup
getl2	2
setl2	10
getl2	4
setl2	8
jmp	array_sum_prog.for.body
	.globl	main
	.type	main,@function
main:
p00
$__abc.main.retbytes = 2
main.entry:
push2	4
push2	1
push2	5
call	basic_clamp
push2	3
push2	7
call	basic_max
push2	3
push2	7
call	basic_min
push2	3
push2	2
call	basic_sum
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	0
ret
