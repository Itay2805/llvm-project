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

//===----------------------------------------------------------------------===//
// Branch analysis / insertion / removal
//
// The CMP instruction (CG_CMPrr/CG_CMPri) defines the implicit STATUS
// register, and the conditional branch (CG_BRcc) reads it.  The LLVM
// scheduler / branch-folder respects this implicit dependency
// automatically.  We do NOT include the CMP in the Cond vector —
// it stays in the block as a normal instruction, just like ARM's
// approach with CPSR.
//
// Cond encoding (matches the BRcc operand):
//   Cond[0] = condition code (immediate, the Q-field value 1..15)
//===----------------------------------------------------------------------===//

static unsigned reverseCC(unsigned CC) {
  switch (CC) {
  case 1:  return 2;   // EQ  <-> NE
  case 2:  return 1;
  case 3:  return 4;   // PL  <-> MI
  case 4:  return 3;
  case 5:  return 6;   // CS  <-> CC
  case 6:  return 5;
  case 7:  return 8;   // VS  <-> VC
  case 8:  return 7;
  case 9:  return 12;  // GT  <-> LE
  case 10: return 11;  // GE  <-> LT
  case 11: return 10;  // LT  <-> GE
  case 12: return 9;   // LE  <-> GT
  case 13: return 14;  // HI  <-> LS
  case 14: return 13;  // LS  <-> HI
  default: return 0;
  }
}

bool ARC4InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                  MachineBasicBlock *&TBB,
                                  MachineBasicBlock *&FBB,
                                  SmallVectorImpl<MachineOperand> &Cond,
                                  bool AllowModify) const {
  MachineBasicBlock::iterator I = MBB.end();
  if (I == MBB.begin())
    return false;
  --I;

  // Skip debug/non-terminator trailing instructions.
  while (I->isDebugInstr() || !I->isTerminator()) {
    if (I == MBB.begin())
      return false;
    --I;
  }

  // Unconditional branch.
  if (I->getOpcode() == ARC4::CG_BR) {
    TBB = I->getOperand(0).getMBB();
    // Look for conditional + unconditional pattern.
    if (I != MBB.begin()) {
      auto Prev = std::prev(I);
      if (Prev->getOpcode() == ARC4::CG_BRcc) {
        FBB = TBB;
        TBB = Prev->getOperand(0).getMBB();
        Cond.push_back(Prev->getOperand(1)); // CC
        return false;
      }
    }
    return false;
  }

  // Conditional branch alone (fallthrough on false).
  if (I->getOpcode() == ARC4::CG_BRcc) {
    TBB = I->getOperand(0).getMBB();
    Cond.push_back(I->getOperand(1)); // CC
    return false;
  }

  return true;
}

unsigned ARC4InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                     int *BytesRemoved) const {
  unsigned Count = 0;
  while (!MBB.empty()) {
    MachineInstr &Back = MBB.back();
    if (Back.isDebugInstr()) {
      Back.eraseFromParent();
      continue;
    }
    if (Back.getOpcode() != ARC4::CG_BR &&
        Back.getOpcode() != ARC4::CG_BRcc)
      break;
    Back.eraseFromParent();
    ++Count;
  }
  if (BytesRemoved)
    *BytesRemoved = Count * 4;
  return Count;
}

unsigned ARC4InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *TBB,
                                     MachineBasicBlock *FBB,
                                     ArrayRef<MachineOperand> Cond,
                                     const DebugLoc &DL,
                                     int *BytesAdded) const {
  unsigned Count = 0;
  if (Cond.empty()) {
    BuildMI(&MBB, DL, get(ARC4::CG_BR)).addMBB(TBB);
    ++Count;
  } else {
    BuildMI(&MBB, DL, get(ARC4::CG_BRcc))
        .addMBB(TBB)
        .addImm(Cond[0].getImm());
    ++Count;
    if (FBB) {
      BuildMI(&MBB, DL, get(ARC4::CG_BR)).addMBB(FBB);
      ++Count;
    }
  }
  if (BytesAdded)
    *BytesAdded = Count * 4;
  return Count;
}

bool ARC4InstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 1 && "Expected single CC operand");
  unsigned Rev = reverseCC(Cond[0].getImm());
  if (Rev == 0)
    return true;
  Cond[0].setImm(Rev);
  return false;
}
