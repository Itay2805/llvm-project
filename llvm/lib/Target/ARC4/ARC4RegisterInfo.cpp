//===-- ARC4RegisterInfo.cpp - ARC4 Register Information -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4RegisterInfo.h"
#include "ARC4FrameLowering.h"
#include "ARC4InstrInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "ARC4GenRegisterInfo.inc"

ARC4RegisterInfo::ARC4RegisterInfo() : ARC4GenRegisterInfo(ARC4::R31) {}

const MCPhysReg *
ARC4RegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  return CSR_ARC4_SaveList;
}

const uint32_t *
ARC4RegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const {
  return CSR_ARC4_RegMask;
}

BitVector ARC4RegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(ARC4::R26);  // GP (global pointer)
  Reserved.set(ARC4::R27);  // FP
  Reserved.set(ARC4::R28);  // SP
  Reserved.set(ARC4::R29);  // ilink1
  Reserved.set(ARC4::R30);  // ilink2
  Reserved.set(ARC4::R31);  // blink
  Reserved.set(ARC4::STATUS); // implicit condition codes
  Reserved.set(ARC4::R60);  // lp_count
  Reserved.set(ARC4::R61);  // sentinel
  Reserved.set(ARC4::R62);  // sentinel
  Reserved.set(ARC4::R63);  // sentinel
  // Reserve extension registers r32-r59
  for (unsigned i = ARC4::R32; i <= ARC4::R59; ++i)
    Reserved.set(i);
  return Reserved;
}

bool ARC4RegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II,
                                           int SPAdj, unsigned FIOperandNum,
                                           RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  int FrameIdx = MI.getOperand(FIOperandNum).getIndex();
  int Offset = MFI.getObjectOffset(FrameIdx) + MFI.getStackSize();
  Offset += MI.getOperand(FIOperandNum + 1).getImm();

  MI.getOperand(FIOperandNum).ChangeToRegister(ARC4::R28, false); // SP
  MI.getOperand(FIOperandNum + 1).ChangeToImmediate(Offset);
  return false;
}

Register ARC4RegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return ARC4::R28; // Use SP directly for MVP (hasFP=false)
}
