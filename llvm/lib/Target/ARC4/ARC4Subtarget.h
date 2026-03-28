//===- ARC4Subtarget.h - Define Subtarget for ARC4 -----------*- C++ -*----===//
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
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include <memory>
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "ARC4GenSubtargetInfo.inc"

namespace llvm {

class StringRef;
class TargetMachine;

class SelectionDAGTargetInfo;

class ARC4Subtarget : public ARC4GenSubtargetInfo {
  virtual void anchor();
  ARC4InstrInfo InstrInfo;
  ARC4FrameLowering FrameLowering;
  ARC4TargetLowering TLInfo;
  std::unique_ptr<const SelectionDAGTargetInfo> TSInfo;

public:
  ARC4Subtarget(const Triple &TT, const std::string &CPU,
                const std::string &FS, const TargetMachine &TM);

  ~ARC4Subtarget() override;

  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const ARC4InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const ARC4FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const ARC4TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const ARC4RegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }

  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return TSInfo.get();
  }
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4SUBTARGET_H
