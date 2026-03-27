//===-- ARC4MCInstLower.h - Lower MachineInstr to MCInst --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4MCINSTLOWER_H
#define LLVM_LIB_TARGET_ARC4_ARC4MCINSTLOWER_H

#include "llvm/CodeGen/MachineOperand.h"

namespace llvm {

class AsmPrinter;
class MCContext;
class MCInst;
class MCOperand;
class MachineInstr;

class ARC4MCInstLower {
  MCContext &Ctx;
  AsmPrinter &Printer;

public:
  ARC4MCInstLower(MCContext &C, AsmPrinter &P) : Ctx(C), Printer(P) {}
  void Lower(const MachineInstr *MI, MCInst &OutMI) const;

private:
  MCOperand LowerOperand(const MachineOperand &MO) const;
  MCOperand LowerSymbolOperand(const MachineOperand &MO) const;
};

} // namespace llvm

#endif
