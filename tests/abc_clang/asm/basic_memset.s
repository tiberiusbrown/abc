	.type	buf,@object
	.section	.bss,"aw",@nobits
buf:
	.zero	8
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 0
main.entry:
push2	8
p00
pushg	buf
sys	memset
popn	2
ret
