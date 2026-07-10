	.type	buf,@object
	.section	.bss,"aw",@nobits
buf:
	.zero	8
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
alloc	2
$__abc.main.retbytes = 0
main.entry:
pushg	8
p00
pushg	buf
sys	76
setl2	4
popn	2
ret
