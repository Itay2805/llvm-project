//===-- ARC4InstPrinter.cpp - Convert ARC4 MCInst to asm ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4InstPrinter.h"
#include "ARC4MCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#define PRINT_ALIAS_INSTR
#include "ARC4GenAsmWriter.inc"

void ARC4InstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
}

void ARC4InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                StringRef Annot, const MCSubtargetInfo &STI,
                                raw_ostream &O) {
  // Suffix operands (f, q, n) are real MCInst operands but are NOT referenced
  // in AsmString, so printInstruction/printAliasInstr won't print them.
  // Suffix printing will be added in a follow-up patch; for now the output
  // matches the pre-refactor behavior (no suffix in printed output).
  if (!printAliasInstr(MI, Address, O))
    printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void ARC4InstPrinter::printLdSsAddr(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O) {
  // LD_ss shimm+shimm form: effective address = 2 * shimm.
  // Print the effective address so round-trip is correct.
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isImm())
    O << (Op.getImm() * 2);
  else if (Op.isExpr())
    MAI.printExpr(O, *Op.getExpr());
}

void ARC4InstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                   raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg())
    printRegName(O, Op.getReg());
  else if (Op.isImm())
    O << Op.getImm();
  else if (Op.isExpr())
    MAI.printExpr(O, *Op.getExpr());
}
