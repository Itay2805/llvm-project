//===-- ARC4AsmPrinter.cpp - ARC4 Assembly Printer -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Converts MachineInstrs to MCInsts (via ARC4MCInstLower) and emits them
// through the MCStreamer. The existing MC layer handles encoding and emission.
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "ARC4MCInstLower.h"
#include "ARC4InstrInfo.h"
#include "ARC4TargetMachine.h"
#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "arc4-asm-printer"

namespace {

class ARC4AsmPrinter : public AsmPrinter {
  ARC4MCInstLower MCInstLowering;

public:
  explicit ARC4AsmPrinter(TargetMachine &TM,
                          std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)),
        MCInstLowering(OutContext, *this) {}

  StringRef getPassName() const override { return "ARC4 Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override;
};

} // namespace

void ARC4AsmPrinter::emitInstruction(const MachineInstr *MI) {
  // Skip pseudo-instructions that don't produce machine code
  if (MI->isDebugInstr() || MI->getOpcode() == ARC4::ADJCALLSTACKDOWN ||
      MI->getOpcode() == ARC4::ADJCALLSTACKUP)
    return;

  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeARC4AsmPrinter() {
  RegisterAsmPrinter<ARC4AsmPrinter> X(getTheARC4Target());
}
