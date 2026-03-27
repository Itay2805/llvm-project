//===-- ARC4TargetMachine.h - ARC4 Target Machine ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H
#define LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H

#include "ARC4Subtarget.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"

namespace llvm {

class ARC4TargetMachine : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  ARC4Subtarget Subtarget;

public:
  ARC4TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    std::optional<Reloc::Model> RM,
                    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                    bool JIT);

  const ARC4Subtarget *getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }

  MachineFunctionInfo *
  createMachineFunctionInfo(BumpPtrAllocator &Allocator, const Function &F,
                            const TargetSubtargetInfo *STI) const override;
};

} // namespace llvm

#endif
