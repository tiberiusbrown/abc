	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"buttons:%u|%d|%d"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
alloc	9
$__abc.main.retbytes = 2
main.entry:
sys	buttons
setl	10
getl	9
p0
setl2	10
p0
sys	just_pressed
setl	7
getl	6
p0
setl2	7
p0
sys	just_released
setl	4
getl	3
p0
setl2	4
getl2	2
getl2	5
getl2	8
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	13
popn	9
ret
