//===-- muldi3.S - 64-bit integer multiplication --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// long long __muldi3(long long a, long long b)
//   a = r1:r0 (hi:lo), b = r3:r2 (hi:lo)  ->  result = r1:r0
//
// 64-bit shift-and-add.  Each iteration tests the multiplier LSB (r2[0]),
// conditionally accumulates the 64-bit multiplicand, shifts both operands,
// and exits early when the 64-bit multiplier reaches zero.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__muldi3
	.type	__muldi3, @function
	.balign	4
__muldi3:
	// Move multiplicand to r8:r9; accumulator in r0:r1
	and	r8, r0, r0		// r8 = a_lo
	and	r9, r1, r1		// r9 = a_hi
	sub	r0, r0, r0		// acc_lo = 0
	sub	r1, r1, r1		// acc_hi = 0
	// Prep first iteration LSB test
	and	r6, r2, 1
.Lmul64_loop:
	sub.f	0, r6, 0		// test multiplier LSB
	b.eq	.Lmul64_noadd
	add.f	r0, r0, r8		// acc_lo += mul_lo, C = carry
	adc	r1, r1, r9		// acc_hi += mul_hi + carry
.Lmul64_noadd:
	// 64-bit left shift multiplicand
	add.f	r8, r8, r8		// mul_lo <<= 1, C = old MSB
	adc	r9, r9, r9		// mul_hi = (mul_hi << 1) | C
	// 64-bit right shift multiplier (hi→lo carry via rrc)
	lsr.f	r3, r3			// mul_hi >>= 1, C = old r3[0]
	rrc	r2, r2			// mul_lo = (C<<31) | (mul_lo>>1)
	// Test if 64-bit multiplier is zero
	or.f	r6, r2, r3		// Z set if both halves zero
	b.jd.ne	.Lmul64_loop
	and	r6, r2, 1		// delay slot (taken only): prep next LSB
	j	[blink]			// r1:r0 = result
	.size	__muldi3, . - __muldi3

	NO_EXEC_STACK_DIRECTIVE
