//===-- udivmodsi4.S - 32-bit unsigned divmod -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// unsigned __udivmodsi4(unsigned a, unsigned b, unsigned *rem)
//   r0 = dividend, r1 = divisor, r2 = pointer to remainder
//   Returns quotient in r0, stores remainder via r2.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__udivmodsi4
	.type	__udivmodsi4, @function
	.balign	4
__udivmodsi4:
	and	r6, r2, r2		// r6 = rem pointer (save before r2 reuse)
	sub	r2, r2, r2		// r2 = 0  (remainder)
	sub	r3, r3, r3
	or	r3, r3, 1		// r3 = 1
	or	lp_count, r2, 32	// lp_count = 32
	lp	.Ludm_end
	add.f	r0, r0, r0
	adc	r2, r2, r2
	sub.f	0, r2, r1
	sub.hs	r2, r2, r1
	or.hs	r0, r0, r3
.Ludm_end:
	// Store remainder if pointer non-null
	sub.f	0, r6, 0
	b.eq	.Ludm_ret
	st	r2, [r6, 0]
.Ludm_ret:
	j	[blink]
	.size	__udivmodsi4, . - __udivmodsi4

	NO_EXEC_STACK_DIRECTIVE
