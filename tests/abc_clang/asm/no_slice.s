	.type	text,@object
	.section	.bss,"aw",@nobits
text:
	.zero	8
	.type	lit_text,@object
	.section	.rodata,"a",@progbits
lit_text:
	.asciz	"ok"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
alloc	2
$__abc.main.retbytes = 2
main.entry:
pushg	8
pushl	lit_text
pushg	text
sys	94
setl2	4
sys	56
p00
setl2	6
popn	2
ret
