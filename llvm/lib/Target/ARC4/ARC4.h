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

} // namespace llvm

#endif // LLVM_LIB_TARGET_ARC4_ARC4_H
