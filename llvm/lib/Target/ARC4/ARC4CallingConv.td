//===-- ARC4CallingConv.td - Calling Conventions ARC4 ------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// ARC4 calling convention:
//   - Arguments: r0-r7 (first 8 i32 args), then stack (4-byte aligned)
//   - Return value: r0
//   - Callee-saved: r13-r26
//   - r27 = FP, r28 = SP, r31 = blink (return address)

def CC_ARC4 : CallingConv<[
  // Promote i1/i8/i16 to i32
  CCIfType<[i1, i8, i16], CCPromoteToType<i32>>,

  // First 8 i32 args go in r0-r7
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,

  // Remaining i32 args go to stack, 4-byte aligned
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

def RetCC_ARC4 : CallingConv<[
  // Return values in r0, r1 (for multi-word returns like smul_lohi)
  CCIfType<[i32], CCAssignToReg<[R0, R1]>>
]>;

// Callee-saved registers: r13-r25
// r26 (gp) is reserved, not callee-saved.
// blink (r31) is saved/restored explicitly by prologue/epilogue.
def CSR_ARC4 : CalleeSavedRegs<(add (sequence "R%u", 13, 25))>;
