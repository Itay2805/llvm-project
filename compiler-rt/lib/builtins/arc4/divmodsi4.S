//===-- divmodsi4.S - 32-bit signed divmod --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __divmodsi4(int a, int b, int *rem)
//   r0 = dividend, r1 = divisor, r2 = pointer to remainder
//   Returns quotient in r0, stores remainder via r2.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__divmodsi4
	.type	__divmodsi4, @function
	.balign	4
__divmodsi4:
	sub	sp, sp, 20
	st	blink, [sp, 0]
	st	r13, [sp, 4]
	st	r14, [sp, 8]
	st	r15, [sp, 12]

	xor	r13, r0, r1		// sign of quotient
	and	r14, r0, r0		// sign of remainder = dividend sign
	and	r15, r2, r2		// rem pointer

	sub	r5, r5, r5
	sub.f	0, r0, 0
	sub.lt	r0, r5, r0
	sub.f	0, r1, 0
	sub.lt	r1, r5, r1

	// Unsigned divmod — remainder stored on stack
	sub	sp, sp, 4
	and	r2, sp, sp		// r2 = &local_rem
	bl	__udivmodsi4

	// Fix quotient sign
	sub	r5, r5, r5
	sub.f	0, r13, 0
	sub.n	r0, r5, r0

	// Fix remainder sign
	ld	r3, [sp, 0]
	sub	r5, r5, r5
	sub.f	0, r14, 0
	sub.n	r3, r5, r3
	add	sp, sp, 4

	// Store remainder if pointer non-null
	sub.f	0, r15, 0
	b.eq	.Ldm_ret
	st	r3, [r15, 0]
.Ldm_ret:
	ld	blink, [sp, 0]
	ld	r15, [sp, 12]
	ld	r14, [sp, 8]
	ld	r13, [sp, 4]
	j.d	[blink]
	add	sp, sp, 20		// delay slot
	.size	__divmodsi4, . - __divmodsi4

	NO_EXEC_STACK_DIRECTIVE
