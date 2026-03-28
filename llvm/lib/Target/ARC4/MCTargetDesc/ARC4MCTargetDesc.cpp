//===-- ARC4MCTargetDesc.cpp - ARC4 Target Descriptions -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4MCTargetDesc.h"
#include "ARC4InstPrinter.h"
#include "ARC4MCAsmInfo.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

#define GET_INSTRINFO_MC_DESC
#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "ARC4GenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "ARC4GenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "ARC4GenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createARC4MCInstrInfo() {
  MCInstrInfo *X = new MCInstrInfo();
  InitARC4MCInstrInfo(X);
  return X;
}

static MCRegisterInfo *createARC4MCRegisterInfo(const Triple & /*TT*/) {
  MCRegisterInfo *X = new MCRegisterInfo();
  InitARC4MCRegisterInfo(X, ARC4::BLINK);
  return X;
}

static MCSubtargetInfo *
createARC4MCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS) {
  std::string CPUName = std::string(CPU);
  if (CPUName.empty())
    CPUName = "generic";
  return createARC4MCSubtargetInfoImpl(TT, CPUName, /*TuneCPU*/ CPUName, FS);
}

static MCStreamer *createARC4MCStreamer(const Triple &T, MCContext &Context,
                                       std::unique_ptr<MCAsmBackend> &&MAB,
                                       std::unique_ptr<MCObjectWriter> &&OW,
                                       std::unique_ptr<MCCodeEmitter> &&Emitter) {
  if (!T.isOSBinFormatELF())
    llvm_unreachable("OS not supported");
  return createELFStreamer(Context, std::move(MAB), std::move(OW),
                           std::move(Emitter));
}

static MCInstPrinter *createARC4MCInstPrinter(const Triple & /*T*/,
                                              unsigned SyntaxVariant,
                                              const MCAsmInfo &MAI,
                                              const MCInstrInfo &MII,
                                              const MCRegisterInfo &MRI) {
  if (SyntaxVariant == 0)
    return new ARC4InstPrinter(MAI, MII, MRI);
  return nullptr;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4TargetMC() {
  RegisterMCAsmInfo<ARC4MCAsmInfo> X(getTheARC4Target());

  TargetRegistry::RegisterMCInstrInfo(getTheARC4Target(),
                                      createARC4MCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(getTheARC4Target(),
                                    createARC4MCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(getTheARC4Target(),
                                          createARC4MCSubtargetInfo);
  TargetRegistry::RegisterMCCodeEmitter(getTheARC4Target(),
                                        createARC4MCCodeEmitter);
  TargetRegistry::RegisterMCAsmBackend(getTheARC4Target(),
                                       createARC4AsmBackend);
  TargetRegistry::RegisterMCInstPrinter(getTheARC4Target(),
                                        createARC4MCInstPrinter);
  TargetRegistry::RegisterELFStreamer(getTheARC4Target(),
                                     createARC4MCStreamer);
}
