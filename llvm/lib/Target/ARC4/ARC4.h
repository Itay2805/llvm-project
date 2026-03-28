//===- ARC4.h - Top-level interface for ARC4 representation -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// ARC4 back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARC4_ARC4_H
#define LLVM_LIB_TARGET_ARC4_ARC4_H

#include "MCTargetDesc/ARC4MCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class ARC4TargetMachine;
class FunctionPass;
class PassRegistry;

FunctionPass *createARC4ISelDag(ARC4TargetMachine &TM,
                                CodeGenOptLevel OptLevel);
void initializeARC4AsmPrinterPass(PassRegistry &);
void initializeARC4DAGToDAGISelLegacyPass(PassRegistry &);

namespace ARC4CC {
enum CondCode {
  AL = 0,
  EQ = 1,
  NE = 2,
  PL = 3,
  MI = 4,
  HS = 5,
  LO = 6,
  VS = 7,
  VC = 8,
  GT = 9,
  GE = 10,
  LT = 11,
  LE = 12,
  HI = 13,
  LS = 14,
  PNZ = 15
};

inline CondCode getOppositeBranchCondition(CondCode CC) {
  switch (CC) {
  case EQ: return NE;
  case NE: return EQ;
  case GT: return LE;
  case GE: return LT;
  case LT: return GE;
  case LE: return GT;
  case HI: return LS;
  case LS: return HI;
  case HS: return LO;
  case LO: return HS;
  case PL: return MI;
  case MI: return PL;
  case VS: return VC;
  case VC: return VS;
  default: llvm_unreachable("Unknown condition code");
  }
}
} // namespace ARC4CC

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4_H
