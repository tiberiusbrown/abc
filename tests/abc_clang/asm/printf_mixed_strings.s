	.type	ram_text,@object
	.section	.bss,"aw",@nobits
ram_text:
	.zero	4
	.type	ram_text_init,@object
	.section	.rodata,"a",@progbits
ram_text_init:
	.asciz	"ram"
	.type	fmt_output,@object
fmt_output:
	.asciz	"strings:%s|%S\n"
	.type	prog_text,@object
prog_text:
	.asciz	"prog"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push2	4
pushl	ram_text_init
pushg	ram_text
sys	94
popn	2
pushl	prog_text
pushg	ram_text
pushl	fmt_output
sys	58
sys	56
p00
setl2	4
ret
