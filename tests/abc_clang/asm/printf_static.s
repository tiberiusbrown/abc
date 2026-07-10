	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"static"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
p00
$__abc.main.retbytes = 2
main.entry:
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	0
ret
