//===-- udivsi3.S - 32-bit unsigned integer division ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// unsigned __udivsi3(unsigned a, unsigned b)
//   r0 = dividend, r1 = divisor  ->  r0 = quotient
//
// Binary restoring division with carry-chain shift and zero-overhead loop.
// 5-instruction loop body, 32 iterations, zero loop overhead.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__udivsi3
	.type	__udivsi3, @function
	.balign	4
__udivsi3:
	sub	r2, r2, r2		// r2 = 0  (remainder)
	sub	r3, r3, r3
	or	r3, r3, 1		// r3 = 1  (constant)
	or	lp_count, r2, 32	// lp_count = 32
	lp	.Ludiv_end
	add.f	r0, r0, r0		// dividend <<= 1, C = old MSB
	adc	r2, r2, r2		// remainder = (remainder << 1) | C
	sub.f	0, r2, r1		// compare remainder with divisor
	sub.hs	r2, r2, r1		// if rem >= div: subtract
	or.hs	r0, r0, r3		// if rem >= div: set quotient bit
.Ludiv_end:
	j	[blink]
	.size	__udivsi3, . - __udivsi3

	NO_EXEC_STACK_DIRECTIVE
