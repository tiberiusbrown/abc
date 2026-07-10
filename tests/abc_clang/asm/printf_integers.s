	.type	fmt_output,@object
	.section	.rodata,"a",@progbits
fmt_output:
	.asciz	"ints:%d|%u|%x|%04u\n"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push2	7
push2	43981
push2	42
push2	65494
pushl	fmt_output
sys	58
sys	56
p00
setl2	4
ret
