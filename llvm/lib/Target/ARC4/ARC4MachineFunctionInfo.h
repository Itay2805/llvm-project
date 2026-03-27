//===-- ARC4MachineFunctionInfo.h - ARC4 machine function info ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4MACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_ARC4_ARC4MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class ARC4MachineFunctionInfo : public MachineFunctionInfo {
  int VarArgsFrameIndex = 0;

public:
  ARC4MachineFunctionInfo() = default;
  explicit ARC4MachineFunctionInfo(const Function &F,
                                   const TargetSubtargetInfo *STI) {}

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override {
    return DestMF.cloneInfo<ARC4MachineFunctionInfo>(*this);
  }

  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
  void setVarArgsFrameIndex(int I) { VarArgsFrameIndex = I; }
};

} // namespace llvm

#endif
