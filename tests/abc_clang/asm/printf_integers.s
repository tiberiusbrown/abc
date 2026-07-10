	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"ints:%d|%u|%x|%04u|%ld|%lu|%lx"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
p00
$__abc.main.retbytes = 2
main.entry:
push4	305419896
push4	123456
push4	4294843840
push2	7
push2	43981
push2	42
push2	65494
pushl	fmt_output
sys	debug_printf
sys	debug_break
p00
setl2	0
ret
