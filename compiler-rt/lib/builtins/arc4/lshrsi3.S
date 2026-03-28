//===-- lshrsi3.S - 32-bit logical right shift ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// unsigned __lshrsi3(unsigned a, int b)
//   r0 = value, r1 = shift amount  ->  r0 = a >> b (logical)
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__lshrsi3
	.type	__lshrsi3, @function
	.balign	4
__lshrsi3:
	and	r1, r1, 31
	sub.f	0, r1, 0
	b.eq	.Llsr_done
	and	lp_count, r1, r1
	lp	.Llsr_end
	lsr	r0, r0
.Llsr_end:
.Llsr_done:
	j	[blink]
	.size	__lshrsi3, . - __lshrsi3

	NO_EXEC_STACK_DIRECTIVE
