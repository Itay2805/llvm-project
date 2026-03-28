//===-- ARC4TargetMachine.h - Define TargetMachine for ARC4 -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H
#define LLVM_LIB_TARGET_ARC4_ARC4TARGETMACHINE_H

#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include <optional>

namespace llvm {

class ARC4TargetMachine : public CodeGenTargetMachineImpl {
public:
  ARC4TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                    StringRef FS, const TargetOptions &Options,
                    std::optional<Reloc::Model> RM,
                    std::optional<CodeModel::Model> CM,
                    CodeGenOptLevel OL, bool JIT);
  ~ARC4TargetMachine() override;

  const TargetSubtargetInfo *getSubtargetImpl(const Function &) const override {
    return nullptr; // No codegen support yet
  }

  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
};

} // namespace llvm

#endif
