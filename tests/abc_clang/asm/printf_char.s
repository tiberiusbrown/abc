	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"char:%c"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push	67
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	4
ret
