//===- ARC4FrameLowering.h - Define frame lowering for ARC4 -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4FRAMELOWERING_H
#define LLVM_LIB_TARGET_ARC4_ARC4FRAMELOWERING_H

#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm {

class ARC4Subtarget;

class ARC4FrameLowering : public TargetFrameLowering {
public:
  ARC4FrameLowering(const ARC4Subtarget &ST)
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 0),
        ST(ST) {}

  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

protected:
  bool hasFPImpl(const MachineFunction &MF) const override;

private:
  const ARC4Subtarget &ST;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4FRAMELOWERING_H
