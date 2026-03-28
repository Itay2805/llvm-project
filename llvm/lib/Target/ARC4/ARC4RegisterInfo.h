//===- ARC4RegisterInfo.h - ARC4 Register Information Impl ------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4REGISTERINFO_H
#define LLVM_LIB_TARGET_ARC4_ARC4REGISTERINFO_H

#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "ARC4GenRegisterInfo.inc"

namespace llvm {

class ARC4Subtarget;

struct ARC4RegisterInfo : public ARC4GenRegisterInfo {
  const ARC4Subtarget &ST;

  ARC4RegisterInfo(const ARC4Subtarget &);

  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;

  BitVector getReservedRegs(const MachineFunction &MF) const override;

  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;

  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;

  Register getFrameRegister(const MachineFunction &MF) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4REGISTERINFO_H
