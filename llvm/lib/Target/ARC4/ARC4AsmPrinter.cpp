//===- ARC4AsmPrinter.cpp - ARC4 LLVM assembly writer -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4.h"
#include "ARC4MCInstLower.h"
#include "ARC4Subtarget.h"
#include "ARC4TargetMachine.h"
#include "TargetInfo/ARC4TargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class ARC4AsmPrinter : public AsmPrinter {
  ARC4MCInstLower MCInstLowering;

public:
  static char ID;

  explicit ARC4AsmPrinter(TargetMachine &TM,
                          std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID),
        MCInstLowering(&OutContext, *this) {}

  StringRef getPassName() const override { return "ARC4 Assembly Printer"; }
  void emitInstruction(const MachineInstr *MI) override;

  bool runOnMachineFunction(MachineFunction &MF) override;
};

} // end anonymous namespace

void ARC4AsmPrinter::emitInstruction(const MachineInstr *MI) {
  // Expand pseudo instructions.
  switch (MI->getOpcode()) {
  case ARC4::ADJCALLSTACKDOWN:
  case ARC4::ADJCALLSTACKUP:
    // These are codegen-only pseudos; don't emit anything.
    return;
  case ARC4::BRcc_rr: {
    // BRcc_rr target, lhs, rhs, cc
    // Expand to: CMP_rr lhs, rhs, 1, 0  +  B target, cc, 0
    MCInst CmpInst;
    CmpInst.setOpcode(ARC4::CMP_rr);
    CmpInst.addOperand(MCInstLowering.LowerOperand(MI->getOperand(1))); // lhs
    CmpInst.addOperand(MCInstLowering.LowerOperand(MI->getOperand(2))); // rhs
    CmpInst.addOperand(MCOperand::createImm(1)); // f=1 (set flags)
    CmpInst.addOperand(MCOperand::createImm(0)); // q=0 (always)
    EmitToStreamer(*OutStreamer, CmpInst);

    MCInst BrInst;
    BrInst.setOpcode(ARC4::B);
    BrInst.addOperand(MCInstLowering.LowerOperand(MI->getOperand(0))); // target
    BrInst.addOperand(MCInstLowering.LowerOperand(MI->getOperand(3))); // cc
    BrInst.addOperand(MCOperand::createImm(0)); // n=0
    EmitToStreamer(*OutStreamer, BrInst);
    return;
  }
  default:
    break;
  }

  MCInst TmpInst;
  MCInstLowering.Lower(MI, TmpInst);
  EmitToStreamer(*OutStreamer, TmpInst);
}

bool ARC4AsmPrinter::runOnMachineFunction(MachineFunction &MF) {
  // Functions are 4-byte aligned.
  MF.ensureAlignment(Align(4));
  return AsmPrinter::runOnMachineFunction(MF);
}

char ARC4AsmPrinter::ID = 0;

INITIALIZE_PASS(ARC4AsmPrinter, "arc4-asm-printer", "ARC4 Assembly Printer",
                false, false)

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeARC4AsmPrinter() {
  RegisterAsmPrinter<ARC4AsmPrinter> X(getTheARC4Target());
}
