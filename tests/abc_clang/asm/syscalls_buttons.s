	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"buttons:%d|%d|%d"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
p0
sys	just_pressed
p0
p0
sys	just_pressed
p0
p0
sys	just_pressed
p0
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	4
ret
