//===-- ARC4TargetMachine.cpp - ARC4 Target Machine -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4TargetMachine.h"
#include "ARC4.h"
#include "ARC4MachineFunctionInfo.h"
#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4Target() {
  RegisterTargetMachine<ARC4TargetMachine> X(getTheARC4Target());
}

ARC4TargetMachine::ARC4TargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(T, TT.computeDataLayout(), TT,
                               CPU.empty() ? "generic" : CPU, FS, Options,
                               RM.value_or(Reloc::Static),
                               CM.value_or(CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU.empty() ? "generic" : std::string(CPU),
                std::string(FS), *this) {
  initAsmInfo();
}

namespace {
class ARC4PassConfig : public TargetPassConfig {
public:
  ARC4PassConfig(ARC4TargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override;
};
} // namespace

bool ARC4PassConfig::addInstSelector() {
  addPass(createARC4ISelDag(getTM<ARC4TargetMachine>()));
  return false;
}

TargetPassConfig *ARC4TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ARC4PassConfig(*this, PM);
}

MachineFunctionInfo *ARC4TargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return ARC4MachineFunctionInfo::create<ARC4MachineFunctionInfo>(Allocator, F,
                                                                  STI);
}
