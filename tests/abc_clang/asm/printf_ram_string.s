	.type	text,@object
	.section	.bss,"aw",@nobits
text:
	.zero	4
	.type	text_init,@object
	.section	.rodata,"a",@progbits
text_init:
	.asciz	"ram"
	.type	fmt_output,@object
fmt_output:
	.asciz	"ram:%s\n"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push2	4
pushl	text_init
pushg	text
sys	94
popn	2
pushg	text
pushl	fmt_output
sys	58
sys	56
p00
setl2	4
ret
