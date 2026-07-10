	.type	text,@object
	.section	.bss,"aw",@nobits
text:
	.zero	16
	.type	lit_text,@object
	.section	.rodata,"a",@progbits
lit_text:
	.asciz	"zap"
	.type	fmt_output,@object
fmt_output:
	.asciz	"W:%04u|S:%S|%%\n"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
$__abc.main.retbytes = 2
main.entry:
push2	16
pushl	lit_text
pushg	text
sys	94
popn	2
pushl	lit_text
push2	42
pushl	fmt_output
sys	58
sys	56
p00
setl2	4
ret
