//===-- ARC4Subtarget.h - ARC4 Subtarget Information ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4SUBTARGET_H
#define LLVM_LIB_TARGET_ARC4_ARC4SUBTARGET_H

#include "ARC4FrameLowering.h"
#include "ARC4ISelLowering.h"
#include "ARC4InstrInfo.h"
#include "ARC4SelectionDAGInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "ARC4GenSubtargetInfo.inc"

namespace llvm {

class ARC4Subtarget : public ARC4GenSubtargetInfo {
  ARC4InstrInfo InstrInfo;
  ARC4FrameLowering FrameLowering;
  ARC4TargetLowering TLInfo;
  ARC4SelectionDAGInfo TSInfo;

public:
  ARC4Subtarget(const Triple &TT, StringRef CPU, StringRef FS,
                const TargetMachine &TM);

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const ARC4InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const ARC4FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const ARC4RegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const ARC4TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const ARC4SelectionDAGInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
};

} // namespace llvm

#endif
