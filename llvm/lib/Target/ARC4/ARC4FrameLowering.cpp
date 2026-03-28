//===-- ARC4FrameLowering.cpp - ARC4 Frame Lowering -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Minimal frame lowering for ARC4. Uses SP-relative addressing.
// Prologue: save blink, allocate stack frame.
// Epilogue: deallocate stack frame, restore blink, return.
//
//===----------------------------------------------------------------------===//

#include "ARC4FrameLowering.h"
#include "ARC4InstrInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

void ARC4FrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const ARC4InstrInfo &TII =
      *static_cast<const ARC4InstrInfo *>(MF.getSubtarget().getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL;

  bool SaveBlink = MFI.hasCalls();
  uint64_t StackSize = MFI.getStackSize();

  // Reserve an extra slot for blink if needed.
  if (SaveBlink)
    StackSize += 4;

  if (StackSize == 0)
    return;

  // Align stack size to 4 bytes.
  StackSize = alignTo(StackSize, 4);
  MFI.setStackSize(SaveBlink ? StackSize - 4 : StackSize);

  // SUB sp, sp, StackSize (allocate stack)
  BuildMI(MBB, MBBI, DL, TII.get(ARC4::CG_SUBri), ARC4::R28)
      .addReg(ARC4::R28)
      .addImm(StackSize);

  // Save blink at the bottom of the frame.
  if (SaveBlink) {
    BuildMI(MBB, MBBI, DL, TII.get(ARC4::CG_STri))
        .addReg(ARC4::R31)
        .addReg(ARC4::R28)
        .addImm(StackSize - 4);
  }
}

void ARC4FrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const ARC4InstrInfo &TII =
      *static_cast<const ARC4InstrInfo *>(MF.getSubtarget().getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  DebugLoc DL;

  bool SaveBlink = MFI.hasCalls();
  uint64_t StackSize = MFI.getStackSize();

  if (SaveBlink)
    StackSize += 4;

  StackSize = alignTo(StackSize, 4);

  if (StackSize == 0)
    return;

  // Restore blink before deallocating.
  if (SaveBlink) {
    BuildMI(MBB, MBBI, DL, TII.get(ARC4::CG_LDri), ARC4::R31)
        .addReg(ARC4::R28)
        .addImm(StackSize - 4);
  }

  // ADD sp, sp, StackSize (deallocate stack)
  BuildMI(MBB, MBBI, DL, TII.get(ARC4::CG_ADDri), ARC4::R28)
      .addReg(ARC4::R28)
      .addImm(StackSize);
}

MachineBasicBlock::iterator ARC4FrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  // Simply erase ADJCALLSTACKDOWN/UP -- stack adjustment is handled
  // by the fixed-size frame in emitPrologue/emitEpilogue.
  return MBB.erase(I);
}
