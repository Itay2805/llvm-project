//===-- ucmpsi2.S - 32-bit unsigned comparison ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __ucmpsi2(unsigned a, unsigned b)
//   Returns 0 if a < b, 1 if a == b, 2 if a > b.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__ucmpsi2
	.type	__ucmpsi2, @function
	.balign	4
__ucmpsi2:
	sub.f	0, r0, r1		// compare
	sub	r0, r0, r0		// r0 = 0
	b.lo	.Lucmp_done		// unsigned lower
	or	r0, r0, 1		// r0 = 1
	b.eq	.Lucmp_done
	add	r0, r0, 1		// r0 = 2
.Lucmp_done:
	j	[blink]
	.size	__ucmpsi2, . - __ucmpsi2

	NO_EXEC_STACK_DIRECTIVE
