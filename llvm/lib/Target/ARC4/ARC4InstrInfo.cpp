//===- ARC4InstrInfo.cpp - ARC4 Instruction Information ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4InstrInfo.h"
#include "ARC4.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "ARC4GenInstrInfo.inc"

#define DEBUG_TYPE "arc4-inst-info"

void ARC4InstrInfo::anchor() {}

ARC4InstrInfo::ARC4InstrInfo(const ARC4Subtarget &ST)
    : ARC4GenInstrInfo(ST, RI), RI(ST) {}

void ARC4InstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I,
                                const DebugLoc &DL, Register DestReg,
                                Register SrcReg, bool KillSrc,
                                bool RenamableDest, bool RenamableSrc) const {
  assert(ARC4::GPR32RegClass.contains(SrcReg) &&
         "Only GPR32 src copy supported.");
  assert(ARC4::GPR32RegClass.contains(DestReg) &&
         "Only GPR32 dest copy supported.");
  // mov dest, src => and dest, src, src (rrr form: f=0, q=0)
  BuildMI(MBB, I, DL, get(ARC4::AND_rrr), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addReg(SrcReg, getKillRegState(KillSrc))
      .addImm(0)  // f
      .addImm(0); // q
}

void ARC4InstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register SrcReg,
    bool IsKill, int FrameIndex, const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  DebugLoc DL = MBB.findDebugLoc(I);
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOStore, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  assert(ARC4::GPR32RegClass.hasSubClassEq(RC) &&
         "Only support GPR32 stores to stack.");
  // st c, [b, offset] with offset=0, suffix v=0, D=0
  BuildMI(MBB, I, DL, get(ARC4::ST_rrs))
      .addReg(SrcReg, getKillRegState(IsKill))
      .addFrameIndex(FrameIndex)
      .addImm(0)  // offset
      .addImm(0)  // v (writeback)
      .addImm(0)  // D (cache bypass)
      .addMemOperand(MMO);
}

void ARC4InstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator I,
                                         Register DestReg, int FrameIndex,
                                         const TargetRegisterClass *RC,
                                         Register VReg, unsigned SubReg,
                                         MachineInstr::MIFlag Flags) const {
  DebugLoc DL = MBB.findDebugLoc(I);
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  MachineMemOperand *MMO = MF.getMachineMemOperand(
      MachinePointerInfo::getFixedStack(MF, FrameIndex),
      MachineMemOperand::MOLoad, MFI.getObjectSize(FrameIndex),
      MFI.getObjectAlign(FrameIndex));

  assert(ARC4::GPR32RegClass.hasSubClassEq(RC) &&
         "Only support GPR32 loads from stack.");
  // ld a, [b, shimm] with shimm=0, suffix X=0, W=0, E=0
  BuildMI(MBB, I, DL, get(ARC4::LD_rs))
      .addReg(DestReg, RegState::Define)
      .addFrameIndex(FrameIndex)
      .addImm(0)  // shimm offset
      .addImm(0)  // X (sign extend)
      .addImm(0)  // W (writeback)
      .addImm(0)  // E (cache bypass)
      .addMemOperand(MMO);
}
