//===-- umodsi3.S - 32-bit unsigned integer modulo ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// unsigned __umodsi3(unsigned a, unsigned b)
//   r0 = dividend, r1 = divisor  ->  r0 = remainder
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__umodsi3
	.type	__umodsi3, @function
	.balign	4
__umodsi3:
	sub	r2, r2, r2		// r2 = 0  (remainder)
	or	lp_count, r2, 32	// lp_count = 32
	lp	.Lumod_end
	add.f	r0, r0, r0		// dividend <<= 1, C = old MSB
	adc	r2, r2, r2		// remainder = (remainder << 1) | C
	sub.f	0, r2, r1		// compare remainder with divisor
	sub.hs	r2, r2, r1		// if rem >= div: subtract
.Lumod_end:
	j.d	[blink]
	and	r0, r2, r2		// delay slot: r0 = remainder
	.size	__umodsi3, . - __umodsi3

	NO_EXEC_STACK_DIRECTIVE
