//===- ARC4MachineFunctionInfo.h - ARC4 machine function info ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares ARC4-specific per-machine-function information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_ARC4_ARC4MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class ARC4FunctionInfo : public MachineFunctionInfo {
  virtual void anchor();
  int VarArgsFrameIndex = 0;
  unsigned ReturnStackOffset = 0;
  bool ReturnStackOffsetSet = false;

public:
  explicit ARC4FunctionInfo(const Function &F, const TargetSubtargetInfo *STI)
      : VarArgsFrameIndex(0), ReturnStackOffset(0),
        ReturnStackOffsetSet(false), MaxCallStackReq(0) {}
  ~ARC4FunctionInfo() {}

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;

  void setVarArgsFrameIndex(int off) { VarArgsFrameIndex = off; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }

  void setReturnStackOffset(unsigned value) {
    ReturnStackOffset = value;
    ReturnStackOffsetSet = true;
  }

  unsigned getReturnStackOffset() const {
    assert(ReturnStackOffsetSet && "Return stack offset not set");
    return ReturnStackOffset;
  }

  unsigned MaxCallStackReq;
};

} // end namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4MACHINEFUNCTIONINFO_H
