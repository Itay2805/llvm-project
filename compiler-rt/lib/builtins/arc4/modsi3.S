//===-- modsi3.S - 32-bit signed integer modulo ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __modsi3(int a, int b)
//   r0 = dividend, r1 = divisor  ->  r0 = remainder
//
// The sign of the remainder matches the sign of the dividend (C semantics).
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__modsi3
	.type	__modsi3, @function
	.balign	4
__modsi3:
	sub	sp, sp, 12
	st	blink, [sp, 0]
	st	r13, [sp, 4]

	and	r13, r0, r0		// r13 = original dividend (for sign)

	sub	r5, r5, r5
	sub.f	0, r0, 0
	sub.lt	r0, r5, r0		// |dividend|
	sub.f	0, r1, 0
	sub.lt	r1, r5, r1		// |divisor|

	bl	__umodsi3

	sub	r5, r5, r5
	sub.f	0, r13, 0		// test original dividend sign
	sub.n	r0, r5, r0		// negate if dividend was negative

	ld	blink, [sp, 0]
	ld	r13, [sp, 4]
	j.d	[blink]
	add	sp, sp, 12		// delay slot
	.size	__modsi3, . - __modsi3

	NO_EXEC_STACK_DIRECTIVE
