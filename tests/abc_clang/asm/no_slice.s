	.type	text,@object
text:
	.zero	8
	.type	lit_text,@object
lit_text:
	.asciz	"ok"
	.globl	main
	.type	main,@function
main:
alloc	2
$__abc.main.retbytes = 2
main.entry:
pushg	8
pushl	<MCOperand Expr:lit_text>
pushg	<MCOperand Expr:text>
p00
sys	47
setl2	4
sys	28
p00
setl2	6
popn	2
ret
