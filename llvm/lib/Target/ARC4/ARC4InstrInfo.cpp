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

// === Branch analysis helpers ===

static bool isUncondBranch(const MachineInstr &MI) {
  // B with q=0 (always) is unconditional
  if (MI.getOpcode() == ARC4::B) {
    // q operand is the second operand (offset, q, n)
    return MI.getOperand(1).getImm() == 0; // q == AL (always)
  }
  return false;
}

static bool isCondBranch(const MachineInstr &MI) {
  // BRcc_rr is always conditional
  if (MI.getOpcode() == ARC4::BRcc_rr)
    return true;
  // B with q != 0 is conditional
  if (MI.getOpcode() == ARC4::B)
    return MI.getOperand(1).getImm() != 0;
  return false;
}

bool ARC4InstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                  MachineBasicBlock *&TBB,
                                  MachineBasicBlock *&FBB,
                                  SmallVectorImpl<MachineOperand> &Cond,
                                  bool AllowModify) const {
  TBB = FBB = nullptr;
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return false;

  // Walk backwards through terminators.
  while (I->isTerminator()) {
    if (I->isDebugInstr()) {
      if (I == MBB.begin())
        return false;
      --I;
      continue;
    }

    if (I->getOpcode() == ARC4::RET || I->isReturn()) {
      // Return — can't analyze further.
      return true;
    }

    if (I->getOpcode() == ARC4::J_r || I->getOpcode() == ARC4::JL_r) {
      // Indirect branch/call — can't analyze.
      return true;
    }

    if (isUncondBranch(*I)) {
      // Unconditional branch.
      if (!AllowModify) {
        TBB = I->getOperand(0).getMBB();
        return false;
      }
      // If preceded by conditional branch, this is the fallthrough.
      // Delete any code after this.
      MachineBasicBlock::iterator Next = std::next(I);
      while (Next != MBB.end()) {
        MachineInstr &Dead = *Next;
        ++Next;
        Dead.eraseFromParent();
      }
      Cond.clear();
      FBB = nullptr;
      TBB = I->getOperand(0).getMBB();

      // If this is the only terminator, we're done.
      if (I == MBB.begin())
        return false;
      --I;
      if (!isCondBranch(*I)) {
        return false;
      }
      // Fall through to conditional branch handling.
    }

    if (I->getOpcode() == ARC4::BRcc_rr) {
      // Conditional branch: BRcc_rr target, lhs, rhs, cc
      if (!Cond.empty())
        return true; // Multiple conditional branches — bail.
      FBB = TBB;     // Previous uncond target becomes false branch.
      TBB = I->getOperand(0).getMBB();
      Cond.push_back(I->getOperand(1)); // lhs
      Cond.push_back(I->getOperand(2)); // rhs
      Cond.push_back(I->getOperand(3)); // cc
      if (I == MBB.begin())
        return false;
      --I;
      continue;
    }

    if (I->getOpcode() == ARC4::B && I->getOperand(1).getImm() != 0) {
      // Conditional B with condition code in q operand.
      // This shouldn't normally appear (we use BRcc_rr), but handle it.
      return true; // Can't analyze directly.
    }

    // Unknown terminator.
    return true;
  }

  return false;
}

unsigned ARC4InstrInfo::insertBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *TBB,
                                     MachineBasicBlock *FBB,
                                     ArrayRef<MachineOperand> Cond,
                                     const DebugLoc &DL,
                                     int *BytesAdded) const {
  assert(!BytesAdded && "Code size not handled.");
  assert(TBB && "insertBranch must not be told to insert a fallthrough.");

  if (Cond.empty()) {
    // Unconditional branch: B target, q=0(al), n=0(nd)
    BuildMI(&MBB, DL, get(ARC4::B)).addMBB(TBB).addImm(0).addImm(0);
    return 1;
  }

  // Conditional branch: BRcc_rr target, lhs, rhs, cc
  assert(Cond.size() == 3 && "ARC4 branch condition has 3 components.");
  MachineInstrBuilder MIB = BuildMI(&MBB, DL, get(ARC4::BRcc_rr));
  MIB.addMBB(TBB);
  MIB.add(Cond[0]); // lhs
  MIB.add(Cond[1]); // rhs
  MIB.add(Cond[2]); // cc

  if (!FBB)
    return 1;

  // Two-way: conditional + unconditional fallthrough
  BuildMI(&MBB, DL, get(ARC4::B)).addMBB(FBB).addImm(0).addImm(0);
  return 2;
}

unsigned ARC4InstrInfo::removeBranch(MachineBasicBlock &MBB,
                                     int *BytesRemoved) const {
  assert(!BytesRemoved && "Code size not handled.");
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return 0;

  if (!isUncondBranch(*I) && !isCondBranch(*I))
    return 0;

  I->eraseFromParent();

  I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return 1;

  if (!isCondBranch(*I))
    return 1;

  I->eraseFromParent();
  return 2;
}

bool ARC4InstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert(Cond.size() == 3 && "Invalid ARC4 branch condition.");
  auto CC = static_cast<ARC4CC::CondCode>(Cond[2].getImm());
  Cond[2].setImm(ARC4CC::getOppositeBranchCondition(CC));
  return false;
}

// === Stack slot operations ===

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
