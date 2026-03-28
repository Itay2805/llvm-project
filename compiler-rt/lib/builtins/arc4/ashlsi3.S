//===-- ashlsi3.S - 32-bit logical left shift -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __ashlsi3(int a, int b)
//   r0 = value, r1 = shift amount  ->  r0 = a << b
//
// Zero-overhead loop: 1-instruction body.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__ashlsi3
	.type	__ashlsi3, @function
	.balign	4
__ashlsi3:
	and	r1, r1, 31		// mask to 0-31
	sub.f	0, r1, 0
	b.eq	.Lshl_done
	and	lp_count, r1, r1	// lp_count = shift amount
	lp	.Lshl_end
	add	r0, r0, r0		// r0 <<= 1
.Lshl_end:
.Lshl_done:
	j	[blink]
	.size	__ashlsi3, . - __ashlsi3

	NO_EXEC_STACK_DIRECTIVE
