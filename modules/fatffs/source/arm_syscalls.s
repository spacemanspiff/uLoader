	.text
	.align 4

	.global syscall_write
syscall_write:
	mov	r2, lr
	adds	r1, r0, #0
	movs	r0, #4
	svc	0xAB
	bx	r2
