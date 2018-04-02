# Code thanks to Torbjörn Granlund tg@gmplib.org
# Original loop count is 1000000000	
	.globl	cycles_test1
	.text
	.align	32
cycles_test1:	mov	$10000000, %rax
	jmp	1f
	.align	32
1:	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dec	%rax
	jnz	1b
	ret
