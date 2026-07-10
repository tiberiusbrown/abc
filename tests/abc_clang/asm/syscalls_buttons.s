	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"buttons:%u|%d|%d|%d|%d|%d"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
alloc	18
$__abc.main.retbytes = 2
main.entry:
sys	buttons
setl	19
getl	18
p0
setl2	19
p00
sys	just_pressed
setl	16
getl	15
p0
setl2	16
p00
sys	just_released
setl	13
getl	12
p0
setl2	13
p0
sys	pressed
setl	10
getl	9
p0
setl2	10
p00
sys	any_pressed
setl	7
getl	6
p0
setl2	7
p00
sys	not_pressed
setl	4
getl	3
p0
setl2	4
getl2	2
getl2	5
getl2	8
getl2	11
getl2	14
getl2	17
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	22
popn	18
ret
