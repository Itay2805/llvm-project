//===-- ARC4TargetMachine.cpp - Define TargetMachine for ARC4 -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4TargetMachine.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4Target() {
  RegisterTargetMachine<ARC4TargetMachine> X(getTheARC4Target());
}

static const char *ARC4DataLayout =
    "e-m:e-p:32:32-i1:8:32-i8:8:32-i16:16:32-i32:32:32"
    "-f32:32:32-a:0:32-n32";

ARC4TargetMachine::ARC4TargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, ARC4DataLayout, TT,
                               CPU.empty() ? "generic" : CPU, FS, Options,
                               RM.value_or(Reloc::Static),
                               getEffectiveCodeModel(CM, CodeModel::Small), OL) {
  initAsmInfo();
}

ARC4TargetMachine::~ARC4TargetMachine() = default;

namespace {
class ARC4PassConfig : public TargetPassConfig {
public:
  ARC4PassConfig(ARC4TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}
};
} // end anonymous namespace

TargetPassConfig *ARC4TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ARC4PassConfig(*this, PM);
}
