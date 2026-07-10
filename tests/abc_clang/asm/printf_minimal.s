	.type	text,@object
text:
	.zero	16
	.type	lit_text,@object
lit_text:
	.asciz	"zap"
	.type	fmt_output,@object
fmt_output:
	.asciz	"W:%04u|S:%s|%%\n"
	.globl	main
	.type	main,@function
main:
alloc	2
$__abc.main.retbytes = 2
main.entry:
pushg	16
pushl	lit_text
pushg	text
p00
sys	47
setl2	4
pushg	text
pushg	42
pushl	fmt_output
sys	29
sys	28
p00
setl2	6
popn	2
ret
