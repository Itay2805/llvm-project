//===- ARC4FrameLowering.cpp - ARC4 Frame Information -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FrameLowering.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "arc4-frame-lowering"

using namespace llvm;

void ARC4FrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  // Minimal: for leaf functions with no stack, prologue is empty.
  // TODO: implement full frame setup for non-leaf functions.
}

void ARC4FrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  // Minimal: for leaf functions with no stack, epilogue is empty.
  // TODO: implement full frame teardown for non-leaf functions.
}

bool ARC4FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const TargetRegisterInfo *RegInfo = MF.getSubtarget().getRegisterInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MF.getFrameInfo().hasVarSizedObjects() ||
         MF.getFrameInfo().isFrameAddressTaken() ||
         RegInfo->hasStackRealignment(MF);
}
