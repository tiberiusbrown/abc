	.section	.text,"ax",@progbits
	.globl	basic_sum
	.type	basic_sum,@function
basic_sum:
alloc	2
basic_sum.entry:
getl2	4
getl2	6
add2
setl2	4
getl2	2
setl2	10
popn	6
ret
	.globl	array_sum
	.type	array_sum,@function
array_sum:
alloc	16
array_sum.entry:
getl2	18
p00
p00
getl2	18
cslt2
setl	17
p00
setl2	15
p00
setl2	13
getl	16
bnz	array_sum.for.body
p00
setl2	17
jmp	array_sum.for.cond.cleanup
array_sum.for.cond.cleanup:
getl2	15
setl2	24
popn	20
ret
array_sum.for.body:
getl2	20
getl2	13
push2	2
mul2
add2
setl2	11
getl2	9
getr2
setl2	9
getl2	7
getl2	11
add2
setl2	7
getl2	13
push2	1
add2
setl2	5
getl2	3
getl2	18
sub2
bool2
not
setl	2
getl2	5
setl2	17
getl	1
bnz	array_sum.for.cond.cleanup
getl2	3
setl2	15
getl2	5
setl2	13
jmp	array_sum.for.body
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
p00
setl2	4
ret
