//===- ARC4FrameLowering.cpp - ARC4 Frame Information -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4FrameLowering.h"
#include "ARC4Subtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "arc4-frame-lowering"

using namespace llvm;

void ARC4FrameLowering::emitPrologue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const ARC4InstrInfo &TII =
      *static_cast<const ARC4InstrInfo *>(ST.getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  uint64_t StackSize = MFI.getStackSize();
  if (StackSize == 0 && !MFI.hasCalls())
    return;

  // For functions that make calls, save BLINK and set up the frame.
  // ARC4 ABI: 16-byte backchain = [old_fp, blink, ...].
  // Minimal: just save blink and adjust SP.
  if (MFI.hasCalls()) {
    // st blink, [sp, -4]
    BuildMI(MBB, MBBI, DL, TII.get(ARC4::ST_rrs))
        .addReg(ARC4::BLINK)
        .addReg(ARC4::SP)
        .addImm(-4)  // offset
        .addImm(0)   // v (writeback)
        .addImm(0);  // D (cache bypass)
    // Adjust stack by 4 (for blink) + any stack frame
    StackSize += 4;
  }

  if (StackSize > 0) {
    // sub sp, sp, stacksize
    if (StackSize <= 255) {
      BuildMI(MBB, MBBI, DL, TII.get(ARC4::SUB_rrs), ARC4::SP)
          .addReg(ARC4::SP)
          .addImm(StackSize)
          .addImm(0); // f=0
    } else {
      // For large frames, use SUB_rrl with limm
      BuildMI(MBB, MBBI, DL, TII.get(ARC4::SUB_rrl), ARC4::SP)
          .addReg(ARC4::SP)
          .addImm(StackSize)
          .addImm(0)   // f=0
          .addImm(0);  // q=0
    }
  }
}

void ARC4FrameLowering::emitEpilogue(MachineFunction &MF,
                                     MachineBasicBlock &MBB) const {
  MachineFrameInfo &MFI = MF.getFrameInfo();
  const ARC4InstrInfo &TII =
      *static_cast<const ARC4InstrInfo *>(ST.getInstrInfo());
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  DebugLoc DL = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  uint64_t StackSize = MFI.getStackSize();
  bool HasCalls = MFI.hasCalls();

  if (StackSize == 0 && !HasCalls)
    return;

  if (HasCalls)
    StackSize += 4; // account for saved blink

  if (StackSize > 0) {
    // add sp, sp, stacksize
    if (StackSize <= 255) {
      BuildMI(MBB, MBBI, DL, TII.get(ARC4::ADD_rrs), ARC4::SP)
          .addReg(ARC4::SP)
          .addImm(StackSize)
          .addImm(0); // f=0
    } else {
      BuildMI(MBB, MBBI, DL, TII.get(ARC4::ADD_rrl), ARC4::SP)
          .addReg(ARC4::SP)
          .addImm(StackSize)
          .addImm(0)   // f=0
          .addImm(0);  // q=0
    }
  }

  if (HasCalls) {
    // ld blink, [sp, -4]  (relative to the restored SP)
    BuildMI(MBB, MBBI, DL, TII.get(ARC4::LD_rs))
        .addReg(ARC4::BLINK, RegState::Define)
        .addReg(ARC4::SP)
        .addImm(-4)  // offset
        .addImm(0)   // X (sign extend)
        .addImm(0)   // W (writeback)
        .addImm(0);  // E (cache bypass)
  }
}

bool ARC4FrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const TargetRegisterInfo *RegInfo = MF.getSubtarget().getRegisterInfo();
  return MF.getTarget().Options.DisableFramePointerElim(MF) ||
         MF.getFrameInfo().hasVarSizedObjects() ||
         MF.getFrameInfo().isFrameAddressTaken() ||
         RegInfo->hasStackRealignment(MF);
}
