//===-- udivdi3.S - 64-bit unsigned integer division ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// unsigned long long __udivdi3(unsigned long long a, unsigned long long b)
//   a = r1:r0, b = r3:r2  ->  quotient = r1:r0
//
// 64-bit binary restoring division with 128-bit carry-chain shift.
// Zero-overhead loop, 64 iterations.
//
//===----------------------------------------------------------------------===//

#include "../assembly.h"

	.text
	.globl	__udivdi3
	.type	__udivdi3, @function
	.balign	4
__udivdi3:
	// r1:r0 = dividend (becomes quotient), r3:r2 = divisor
	// r5:r4 = remainder, r7 = constant 1, r9:r8 = temp subtract
	sub	r4, r4, r4		// remainder_lo = 0
	sub	r5, r5, r5		// remainder_hi = 0
	sub	r7, r7, r7
	or	r7, r7, 1		// r7 = 1
	or	lp_count, r4, 64	// lp_count = 64
	lp	.Ludiv64_end
	// 128-bit left shift: (r5:r4:r1:r0) <<= 1
	add.f	r0, r0, r0		// dividend_lo <<= 1, C = old MSB
	adc.f	r1, r1, r1		// dividend_hi <<= 1 | C, C = old MSB
	adc.f	r4, r4, r4		// remainder_lo <<= 1 | C, C = old MSB
	adc	r5, r5, r5		// remainder_hi <<= 1 | C
	// 64-bit compare: remainder >= divisor?
	sub.f	r8, r4, r2		// temp_lo = rem_lo - div_lo
	sbc.f	r9, r5, r3		// temp_hi = rem_hi - div_hi - borrow
	b.lo	.Ludiv64_skip		// if rem < div (borrow), skip
	and	r4, r8, r8		// rem_lo = temp_lo
	and	r5, r9, r9		// rem_hi = temp_hi
	or	r0, r0, r7		// set quotient bit
.Ludiv64_skip:
.Ludiv64_end:
	j	[blink]
	.size	__udivdi3, . - __udivdi3

	NO_EXEC_STACK_DIRECTIVE
