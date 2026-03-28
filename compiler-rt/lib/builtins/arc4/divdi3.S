//===-- divdi3.S - 64-bit signed integer division -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// long long __divdi3(long long a, long long b)
//   a = r1:r0, b = r3:r2  ->  quotient = r1:r0
//
// Computes absolute values, calls __udivdi3, then fixes the sign.
// 64-bit negate = bitwise NOT + 1  (two's complement).
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__divdi3
	.type	__divdi3, @function
	.balign	4
__divdi3:
	sub	sp, sp, 12
	st	blink, [sp, 0]
	st	r13, [sp, 4]

	xor	r13, r1, r3		// r13[31] = sign of result

	sub	r5, r5, r5		// r5 = 0

	// 64-bit |dividend|
	sub.f	0, r1, 0		// test high word sign
	b.ge	.Ldivdi_abs_b
	xor	r0, r0, -1		// ~lo
	xor	r1, r1, -1		// ~hi
	add.f	r0, r0, 1		// lo + 1, C = carry
	adc	r1, r1, r5		// hi + carry
.Ldivdi_abs_b:
	// 64-bit |divisor|
	sub.f	0, r3, 0
	b.ge	.Ldivdi_call
	xor	r2, r2, -1
	xor	r3, r3, -1
	add.f	r2, r2, 1
	adc	r3, r3, r5
.Ldivdi_call:
	bl	__udivdi3

	// Fix sign
	sub.f	0, r13, 0
	b.ge	.Ldivdi_done
	sub	r5, r5, r5
	xor	r0, r0, -1
	xor	r1, r1, -1
	add.f	r0, r0, 1
	adc	r1, r1, r5
.Ldivdi_done:
	ld	blink, [sp, 0]
	ld	r13, [sp, 4]
	j.d	[blink]
	add	sp, sp, 12		// delay slot
	.size	__divdi3, . - __divdi3

	NO_EXEC_STACK_DIRECTIVE
