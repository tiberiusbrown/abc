	.type	text_a,@object
	.section	.bss,"aw",@nobits
text_a:
	.zero	32
	.type	text_b,@object
text_b:
	.zero	32
	.type	pad,@object
pad:
	.zero	8
	.type	trunc_buf,@object
trunc_buf:
	.zero	5
	.type	embedded_buf,@object
embedded_buf:
	.zero	8
	.type	lit_head,@object
	.section	.rodata,"a",@progbits
lit_head:
	.asciz	"cat"
	.type	lit_tail,@object
lit_tail:
	.asciz	"erpillar"
	.type	lit_exact,@object
lit_exact:
	.asciz	"cater"
	.type	fmt_string,@object
fmt_string:
	.asciz	"%s"
	.type	lit_embedded,@object
lit_embedded:
	.asciz	"ok\000xx"
	.type	fmt_output,@object
fmt_output:
	.asciz	"A:%s|L:%u|C:%d|M:%s|P:%s|T:%s|E:%s\n"
	.section	.text,"ax",@progbits
	.globl	main
	.type	main,@function
main:
alloc	36
$__abc.main.retbytes = 2
main.entry:
pushg	32
p00
pushg	text_a
sys	76
setl2	34
pushg	32
p00
pushg	text_b
sys	76
setl2	32
pushg	8
p00
pushg	pad
sys	76
setl2	30
pushg	5
p00
pushg	trunc_buf
sys	76
setl2	28
pushg	8
p00
pushg	embedded_buf
sys	76
setl2	26
pushg	32
pushl	lit_head
pushg	text_a
sys	94
setl2	24
pushg	2
pushl	lit_tail
pushg	text_a
sys	98
setl2	22
pushg	32
pushl	lit_exact
pushg	text_b
sys	94
setl2	20
pushg	32
pushg	text_a
sys	82
setl2	18
getl2	16
setl2	38
getl2	36
setl2	16
getl2	14
pushg	text_b
pushg	text_a
sys	86
setl2	14
getl2	12
setl2	36
pushg	6
pushg	text_a
pushg	text_b
sys	78
setl2	12
pushg	3
pushg	88
pushg	pad
sys	76
setl2	10
pushg	text_a
pushl	fmt_string
pushg	5
pushg	trunc_buf
sys	100
pushg	6
pushl	lit_embedded
pushg	embedded_buf
sys	80
setl2	8
getl2	36
setl2	6
getl2	34
setl2	4
pushg	embedded_buf
pushg	trunc_buf
pushg	pad
pushg	text_b
getl2	2
getl2	4
pushg	text_a
pushl	fmt_output
sys	58
sys	56
p00
setl2	40
popn	36
ret
