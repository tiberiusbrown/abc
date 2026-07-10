	.type	buf,@object
	.section	.bss,"aw",@nobits
buf:
	.zero	8
	.type	result,@object
result:
	.zero	2
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
alloc	2
$__abc.main.retbytes = 0
main.entry:
push2	8
p00
pushg	buf
sys	memset
setl2	4
getl2	2
setg2	result
popn	2
ret
