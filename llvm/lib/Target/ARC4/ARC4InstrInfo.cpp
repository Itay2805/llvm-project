//===-- ARC4InstrInfo.cpp - ARC4 Instruction Information -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4InstrInfo.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "ARC4GenInstrInfo.inc"

// RI must be initialized before base class since it's passed by reference.
// Use a static helper to work around the init order.
static const ARC4RegisterInfo StaticRI;

ARC4InstrInfo::ARC4InstrInfo(const ARC4Subtarget &ST)
    : ARC4GenInstrInfo(ST, StaticRI, ARC4::ADJCALLSTACKDOWN,
                       ARC4::ADJCALLSTACKUP) {}

void ARC4InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I,
                                const DebugLoc &DL, Register DstReg,
                                Register SrcReg, bool KillSrc,
                                bool RenamableDest, bool RenamableSrc) const {
  BuildMI(MBB, I, DL, get(ARC4::CG_MOVrr), DstReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}

void ARC4InstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register SrcReg,
    bool IsKill, int FI, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();
  BuildMI(MBB, I, DL, get(ARC4::CG_STri))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FI)
      .addImm(0);
}

void ARC4InstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register DestReg,
    int FI, const TargetRegisterClass *RC, Register VReg, unsigned SubReg,
    MachineInstr::MIFlag Flags) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();
  BuildMI(MBB, I, DL, get(ARC4::CG_LDri), DestReg)
      .addFrameIndex(FI)
      .addImm(0);
}
