//===- ARC4CallingConv.td - Calling Conventions for ARC4 ----*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This describes the calling conventions for the ARC4 architecture.
// GCC A4 ABI: 8 arg regs (r0-r7), return in r0, 16-byte backchain.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// ARC4 Return Value Calling Convention
//===----------------------------------------------------------------------===//
def RetCC_ARC4 : CallingConv<[
  // i32 are returned in registers R0, R1, R2, R3
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

//===----------------------------------------------------------------------===//
// ARC4 Argument Calling Conventions
//===----------------------------------------------------------------------===//
def CC_ARC4 : CallingConv<[
  // Promote i8/i16 arguments to i32.
  CCIfType<[i8, i16], CCPromoteToType<i32>>,

  // The first 8 integer arguments are passed in integer registers.
  CCIfType<[i32], CCAssignToReg<[R0, R1, R2, R3, R4, R5, R6, R7]>>,

  // Integer values get stored in stack slots that are 4 bytes in
  // size and 4-byte aligned.
  CCIfType<[i32], CCAssignToStack<4, 4>>
]>;

// Callee-saved registers: r13-r25, GP(r26), FP(r27)
def CSR_ARC4 : CalleeSavedRegs<(add (sequence "R%u", 13, 25), GP, FP)>;
