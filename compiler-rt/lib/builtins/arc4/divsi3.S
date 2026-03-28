//===-- divsi3.S - 32-bit signed integer division -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __divsi3(int a, int b)
//   r0 = dividend, r1 = divisor  ->  r0 = quotient
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__divsi3
	.type	__divsi3, @function
	.balign	4
__divsi3:
	sub	sp, sp, 12
	st	blink, [sp, 0]
	st	r13, [sp, 4]

	xor	r13, r0, r1		// r13[31] = sign of result

	sub	r5, r5, r5		// r5 = 0
	sub.f	0, r0, 0
	sub.lt	r0, r5, r0		// |dividend|
	sub.f	0, r1, 0
	sub.lt	r1, r5, r1		// |divisor|

	bl	__udivsi3

	sub	r5, r5, r5
	sub.f	0, r13, 0		// test result sign
	sub.n	r0, r5, r0		// negate if signs differed

	ld	blink, [sp, 0]
	ld	r13, [sp, 4]		// pipeline: separates ld blink / j [blink]
	j.d	[blink]
	add	sp, sp, 12		// delay slot: restore SP
	.size	__divsi3, . - __divsi3

	NO_EXEC_STACK_DIRECTIVE
