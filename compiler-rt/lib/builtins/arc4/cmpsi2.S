//===-- cmpsi2.S - 32-bit signed comparison -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __cmpsi2(int a, int b)
//   Returns 0 if a < b, 1 if a == b, 2 if a > b.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__cmpsi2
	.type	__cmpsi2, @function
	.balign	4
__cmpsi2:
	sub.f	0, r0, r1		// compare (flags persist through non-.f insns)
	sub	r0, r0, r0		// r0 = 0 (a < b case)
	b.lt	.Lcmp_done
	or	r0, r0, 1		// r0 = 1 (a == b case)
	b.eq	.Lcmp_done
	add	r0, r0, 1		// r0 = 2 (a > b case)
.Lcmp_done:
	j	[blink]
	.size	__cmpsi2, . - __cmpsi2

	NO_EXEC_STACK_DIRECTIVE
