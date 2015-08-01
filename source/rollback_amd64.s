.globl __dt_rollback_ctx
.type __dt_rollback_ctx,@function
__dt_rollback_ctx:
	push %rbp
	mov %rsp, %rbp

	// hold onto 4th arg (ucontext_t *)
	mov %rcx, %r8

	// musl's memcpy - after this sequence the stack has been
	// restored but the current values of RBP and RSP may point
	// into the middle of this region - they MUST be restored by us.
	mov %rdi,%rax
	cmp $8,%rdx
	jc 1f
	test $7,%edi
	jz 1f
2:	movsb
	dec %rdx
	test $7,%edi
	jnz 2b
1:	mov %rdx,%rcx
	shr $3,%rcx
	rep
	movsq
	and $7,%edx
	jz 1f
2:	movsb
	dec %edx
	jnz 2b

	// restore the stack + base pointers - we can't simply jmp
	// to setcontext (to avoid the call instruction pushing rip
	// onto the stack) as setcontext requires the stack itself.
1:	mov 160(%r8), %rsp // (ucontext_t *)->uc_mcontext.gregs[REG_RSP]
	mov 120(%r8), %rbp // (ucontext_t *)->uc_mcontext.gregs[REG_RBP]

	// now call setcontext with the (ucontext_t *) as its only arg
	mov %r8, %rdi
	callq setcontext@PLT
