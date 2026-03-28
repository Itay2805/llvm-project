//===- ARC4InstrInfo.h - ARC4 Instruction Information -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4INSTRINFO_H
#define LLVM_LIB_TARGET_ARC4_ARC4INSTRINFO_H

#include "ARC4RegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "ARC4GenInstrInfo.inc"

namespace llvm {

class ARC4Subtarget;

class ARC4InstrInfo : public ARC4GenInstrInfo {
  const ARC4RegisterInfo RI;
  virtual void anchor();

public:
  ARC4InstrInfo(const ARC4Subtarget &);

  const ARC4RegisterInfo &getRegisterInfo() const { return RI; }

  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;

  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;

  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MI, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC, Register VReg,
      unsigned subReg = 0,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4INSTRINFO_H
