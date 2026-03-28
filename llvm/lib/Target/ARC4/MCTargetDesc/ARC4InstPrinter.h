//===-- ARC4InstPrinter.h - Convert ARC4 MCInst to asm --------*- C++ -*---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4INSTPRINTER_H
#define LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4INSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class ARC4InstPrinter : public MCInstPrinter {
public:
  ARC4InstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                  const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &O) override;
  void printRegName(raw_ostream &O, MCRegister Reg) override;

  std::pair<const char *, uint64_t> getMnemonic(const MCInst &MI) const override;
  bool printAliasInstr(const MCInst *MI, uint64_t Address, raw_ostream &O);
  void printInstruction(const MCInst *MI, uint64_t Address, raw_ostream &O);
  static const char *getRegisterName(MCRegister Reg);

  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_MCTARGETDESC_ARC4INSTPRINTER_H
