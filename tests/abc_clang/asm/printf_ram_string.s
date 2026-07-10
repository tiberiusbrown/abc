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
	.asciz	"ram:%s"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push2	4
pushl	text_init
pushg	text
sys	strncpy_P
popn	2
pushg	text
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	4
ret
