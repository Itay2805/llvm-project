//===-- ashrsi3.S - 32-bit arithmetic right shift -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __ashrsi3(int a, int b)
//   r0 = value, r1 = shift amount  ->  r0 = a >> b (arithmetic)
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__ashrsi3
	.type	__ashrsi3, @function
	.balign	4
__ashrsi3:
	and	r1, r1, 31
	sub.f	0, r1, 0
	b.eq	.Lasr_done
	and	lp_count, r1, r1
	lp	.Lasr_end
	asr	r0, r0
.Lasr_end:
.Lasr_done:
	j	[blink]
	.size	__ashrsi3, . - __ashrsi3

	NO_EXEC_STACK_DIRECTIVE
