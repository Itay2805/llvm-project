//===-- ARC4MCTargetDesc.cpp - ARC4 Target Descriptions -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCTargetDesc.h"
#include "ARC4MCAsmInfo.h"
#include "../InstPrinter/ARC4InstPrinter.h"
#include "../TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "ARC4GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "ARC4GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "ARC4GenRegisterInfo.inc"

static MCInstrInfo *createARC4MCInstrInfo() {
  auto *X = new MCInstrInfo();
  InitARC4MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createARC4MCRegisterInfo(const Triple &TT) {
  auto *X = new MCRegisterInfo();
  // blink (r31) is the return address register
  InitARC4MCRegisterInfo(X, ARC4::R31);
  return X;
}

static MCSubtargetInfo *createARC4MCSubtargetInfo(const Triple &TT,
                                                   StringRef CPU,
                                                   StringRef FS) {
  return createARC4MCSubtargetInfoImpl(TT, CPU, /*TuneCPU=*/CPU, FS);
}

static MCAsmInfo *createARC4MCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) {
  return new ARC4MCAsmInfo(TT);
}

static MCInstPrinter *createARC4MCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) {
  return new ARC4InstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4TargetMC() {
  Target &TheARC4Target = getTheARC4Target();

  RegisterMCAsmInfoFn X(TheARC4Target, createARC4MCAsmInfo);
  TargetRegistry::RegisterMCInstrInfo(TheARC4Target, createARC4MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(TheARC4Target, createARC4MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(TheARC4Target,
                                          createARC4MCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(TheARC4Target, createARC4MCInstPrinter);
  TargetRegistry::RegisterMCCodeEmitter(TheARC4Target,
                                        createARC4MCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(TheARC4Target, createARC4AsmBackend);
}
