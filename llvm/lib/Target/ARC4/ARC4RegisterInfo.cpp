//===- ARC4RegisterInfo.cpp - ARC4 Register Information ---------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4RegisterInfo.h"
#include "ARC4.h"
#include "ARC4FrameLowering.h"
#include "ARC4Subtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/Support/Debug.h"

using namespace llvm;

#define DEBUG_TYPE "arc4-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "ARC4GenRegisterInfo.inc"

ARC4RegisterInfo::ARC4RegisterInfo(const ARC4Subtarget &ST)
    : ARC4GenRegisterInfo(ARC4::BLINK), ST(ST) {}

const MCPhysReg *
ARC4RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_ARC4_SaveList;
}

BitVector ARC4RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());

  Reserved.set(ARC4::SP);
  Reserved.set(ARC4::GP);
  Reserved.set(ARC4::ILINK1);
  Reserved.set(ARC4::ILINK2);
  Reserved.set(ARC4::BLINK);
  Reserved.set(ARC4::FP);

  return Reserved;
}

bool ARC4RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj, unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const TargetFrameLowering *TFI = getFrameLowering(MF);

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MF.getFrameInfo().getObjectOffset(FrameIndex);
  int StackSize = MF.getFrameInfo().getStackSize();

  // Determine the base register.
  Register FrameReg = getFrameRegister(MF);

  if (!TFI->hasFP(MF))
    Offset = StackSize + Offset;

  // Replace the frame index with the frame register and offset.
  MI.getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
  if (FIOperandNum + 1 < MI.getNumOperands() &&
      MI.getOperand(FIOperandNum + 1).isImm()) {
    Offset += MI.getOperand(FIOperandNum + 1).getImm();
    MI.getOperand(FIOperandNum + 1).setImm(Offset);
  }

  return false;
}

Register ARC4RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  const TargetFrameLowering *TFI = getFrameLowering(MF);
  return TFI->hasFP(MF) ? ARC4::FP : ARC4::SP;
}

const uint32_t *
ARC4RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const {
  return CSR_ARC4_RegMask;
}
