	.type	text,@object
	.section	.rodata,"a",@progbits
text:
	.asciz	"prog"
	.type	fmt_output,@object
fmt_output:
	.asciz	"prog:%S"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
pushl	text
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	4
ret
