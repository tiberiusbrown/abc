	.type	ram_text_init,@object
	.section	.rodata,"a",@progbits
ram_text_init:
	.asciz	"ram"
	.type	ram_text,@object
	.section	.bss,"aw",@nobits
ram_text:
	.zero	4
	.type	prog_text,@object
	.section	.rodata,"a",@progbits
prog_text:
	.asciz	"prog"
	.type	fmt_output,@object
fmt_output:
	.asciz	"strings:%s|%S"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push2	4
pushl	ram_text_init
pushg	ram_text
sys	strncpy_P
popn	2
pushl	prog_text
pushg	ram_text
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	4
ret
