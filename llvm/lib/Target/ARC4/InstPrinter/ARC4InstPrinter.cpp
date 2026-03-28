//===-- ARC4InstPrinter.cpp - ARC4 MCInst to assembly ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ARC4InstPrinter.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include "ARC4GenAsmWriter.inc"

void ARC4InstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  OS << getRegisterName(Reg);
}

void ARC4InstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                 StringRef Annot, const MCSubtargetInfo &STI,
                                 raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void ARC4InstPrinter::printOperand(const MCInst *MI, unsigned OpNum,
                                    raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNum);
  if (Op.isReg()) {
    printRegName(O, Op.getReg());
    return;
  }
  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }
  assert(Op.isExpr() && "Unknown operand kind");
  MAI.printExpr(O, *Op.getExpr());
}

void ARC4InstPrinter::printBranchTarget(const MCInst *MI, unsigned OpNum,
                                         raw_ostream &O) {
  printImm(MI, OpNum, O);
}

void ARC4InstPrinter::printImm(const MCInst *MI, unsigned OpNum,
                                raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNum);
  if (Op.isImm()) {
    O << Op.getImm();
    return;
  }
  assert(Op.isExpr() && "Expected immediate or expression");
  MAI.printExpr(O, *Op.getExpr());
}

// NN (delay-slot mode) operand:
//   0 = no delay slot (nothing printed)
//   1 = .d  — delay slot, instruction always executes
//   2 = .jd — jump delayed, delay slot executes only if branch taken
void ARC4InstPrinter::printDelaySlot(const MCInst *MI, unsigned OpNum,
                                      raw_ostream &O) {
  int64_t V = MI->getOperand(OpNum).getImm();
  if (V == 1)
    O << ".d";
  else if (V == 2)
    O << ".jd";
  else if (V != 0)
    O << ".d"; // fallback for unknown non-zero values
}

// Q (condition code) operand: 0 = always (no suffix printed).
void ARC4InstPrinter::printCondCode(const MCInst *MI, unsigned OpNum,
                                     raw_ostream &O) {
  static const char *const Names[] = {
      "",      // 0x00 AL (always)
      ".z",    // 0x01 EQ
      ".nz",   // 0x02 NE
      ".p",    // 0x03 PL (positive)
      ".n",    // 0x04 MI (negative)
      ".c",    // 0x05 CS (carry set)
      ".nc",   // 0x06 CC (carry clear)
      ".v",    // 0x07 VS
      ".nv",   // 0x08 VC
      ".gt",   // 0x09
      ".ge",   // 0x0A
      ".lt",   // 0x0B
      ".le",   // 0x0C
      ".hi",   // 0x0D
      ".ls",   // 0x0E
      ".pnz",  // 0x0F
  };
  int64_t V = MI->getOperand(OpNum).getImm();
  if (V >= 0 && V < (int64_t)(sizeof(Names) / sizeof(Names[0])))
    O << Names[V];
  else
    O << ".cc" << V;
}
