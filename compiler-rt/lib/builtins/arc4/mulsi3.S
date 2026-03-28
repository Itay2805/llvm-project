//===-- mulsi3.S - 32-bit integer multiplication --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// int __mulsi3(int a, int b)
//   r0 = a (multiplicand), r1 = b (multiplier)  ->  r0 = a * b
//
// lsr.f simultaneously shifts the multiplier right, captures the old LSB
// in the Carry flag, and sets Zero when the multiplier reaches zero.
// add.cs accumulates only when C=1, b.jd.ne loops with a jump-delayed
// delay slot (executes only when branch taken), feeding the next lsr.f.
// Effective loop body: 3 instructions.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__mulsi3
	.type	__mulsi3, @function
	.balign	4
__mulsi3:
	sub	r2, r2, r2		// accumulator = 0
	lsr.f	r1, r1			// initial shift: C = bit 0, Z = (r1==0)
.Lmul_loop:
	add.cs	r2, r2, r0		// if old LSB set (C=1), acc += multiplicand
	add	r0, r0, r0		// multiplicand <<= 1
	b.jd.ne	.Lmul_loop		// loop while r1 != 0; delay slot on taken only
	lsr.f	r1, r1			// delay slot: next shift, sets C & Z
	j.d	[blink]
	and	r0, r2, r2		// delay slot: r0 = result
	.size	__mulsi3, . - __mulsi3

	NO_EXEC_STACK_DIRECTIVE
